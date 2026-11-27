#pragma once

#include "FFmpegSource.h"
#include "../ArgsBuilder/FFmpegInputArgs.h"
#include "../FFmpegOptions.h"

class DirectShowSource: public FFmpegSource {
public:
    static constexpr auto SOURCE_ID = "directshow";

    DirectShowSource(std::string deviceId)
        : FFmpegSource("directshow", true)
        ,deviceId_(deviceId) {
    }

    void apply(const FFmpegOptions& settings, FFmpegInputArgs& inputArgs, GlobalFFmpegArgs& globalArgs) override {
        if (inputArgs.isSourceApplied(SOURCE_ID)) {
            return;
        }
        std::string input;
       
        if (!settings.source.empty()) {
            const auto [id, deviceId] = parseSourceId(settings.source);
            if (!deviceId.empty()) {
                input = "video=" + deviceId + "";
            }
        }

        bool hasAudio = false;

        if (!settings.audioSource.empty()) {
            const auto [id, deviceId] = parseSourceId(settings.audioSource);
            if (!deviceId.empty()) {
                if (!input.empty()) {
                    input += ":";
                }
                input += "audio=" + deviceId;
                hasAudio = true;
            }
        }

        if (input.empty()) {
            return;
        }
        inputArgs.addArg("f", "dshow");
           
        if (hasAudio) {
            inputArgs.addArg("audio_buffer_size", 80);
        }

        inputArgs.addArg("i", input);
        inputArgs.setSourceApplied(SOURCE_ID);
    }

private:
    std::string deviceId_;
}; 
