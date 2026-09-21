#ifndef IU_CORE_VIDEO_DIRECTSHOWVIDEOFRAME_H
#define IU_CORE_VIDEO_DIRECTSHOWVIDEOFRAME_H

#include <windows.h>

#include "AbstractVideoFrame.h"

class DirectshowVideoFrame : public AbstractVideoFrame {
public:
    DirectshowVideoFrame(unsigned char *data, size_t dataSize, int64_t time, int width, int height);
    ~DirectshowVideoFrame() override;
    bool saveToFile(const std::string& fileName) const override;
    std::unique_ptr<AbstractImage> toImage() const override;

protected:
    unsigned char *data_;
    size_t dataSize_;
    DISALLOW_COPY_AND_ASSIGN(DirectshowVideoFrame);
};

#endif