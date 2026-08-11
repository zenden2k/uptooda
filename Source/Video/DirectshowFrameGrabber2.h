#ifndef DIRECTSHOWFRAMEGRABBER2_H
#define DIRECTSHOWFRAMEGRABBER2_H

#include "AbstractFrameGrabber.h"

class DirectshowFrameGrabber2Private;

class DirectshowFrameGrabber2 : public AbstractFrameGrabber
{
public:
    DirectshowFrameGrabber2();
    ~DirectshowFrameGrabber2() override;
    bool open(const std::string& fileName) override;
    bool seek(int64_t time) override;
    AbstractVideoFrame* grabCurrentFrame() override;
    int64_t duration() override;
    void abort() override;
protected:
    int64_t duration_;
    std::unique_ptr<DirectshowFrameGrabber2Private> d_ptr;
    DISALLOW_COPY_AND_ASSIGN(DirectshowFrameGrabber2);
};

#endif 
