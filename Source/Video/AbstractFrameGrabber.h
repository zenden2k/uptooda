#ifndef ABSTRACTFRAMEGRABBER_H
#define ABSTRACTFRAMEGRABBER_H

#include <optional>
#include <string>
#include "AbstractVideoFrame.h"
#include "Core/Utils/CoreTypes.h"

class AbstractFrameGrabber
{
public:
    struct StreamInfo {
        int width = 0;
        int height = 0;
        std::string codecName;
    };
    AbstractFrameGrabber() = default;
    virtual ~AbstractFrameGrabber() = default;

    /**
     * @throws FrameGrabberException
     */
    virtual bool open(const std::string& fileName)=0;

    /**
     * @throws FrameGrabberException
     */
    virtual bool seek(int64_t time)=0;

    virtual AbstractVideoFrame* grabCurrentFrame()=0;
    virtual int64_t duration()=0;
    std::string error() const;
    virtual void abort();
    virtual std::optional<StreamInfo> getInfo();

protected:
    std::string error_;
private:
    DISALLOW_COPY_AND_ASSIGN(AbstractFrameGrabber);
};

#endif // ABSTRACTFRAMEGRABBER_H
