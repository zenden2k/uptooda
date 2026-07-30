#pragma once

#include <string>
#include <vector>

#include "../FFmpegCodec.h"

class FFmpegOutputArgs;
class FFmpegOptions;
class FFmpegAudioArgsProvider;

class FFmpegVideoCodec: public FFmpegCodec
{
public:
    FFmpegVideoCodec(std::string name, std::string extension, std::string description, bool canUseBitrate = true, bool canUseQuality = true)
        : FFmpegCodec(std::move(name), std::move(description), canUseBitrate, canUseQuality),
        extension_(std::move(extension))
    {
    }

private:
    std::string extension_;

};
