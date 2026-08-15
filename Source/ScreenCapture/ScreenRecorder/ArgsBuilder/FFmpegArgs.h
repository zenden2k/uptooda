
#pragma once

#include <vector>
#include <string>
#include <sstream>

class FFmpegArgsBase {
public:
    virtual ~FFmpegArgsBase() = default;
    virtual std::vector<std::string> getArgs() {
        return args_;
    }
protected:
    std::vector<std::string> args_;
};

template<typename T> class FFmpegArgs: public FFmpegArgsBase
{
public:
    T& addArg(const std::string& arg) {
        args_.push_back(arg);
        return static_cast<T&>(*this);
    }

    template<typename U> T& addArg(const std::string& key, const std::string& value)
    {
        args_.push_back("-" + key);
        args_.push_back(value);
        return static_cast<T&>(*this);
    }

    template<typename U> T& addArg(const std::string& key, const U& value)
    {
        args_.push_back("-" + key);
        std::ostringstream os;
        os << value;

        args_.push_back(os.str());
        return static_cast<T&>(*this);
    }
};

class GlobalFFmpegArgs : public FFmpegArgs<GlobalFFmpegArgs> {

};