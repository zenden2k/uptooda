#pragma once

#include <memory>
#include <string>

#include "FFmpegInputArgs.h"
#include "FFmpegOutputArgs.h"

class FFMpegArgsBuilder {
public:
    GlobalFFmpegArgs& globalArgs() {
        return globalArgs_;
    }

    FFmpegInputArgs& addInputFile(const std::string& fileName) {
        auto input = std::make_unique<FFmpegInputArgs>(fileName);

        inputs_.push_back(std::move(input));

        return *inputs_.back();
    }

    FFmpegInputArgs& addStdIn() {
        auto input = std::make_unique<FFmpegInputArgs>("-");

        inputs_.push_back(std::move(input));

        return *inputs_.back();
    }

    /*FFmpegInputArgs& AddInputPipe(string NamedPipe)
    {
        var input = new FFmpegInputArgs($"{PipePrefix}{NamedPipe}");

        _inputs.Add(input);

        return input;
    }*/

    FFmpegOutputArgs& addOutputFile(const std::string& fileName) {
        auto output = std::make_unique<FFmpegOutputArgs>(fileName);

        outputs_.push_back(std::move(output));

        return *outputs_.back();
    }

    FFmpegOutputArgs& addStdOut() {
        auto output = std::make_unique<FFmpegOutputArgs>("-");

        outputs_.push_back(std::move(output));

        return *outputs_.back();
    }

    /*public FFmpegOutputArgs AddOutputPipe(string NamedPipe)
    {
        var output = new FFmpegOutputArgs($"{PipePrefix}{NamedPipe}");

        _outputs.Add(output);

        return output;
    }*/

    std::vector<std::string> getArgs() {
        std::vector<std::string> res;

        auto gl = globalArgs_.getArgs();
        res.insert(res.end(), gl.begin(), gl.end());

        for (const auto& input : inputs_) {
            auto r = input->getArgs();
            res.insert(res.end(), r.begin(), r.end());

        }

        for (const auto& output : outputs_) {
            auto r = output->getArgs();
            res.insert(res.end(), r.begin(), r.end());
        }

        return res;
    }

private:
    GlobalFFmpegArgs globalArgs_;
    std::vector<std::unique_ptr<FFmpegInputArgs>> inputs_;
    std::vector<std::unique_ptr<FFmpegOutputArgs>> outputs_;
};
