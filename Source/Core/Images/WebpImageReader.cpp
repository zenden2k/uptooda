#include "WebpImageReader.h"

#include <webp/decode.h>
#include <webp/demux.h>

#include <boost/format.hpp>

#include "Core/Logging.h"
#include "Core/Images/GdiPlusImage.h"
#include "Utils.h"

struct WebPPic {
    int width;
    int height;
    uint8_t* rgba;
    bool animated;
};

using namespace Gdiplus;

std::unique_ptr<GdiPlusImage> WebpImageReader::readFromFile(const wchar_t* fileName) {
    try {
        auto [data, dataSize] = ImageUtils::ExUtilReadFile(fileName);
        if (!data) {
            return {};
        }
        return readFromMemory(data.get(), dataSize);
    } catch (const std::exception&) {
        return {};
    }
}

std::unique_ptr<GdiPlusImage> WebpImageReader::readFromMemory(uint8_t* data, size_t size) {
    WebPPic pic = {};
    if (!readWebP(data, size, &pic)) {
        return nullptr;
    }

    auto* bm = new Gdiplus::Bitmap(pic.width, pic.height, PixelFormat32bppARGB);
    std::unique_ptr<GdiPlusImage> img = std::make_unique<GdiPlusImage>(bm);
    //std::unique_ptr<Gdiplus::Bitmap> bm = std::make_unique<Gdiplus::Bitmap>(pic.width, pic.height, PixelFormat32bppARGB);
    BitmapData dstData;
    Rect rc(0, 0, pic.width, pic.height);

    if (bm->LockBits(&rc, ImageLockModeWrite, PixelFormat32bppARGB, &dstData) == Ok) {
        auto dstBits = static_cast<uint8_t *>(dstData.Scan0);
        memcpy(dstBits, pic.rgba, pic.height * pic.width * 4);

        bm->UnlockBits(&dstData);
    }
    if (pic.animated) {
        delete[] pic.rgba;
    }
    else {
        WebPFree(pic.rgba);
    }
    img->setSrcMultiFrame(pic.animated);
    img->setSrcFormat("webp");
    return img;
}

std::unique_ptr<GdiPlusImage> WebpImageReader::readFromStream(IStream* stream) {
    LARGE_INTEGER li;
    li.QuadPart = 0;

    if (FAILED(stream->Seek(li, STREAM_SEEK_SET, nullptr))) {
        return nullptr;
    }

    STATSTG stats;
    if (FAILED(stream->Stat(&stats, STATFLAG_NONAME))) {
        return nullptr;
    }
    ULONGLONG sSize = stats.cbSize.QuadPart;
    ULONG bytesRead;
    std::unique_ptr<uint8_t[]> pBuffer;
    try {
        pBuffer = std::make_unique<uint8_t[]>(sSize);
    }
    catch (std::exception &) {
        lastError_ = str(boost::wformat(L"Unable to allocate %d bytes") % sSize);
        return nullptr;
    }
   
    if (FAILED(stream->Read(pBuffer.get(), static_cast<ULONG>(sSize), &bytesRead))) {
        lastError_ = L"Failed to read from IStream";
        return nullptr;
    }

    return readFromMemory(pBuffer.get(), sSize);
}

bool WebpImageReader::readWebP(const uint8_t* const data, size_t data_size, WebPPic* pic) {
    VP8StatusCode status = VP8_STATUS_OK;
    WebPDecoderConfig config;
    WebPDecBuffer* const output_buffer = &config.output;
    WebPBitstreamFeatures* const bitstream = &config.input;

    if (!WebPInitDecoderConfig(&config)) {
        LOG(ERROR) << "Library version mismatch!\n";
        return false;
    }

    status = WebPGetFeatures(data, data_size, bitstream);
    if (status != VP8_STATUS_OK) {
        LOG(WARNING) << "Error loading input webp data, status code=" << status;
        return false;
    }
    pic->animated = !!bitstream->has_animation;

    if (bitstream->has_animation) {
        WebPData webp_data;
        WebPDataInit(&webp_data);
        webp_data.bytes = data;
        webp_data.size = data_size;
        WebPAnimInfo anim_info;
        WebPAnimDecoderOptions options;
        memset(&options, 0, sizeof(options));
        options.color_mode = MODE_BGRA;

        WebPAnimDecoder* dec = WebPAnimDecoderNew(&webp_data, &options);
        if (dec == nullptr) {
            LOG(WARNING) << "Error parsing webp image";
            return false;
        }

        if (!WebPAnimDecoderGetInfo(dec, &anim_info)) {
            WebPAnimDecoderDelete(dec);
            LOG(WARNING) << "Error getting global info about the animation";
            return false;
        }

        pic->width = anim_info.canvas_width;
        pic->height = anim_info.canvas_height;

        // Decode just first frame
        if (WebPAnimDecoderHasMoreFrames(dec)) {
            uint8_t* frame_rgba;
            int timestamp;

            if (!WebPAnimDecoderGetNext(dec, &frame_rgba, &timestamp)) {
                WebPAnimDecoderDelete(dec);
                return false;
            }
            unsigned int frameSize = anim_info.canvas_width*anim_info.canvas_height * 4;
            pic->rgba = new uint8_t[frameSize];
            memcpy(pic->rgba, frame_rgba, frameSize);
        }
        WebPAnimDecoderDelete(dec);

    }
    else {
        //output_buffer->colorspace = has_alpha ? MODE_RGBA : MODE_RGB;

        pic->rgba = WebPDecodeBGRA(data, data_size, &pic->width, &pic->height);
    }
    if (!pic->rgba) {
        return false;
    }
    /*if (status == VP8_STATUS_OK) {
          /*pic->rgba = output_buffer->u.RGBA.rgba;
          pic->stride = output_buffer->u.RGBA.stride;
          pic->width = output_buffer->width;
          pic->height = output_buffer->height;*/


          /*ok = has_alpha ? WebPPictureImportRGBA(pic, rgba, stride)
                    : WebPPictureImportRGB(pic, rgba, stride);*
            }*/


    WebPFreeDecBuffer(output_buffer);
    return true;
}

std::wstring WebpImageReader::getLastError() {
    return lastError_;
}
