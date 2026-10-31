/*

    Uptooda - free application for uploading images/files to the Internet

    Copyright 2007-2025 Sergey Svistunov (zenden2k@gmail.com)

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

*/

#include "AvcodecFrameGrabber.h"

#include <boost/format.hpp>

#include "Core/Images/AbstractImage.h"
#include "Core/Utils/CoreUtils.h"
#include "FrameGrabberException.h"
#include "Core/i18n/Translator.h"

extern "C" {

#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libswscale/swscale.h>
#include <libavutil/imgutils.h>

}

class AvcodecVideoFrame : public AbstractVideoFrame
{
public :
    AvcodecVideoFrame(AVFrame* frame, unsigned char* data, unsigned int dataSize, int64_t time, int width, int height) {
        time_ = time;
        frame_ = frame;
        data_ = data;
        dataSize_ = dataSize;
        width_ = width;
        height_ = height;
    }

    ~AvcodecVideoFrame() override {
        frame_ = nullptr;
    }

    bool saveToFile(const std::string& fileName) const override {
        std::unique_ptr<AbstractImage> img(AbstractImage::createImage());
        if (!img) {
            return false;
        }
        if (!img->loadFromRawData(AbstractImage::dfRGB888, width_, height_, data_, dataSize_,
                reinterpret_cast<void*>(static_cast<intptr_t>(frame_->linesize[0])))) {
            return false;
        }
        return img->saveToFile(fileName);
    }

    std::unique_ptr<AbstractImage> toImage() const override {
        std::unique_ptr<AbstractImage> img(AbstractImage::createImage());
        if (!img) {
            return nullptr;
        }
        if (!img->loadFromRawData(AbstractImage::dfRGB888, width_, height_, data_, dataSize_,
                reinterpret_cast<void*>(static_cast<intptr_t>(frame_->linesize[0])))) {
            return nullptr;
        }
        return img;
    }

protected:
    unsigned char* data_;
    unsigned int dataSize_;
    AVFrame* frame_;
    DISALLOW_COPY_AND_ASSIGN(AvcodecVideoFrame);
};


class AvcodecFrameGrabberPrivate
{
protected:
    int videoStream;
    AVFormatContext* pFormatCtx;
    AVCodecContext* pCodecCtx;
    const AVCodec* pCodec;
    AVFrame* pFrame;
    AVFrame* pFrameRGB;
    AVPacket packet {};
    AVFormatContext* ic;
    uint8_t* buffer;
    bool frameFinished;
    int headerlen;
    struct SwsContext* img_convert_ctx;
    int numBytes;
    int allocatedFrameWidth_;
    int allocatedFrameHeight_;
    size_t allocatedBufferSize_;
    bool NeedStop;
    bool SeekToKeyFrame;
    bool seekByBytes;
    std::unique_ptr<AvcodecVideoFrame> currentFrame_;
    int64_t duration_;
    std::unique_ptr<AvcodecFrameGrabber::StreamInfo> streamInfo_;
    static bool initialized_;
public:
    friend class AvcodecFrameGrabber;

    AvcodecFrameGrabberPrivate() {
        NeedStop = false;
        SeekToKeyFrame = true;
        seekByBytes = false;
        img_convert_ctx = nullptr;
        currentFrame_ = nullptr;
        buffer = nullptr;
        pFrameRGB = nullptr;
        pFrame = nullptr;
        pCodecCtx = nullptr;
        videoStream = 0;
        pFormatCtx = nullptr;
        pCodec = nullptr;
        frameFinished = false;
        ic = nullptr;
        headerlen = 0;
        duration_ = 0;
        allocatedFrameWidth_ = 0;
        allocatedFrameHeight_ = 0;
        numBytes = 0;
        allocatedBufferSize_ = 0;
    }

    ~AvcodecFrameGrabberPrivate() {
        close();
    }

    static std::string getFFmpegErrorMessage(int code) {
        char strBuf[256] = {};
        av_strerror(code, strBuf, sizeof(strBuf));
        return strBuf;
    }

    static bool isImageCodec(AVCodecID codec)
    {
        return codec == AV_CODEC_ID_PNG
            /* || codec == AV_CODEC_ID_MJPEG*/;
    }

    static void checkReturnCode(int code, const std::string& msg) {
        if (code < 0) {
            throw FrameGrabberException(msg + "\n" + getFFmpegErrorMessage(code));
        }
    }

    static std::string convertCodecName(const std::string& name) {
        if (name == "mpeg2video") {
            return "mpeg2";
        }
        if (name == "mpeg1video") {
            return "mpeg1";
        }
        return name;
    }


    bool open(const std::string& fileName) {
        int numOfFrames = 5;

        // Register all formats and codecs
        if (! initialized_) {
            //av_register_all();

            initialized_ = true;
        }
        pFormatCtx = nullptr;

        // Open video file
        int result = avformat_open_input(&pFormatCtx, fileName.c_str(), nullptr, nullptr);
        if (result != 0) {
            std::string errMsg = getFFmpegErrorMessage(result);
            throw FrameGrabberException(
                boost::str(boost::format("Cannot open input file '%s', error code=%d\n%s") % fileName % result % errMsg));
        }

        //AVInputFormat *inputFormat = av_find_input_format( fileName.c_str() );

        // Retrieve stream information
        checkReturnCode(avformat_find_stream_info(pFormatCtx, nullptr), "Couldn't find stream information.");

        if (pFormatCtx->iformat) {
            seekByBytes = !!(pFormatCtx->iformat->flags & AVFMT_TS_DISCONT);
        }

        // Find the first video stream
        videoStream = -1;
        for (size_t i = 0; i < pFormatCtx->nb_streams; i++) {
            AVCodecParameters* codecpar = pFormatCtx->streams[i]->codecpar;
            if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO
                    && !isImageCodec(codecpar->codec_id)) {
                videoStream = i;
                break;
            }
        }

        if (videoStream == -1) {
            throw FrameGrabberException(_("Could not find a video stream."));
        }

        // Dump information about file onto standard error
        //av_dump_format(pFormatCtx, 0, fileName.c_str(), false);

        // Get a pointer to the codec context for the video stream
        AVCodecParameters* pars = pFormatCtx->streams[videoStream]->codecpar;

        // Find the decoder for the video stream
        pCodec = avcodec_find_decoder(pars->codec_id);
        if (pCodec == nullptr) {
            throw FrameGrabberException("Couldn't find video codec.");
        }
        streamInfo_ = std::make_unique<AvcodecFrameGrabber::StreamInfo>();
        streamInfo_->width = pars->width;
        streamInfo_->height = pars->height;
        streamInfo_->codecName = pCodec->name;

        const AVCodecDescriptor* desc = avcodec_descriptor_get(pars->codec_id);
        streamInfo_->codecName = convertCodecName(desc ? desc->name : pCodec->name);

        pCodecCtx = avcodec_alloc_context3(pCodec);

        if (!pCodecCtx) {
            throw FrameGrabberException("Failed to allocate codec context");
        }

        int ret;

        checkReturnCode(avcodec_parameters_to_context(pCodecCtx, pars),
            "Failed to copy video codec parameters to decoder context"
        );


        //pCodecCtx->refcounted_frames = 1;
        // Open codec
        checkReturnCode(avcodec_open2(pCodecCtx, pCodec, nullptr), "Failed to open video codec");

        // Hack to correct wrong frame rates that seem to be generated by some codecs
        if (pCodecCtx->time_base.num > 1000 && pCodecCtx->time_base.den == 1) {
            pCodecCtx->time_base.den = 1000;
        }

        // Allocate video frame
        pFrame = av_frame_alloc();

        // Allocate an AVFrame structure
        pFrameRGB = av_frame_alloc();

        if (pFrameRGB == nullptr) {
            throw FrameGrabberException("Failed to allocate video frame");
        }
        memset(pFrameRGB->linesize, 0, sizeof(pFrameRGB->linesize));

        // Determine required buffer size and allocate buffer
        allocatedFrameWidth_ = pCodecCtx->width;
        allocatedFrameHeight_ = pCodecCtx->height;
        numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 4) + 64;

        buffer = new uint8_t[numBytes];
        allocatedBufferSize_ = numBytes;
        memset(buffer, 0, numBytes);
        headerlen = sprintf(reinterpret_cast<char*>(buffer), "P6\n%d %d\n255\n", pCodecCtx->width, pCodecCtx->height);

        av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer + headerlen, AV_PIX_FMT_RGB24,
                             pCodecCtx->width, pCodecCtx->height, 4);

        //AVRational ss = pFormatCtx->streams[videoStream]->time_base;

        ic = pFormatCtx;

        /*AVDictionaryEntry* tag = nullptr;
        while ((tag = av_dict_get(pFormatCtx->streams[videoStream]->metadata, "", tag,AV_DICT_IGNORE_SUFFIX)) != 0) {
        }*/

        duration_ = ic->duration;

        /*
        if (duration_ < 0) {
            LOG(WARNING) << "Invalid duration";
            duration_ = 0;
        }*/

        int64_t my_start_time = static_cast<uint64_t>(static_cast<double>(ic->duration) / numOfFrames * 1.6);

        AVRational rat = {1, AV_TIME_BASE};
        int64_t seek_target = av_rescale_q(my_start_time, rat, pFormatCtx->streams[videoStream]->time_base);
        if (seekByBytes) {
            uint64_t size = avio_size(pFormatCtx->pb);
            seek_target = static_cast<uint64_t>(static_cast<double>(my_start_time) / ic->duration * size);
        }
        if (avformat_seek_file(pFormatCtx, videoStream, 0, seek_target, seek_target,
                               seekByBytes ? AVSEEK_FLAG_BYTE : 0) < 0) {
            throw FrameGrabberException(str(
                boost::format("avformat_seek_file failed to seek to position %1% seekByBytes %2%")
                % seek_target % seekByBytes));
        }
        avcodec_flush_buffers(pCodecCtx);

        avcodec_flush_buffers(pCodecCtx);
        frameFinished = false;
        while (av_read_frame(pFormatCtx, &packet) >= 0) {
            if (packet.stream_index == videoStream) {
                //av_frame_unref(pFrame);

                ret = avcodec_send_packet(pCodecCtx, &packet);
                if (ret < 0) {
                    LOG(WARNING) << "Error sending a packet for decoding" << std::endl << getFFmpegErrorMessage(ret);
                } else {
                    ret = avcodec_receive_frame(pCodecCtx, pFrame);
                    av_packet_unref(&packet);

                    if (ret == AVERROR(EAGAIN)) {
                        continue;
                        //return false;
                    } else if (ret == AVERROR_EOF) {

                        return false;
                    } else if (ret < 0) {

                        throw FrameGrabberException("Error during decoding");
                    }
                    frameFinished = true;

                    break;
                }
            }

            // Free the packet that was allocated by av_read_frame
            av_packet_unref(&packet);
        }
        return true;
    }

    void close() {
        // Free the RGB image
        delete[] buffer;

        if (pFrameRGB) {
            av_frame_free(&pFrameRGB);
        }
        if (img_convert_ctx) {
            sws_freeContext(img_convert_ctx);
        }

        if (pFrame) {
            // Free the YUV frame
            av_frame_free(&pFrame);
        }

        if (pCodecCtx) {
            // Close the codec
            avcodec_free_context(&pCodecCtx);
        }

        if (pFormatCtx) {
            // Close the video file
            avformat_close_input(&pFormatCtx);
        }
    }

    bool seek(int64_t time) {
        if (allocatedFrameWidth_ != pCodecCtx->width || allocatedFrameHeight_ != pCodecCtx->height) {
            numBytes = av_image_get_buffer_size(AV_PIX_FMT_RGB24, pCodecCtx->width, pCodecCtx->height, 4) + 64;
            if (allocatedBufferSize_ < numBytes) {
                delete[] buffer;
                buffer = nullptr;
                buffer = new uint8_t[numBytes];
                allocatedBufferSize_ = numBytes;
                memset(buffer, 0, numBytes);
            }
            allocatedFrameWidth_ = pCodecCtx->width;
            allocatedFrameHeight_ = pCodecCtx->height;
        }
        av_image_fill_arrays(pFrameRGB->data, pFrameRGB->linesize, buffer + headerlen, AV_PIX_FMT_RGB24,
                             pCodecCtx->width, pCodecCtx->height, 4);

        SeekToKeyFrame = true;

        int64_t seek_target = time;
        if (seek_target == 0) seek_target = 1;

        AVRational rat = {1, AV_TIME_BASE};
        //seekByBytes = true;
        if (seekByBytes) {
            uint64_t size = avio_size(pFormatCtx->pb);
            seek_target = static_cast<int64_t>(static_cast<double>(seek_target) / ic->duration * size);
        } else {
            seek_target = av_rescale_q(seek_target, rat, pFormatCtx->streams[videoStream]->time_base);
        }
        int res = avformat_seek_file(pFormatCtx, videoStream, 0, seek_target, seek_target,
            seekByBytes ? AVSEEK_FLAG_BYTE : 0);
        if (res < 0) {
            std::string errMsg = getFFmpegErrorMessage(res);
            throw FrameGrabberException(str(
                boost::format("avformat_seek_file failed to seek to position %1% seekByBytes %2%\n%3%")
                % seek_target % seekByBytes % errMsg));
        }

        avcodec_flush_buffers(pCodecCtx);
        // FIXME:
        bool isQt = AbstractImage::factoryName() == "QtImage";
        AVPixelFormat pixelFormat = isQt ? AV_PIX_FMT_RGB24 : AV_PIX_FMT_BGR24;

        int ret = 0;
        while (av_read_frame(pFormatCtx, &packet) >= 0) {
            // Is this a packet from the video stream?
            if (packet.stream_index == videoStream) {
                frameFinished = false;
                //av_frame_unref(pFrame);

                ret = avcodec_send_packet(pCodecCtx, &packet);
                if (ret < 0) {
                    av_packet_unref(&packet);
                    throw FrameGrabberException("Error sending a packet for decoding");
                }

                ret = avcodec_receive_frame(pCodecCtx, pFrame);

                if (ret == AVERROR(EAGAIN)) {
                    av_packet_unref(&packet);
                    continue;
                }
                if (ret == AVERROR_EOF) {
                    av_packet_unref(&packet);
                    return false;
                }
                if (ret < 0) {
                    av_packet_unref(&packet);
                    throw FrameGrabberException("Error during decoding");
                }
                frameFinished = true;

                if (frameFinished) {

                    //write_frame_to_file(pFrame, pCodecCtx->width,pCodecCtx->height,rand()%100);
                    // Convert the image into YUV format that SDL uses
                    if (img_convert_ctx == nullptr) {
                        int w = pCodecCtx->width;
                        int h = pCodecCtx->height;
                        int dstWidth = w;
                        int dstHeight = h;

                        img_convert_ctx = sws_getCachedContext(img_convert_ctx, w, h, pCodecCtx->pix_fmt, dstWidth,
                                                               dstHeight, pixelFormat, /*SWS_BICUBIC*/SWS_LANCZOS,
                                                               nullptr, nullptr, nullptr);
                        if (img_convert_ctx == nullptr) {
                            av_packet_unref(&packet);
                            throw FrameGrabberException("Cannot initialize the conversion context!");
                        }
                    }

                    sws_scale(img_convert_ctx, pFrame->data, pFrame->linesize, 0, pCodecCtx->height,
                              pFrameRGB->data, pFrameRGB->linesize);
                    int64_t display_time = static_cast<int64_t>(static_cast<double>(av_rescale_q(
                        pFrame->pts, pFormatCtx->streams[videoStream]->time_base, rat)) / static_cast<double>(AV_TIME_BASE));
                    //pFrame->pkt_pts/ (double)AV_TIME_BASE;
                    if (display_time <= 0) {
                        display_time = static_cast<int64_t>(static_cast<double>(time) / static_cast<double>(AV_TIME_BASE));
                    }

                    if (display_time < 0) {
                        display_time = 0;
                    }


                    currentFrame_ = std::make_unique<AvcodecVideoFrame>(pFrameRGB, pFrameRGB->data[0], numBytes, display_time,
                                                          pCodecCtx->width, pCodecCtx->height);
                    av_packet_unref(&packet);
                    break;
                }
            }

            // Free the packet that was allocated by av_read_frame
            av_packet_unref(&packet);
            memset(&packet, 0, sizeof(packet));
        }

        return true;
    }

    AbstractVideoFrame* grabCurrentFrame() const {
        return currentFrame_.get();
    }
};

bool AvcodecFrameGrabberPrivate::initialized_ = false;

AvcodecFrameGrabber::AvcodecFrameGrabber(): d_ptr(new AvcodecFrameGrabberPrivate) {

}

AvcodecFrameGrabber::~AvcodecFrameGrabber() {
    delete d_ptr;
}

bool AvcodecFrameGrabber::open(const std::string& fileName) {
    return d_ptr->open(fileName);
}

bool AvcodecFrameGrabber::seek(int64_t time) {
    return d_ptr->seek(time);
}

AbstractVideoFrame* AvcodecFrameGrabber::grabCurrentFrame() {
    return d_ptr->grabCurrentFrame();
}

int64_t AvcodecFrameGrabber::duration() {
    return d_ptr->duration_;
}

std::optional<AbstractFrameGrabber::StreamInfo> AvcodecFrameGrabber::getInfo() {
    if (!d_ptr->streamInfo_) {
        return std::nullopt;
    }
    return *d_ptr->streamInfo_;
}
