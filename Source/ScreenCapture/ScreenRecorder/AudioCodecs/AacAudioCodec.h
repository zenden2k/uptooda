#pragma once
#include <memory>
#include <string>

#include "AudioCodec.h"
#include "ScreenCapture/ScreenRecorder/ArgsBuilder/FFmpegOutputArgs.h"
#include "ScreenCapture/ScreenRecorder/FFmpegOptions.h"

class AacAudioCodec: public AudioCodec
{
public:
    inline static auto CODEC_ID = "aac";

    AacAudioCodec()
        : AudioCodec("aac", "")
    {
    }

    void apply(const FFmpegOptions& settings, FFmpegOutputArgs& outputArgs) override {
         // bitrate: 32k to 512k (steps of 32k)
         //int b = 32 * (1 + (15 * (settings.quality - 1)) / 99);

        outputArgs.setAudioCodec("aac")
             //.addArg("-strict -2")
             .addArg("b:a", (settings.audioQuality.empty() ? defaultQuality() : settings.audioQuality) + "k");
    }

    IdNameArray qualities() const override {
        IdNameArray res;
        for (int i = 32; i <= 512; i += 32) {
            res.emplace_back( std::to_string(i), std::to_string(i) + " kpbs" );
        }
        return res;
    }

    std::string defaultQuality() const override {
        return "192";
    }

    private:
};
