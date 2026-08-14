#pragma once

#include <memory>
//#include <boost/shared_ptr.hpp>
//#include <boost/enable_shared_from_this.hpp>
#include <boost/asio.hpp>
#include <boost/process/v2/process.hpp>

#include "ScreenRecorder.h"
#include "FFmpegOptions.h"
#include "FFMpegOptionsManager.h"


class TaskDispatcher;

class FFmpegScreenRecorder : public ScreenRecorder,
                             public std::enable_shared_from_this<FFmpegScreenRecorder>
{
public:
    FFmpegScreenRecorder(std::string ffmpegPath, std::string outFile, CRect rect, FFmpegOptions options);
    ~FFmpegScreenRecorder() override;
    void start() override;
    void stop() override;
    void pause() override;
    void cancel() override;
    bool isRunning() const override;
private:
    std::string ffmpegPath_;
    //std::thread thread_;
    std::shared_ptr<boost::asio::writable_pipe> inStream_;
    std::string fileNoExt_;
    std::string currentOutFilePath_;
    std::vector<std::string> parts_;
    std::shared_future<int> future_;
    void sendStopSignal();
    std::future<int> launchFFmpeg(const std::vector<std::string>& args, const std::function<void(int)>& onFinish);
    void cleanupAfter();
    const std::string stopData_ = "q\n";
    FFmpegOptions options_;
    FFMpegOptionsManager optionsManager_;
};
