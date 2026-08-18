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

#include "VideoGrabber.h"

#include <atomic>
#include <boost/format.hpp>

#include "AbstractFrameGrabber.h"
#ifdef _WIN32
#include "DirectshowFrameGrabber.h"
#include "DirectshowFrameGrabber2.h"
#include "MediaFoundationFrameGrabber.h"
#endif
#include "AbstractVideoFrame.h"
#ifdef IU_ENABLE_FFMPEG
    #include "AvcodecFrameGrabber.h"
#endif
#include "Core/Utils/CoreUtils.h"
#include "Core/Logging.h"
#include "Core/i18n/Translator.h"

class VideoGrabberRunnable {
public:
    explicit VideoGrabberRunnable(VideoGrabber* videoGrabber, bool logErrors)
    {
        videoGrabber_ = videoGrabber;
        canceled_ = false;
        isRunning_ = false;
        logErrors_ = logErrors;
    }

    virtual ~VideoGrabberRunnable() = default;

    void cancel()
    {
        canceled_ = true;
    }

    virtual void run()
    {
        isRunning_ = true;
        if ( !IuCoreUtils::FileExists(videoGrabber_->fileName_) ) {
            LOG(ERROR) << (boost::format(_("File \"%s\" not found!")) % videoGrabber_->fileName_);
            if ( videoGrabber_->onFinished_ ) {
                videoGrabber_->onFinished_(false);
            }
            isRunning_ = false;
            return;
        }
        std::unique_ptr<AbstractFrameGrabber> grabber;

        try {
            grabber = videoGrabber_->createGrabber();

            if (!grabber->open(videoGrabber_->fileName_)) {
                isRunning_ = false;
                throw std::runtime_error(str(boost::format(_("Failed to open video file '%s'.")) % videoGrabber_->fileName_));
            }
        } catch (const std::exception& ex) {
            std::string s = ex.what();
            (logErrors_ ? LOG(ERROR) : LOG(WARNING)) << "File: " << videoGrabber_->fileName_ << std::endl
                                   << ex.what();

            if ( videoGrabber_->onFinished_ ) {
                videoGrabber_->onFinished_(false);
            }
            isRunning_ = false;
            return;
        }

        videoGrabber_->streamInfo_ = grabber->getInfo();

        int64_t duration = grabber->duration();
        int64_t step = duration / ( videoGrabber_->frameCount_ + 1 );
        int successFrameCount = 0;
        for( int i = 0; i < videoGrabber_->frameCount_; i++ ) {
            if ( canceled_) {
                break;
            }
            auto curTime = static_cast<int64_t>(( i + 0.5 ) * static_cast<double>(step));
            AbstractVideoFrame* frame = nullptr;
            try {
                grabber->seek(curTime);
                frame = grabber->grabCurrentFrame();
                if (!frame) {
                    grabber->seek(curTime);
                    frame = grabber->grabCurrentFrame();
                }
            } catch (const std::exception& ex) {
                LOG(WARNING) << "File: " << videoGrabber_->fileName_ << std::endl <<ex.what();
            }
            if (!frame) {
                LOG(WARNING) << "File: " << videoGrabber_->fileName_ << std::endl << "grabber->grabCurrentFrame returned NULL";
                continue;
            }
            int64_t sampleTime = frame->getTime();
            int hours = static_cast<int>(sampleTime / 3600);
            int minutes = static_cast<int>(sampleTime / 60 % 60);
            int seconds = static_cast<int>(sampleTime % 60);
            std::string s = str(boost::format("%02d:%02d:%02d") % hours % minutes % seconds);

            if ( /*frame && */videoGrabber_->onFrameGrabbed_ ) {
                videoGrabber_->onFrameGrabbed_(s, sampleTime, frame->toImage());
                successFrameCount++;
            }
        }
        grabber.reset();
        if ( videoGrabber_->onFinished_ ) {
            videoGrabber_->onFinished_(successFrameCount != 0);
        }
        isRunning_ = false;
    }

    bool isRunning() const
    {
        return isRunning_;
    }

protected:
    VideoGrabber* videoGrabber_;
    std::atomic<bool> canceled_;
    std::atomic<bool> isRunning_;
    bool logErrors_;
};

VideoGrabber::VideoGrabber(bool async, bool logErrors)
    : async_(async)
    , logErrors_(logErrors)
{
    videoEngine_ = veAuto;
    frameCount_ = 5;
}

VideoGrabber::~VideoGrabber()= default;

void VideoGrabber::grab(const std::string& fileName) {
     if ( !IuCoreUtils::FileExists(fileName) ) {
         return;
     }
     //std::string ext = IuCoreUtils::ExtractFileExt(fileName);
     fileName_ = fileName;
     worker_ = std::make_unique<VideoGrabberRunnable>(this, logErrors_);
     if (async_) {
         std::thread t1(&VideoGrabberRunnable::run, worker_.get());
         t1.detach();
     } else {
         worker_->run();
     }
 }

void VideoGrabber::abort() {
    worker_->cancel();
}

bool VideoGrabber::isRunning() const {
    return worker_->isRunning();
}

void VideoGrabber::setVideoEngine(VideoEngine engine) {
    videoEngine_ = engine;
}

VideoGrabber::VideoEngine VideoGrabber::videoEngine() const {
    return videoEngine_;
}

void VideoGrabber::setFrameCount(int frameCount) {
    frameCount_ = frameCount;
}

void VideoGrabber::setOnFrameGrabbed(FrameGrabbedCallback cb) {
    onFrameGrabbed_ = std::move(cb);
}

void VideoGrabber::setOnFinished(FinishCallback cb) {
    onFinished_ = std::move(cb);
}

std::optional<AbstractFrameGrabber::StreamInfo> VideoGrabber::getInfo() const {
    return streamInfo_;
}

std::unique_ptr<AbstractFrameGrabber> VideoGrabber::createGrabber() {
#ifdef _WIN32
    #ifdef IU_ENABLE_FFMPEG
    if ( videoEngine_ == veAvcodec ) {
        return std::make_unique<AvcodecFrameGrabber>();
    } else
    #endif
    if (videoEngine_ == veDirectShow2) {
        return std::make_unique<DirectshowFrameGrabber2>();
    } else if (videoEngine_ == veMediaFoundation) {
        return std::make_unique<MediaFoundationFrameGrabber>();
    }
#elif defined(IU_ENABLE_FFMPEG)
    return std::make_unique<AvcodecFrameGrabber>();
#endif
    throw std::runtime_error("Cannot create video grabber");
}
