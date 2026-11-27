#pragma once

#include <boost/format.hpp>

#include "FFmpegSource.h"
#include "../ArgsBuilder/FFmpegInputArgs.h"
#include "../FFmpegOptions.h"

class DDAGrabSource: public FFmpegSource {
public:
    static constexpr auto SOURCE_ID = "ddagrab";

    DDAGrabSource(): FFmpegSource("ddagrab")
    {
    }

    void apply(const FFmpegOptions& settings, FFmpegInputArgs& inputArgs, GlobalFFmpegArgs& globalArgs) override {
        bool hwEncoder = settings.codec.find("_nvenc") != std::string::npos;

        if (hwEncoder || settings.hwAdapterIndex) {
            std::string arg = "d3d11va";
            if (settings.hwAdapterIndex) {
                arg += ":";
                arg += std::to_string(*settings.hwAdapterIndex);
            }
            inputArgs.addArg("init_hw_device", arg);
        }

        const std::string dimensions = (settings.width == 0 || settings.height == 0) ? "" :
            str(boost::format("video_size=%dx%d:offset_x=%d:offset_y=%d:") % settings.width % settings.height % settings.offsetX % settings.offsetY);

        std::string filterComplex = boost::str(boost::format("ddagrab=%sframerate=%d:draw_mouse=%d:output_idx=%d") % dimensions % settings.framerate
            % settings.showCursor % std::max(0, settings.outputIdx));

        if (!hwEncoder) {
            filterComplex += ",hwdownload,format=bgra";
        }

        inputArgs.addArg("filter_complex", filterComplex);
            std::vector<std::string> args = {
                   // "-hide_banner",
                    "init_hw_device", "d3d11va",
                    //"-framerate","60",
                    /*"-offset_x",std::to_string(captureRect_.left),
                    "-offset_y",std::to_string(captureRect_.top),
                    "-video_size",std::to_string(captureRect_.Width())+"x" + std::to_string(captureRect_.Height()),*/
                   
                    /*,video_size = "+ std::to_string(captureRect_.Width()) + "x" + std::to_string(captureRect_.Height())
                    + ",offset_x=" + std::to_string(captureRect_.left),
                    + ",offset_y=" + std::to_string(captureRect_.top),**/

                /*-c:v", "h264_nvenc",
                    "-cq:v", "20", outFilePath_*/
        };
    }
}; 
