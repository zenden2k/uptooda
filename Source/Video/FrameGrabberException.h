
#pragma once

#include <stdexcept>
#include <string>

class FrameGrabberException : virtual public std::runtime_error {
public:
    explicit
        FrameGrabberException(const std::string& msg) :
        std::runtime_error(msg)
    {
    }

    ~FrameGrabberException() throw () override {}
};
