#ifndef IU_SCREENCAPTURE_SCREENRECORDER_SCREENRECORDER_H
#define IU_SCREENCAPTURE_SCREENRECORDER_SCREENRECORDER_H

#pragma once

#include <atomic>
#include <string>
#include <future>
#include <functional>

#include "atlheaders.h"

#include <boost/signals2.hpp>

class ScreenRecorder
{
public:
    enum class Status
    {
        Invalid,
        Recording,
        Paused,
        RunningConcatenation,
        RunningConversion,
        Finished,
        Canceled,
        Failed
    };
    ScreenRecorder(std::string outFile, CRect rect);
    virtual ~ScreenRecorder() = default;
    virtual void start() = 0;
    virtual void stop() = 0;
    virtual void pause() = 0;
    virtual void cancel() = 0;
    virtual bool isRunning() const;
    virtual void setOffset(int x, int y);
    Status status() const;
    std::string outFileName() const;

    using StatusChangeSignal = boost::signals2::signal<void(Status)>;

    template<typename F>
    boost::signals2::connection addStatusChangeCallback(F&& f) {
        return onStatusChange_.connect(std::forward<F>(f));
    }

protected:
    void changeStatus(Status status);
    CRect captureRect_;
    bool isRunning_ = false;
    std::string outFilePath_;

    std::atomic<Status> status_{ Status::Invalid };
    StatusChangeSignal onStatusChange_;
};


#endif
