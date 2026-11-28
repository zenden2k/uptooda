#ifndef GDIPLUSIMAGE_H
#define GDIPLUSIMAGE_H

#include <memory>
#include <vector>

#include "3rdpart/GdiPlusH.h"
#include "AbstractImage.h"

class GdiPlusImage : public AbstractImage {
public:
    GdiPlusImage();
    explicit GdiPlusImage(Gdiplus::Image* bm, bool takeOwnership = true);
    ~GdiPlusImage() override;
    bool loadFromFile(const std::string& fileName) override;
    bool saveToFile(const std::string& fileName) const override;
    bool isNull() const override;
    bool loadFromRawData(DataFormat dt, int width, int height, uint8_t* data, size_t dataSize,
                         void* parameter) override;
    bool loadFromRgb(int width, int height, uint8_t* data, size_t dataSize);
    Gdiplus::Bitmap* getBitmap() const;
    Gdiplus::Bitmap* releaseBitmap();
    Gdiplus::Image* releaseImage();

    int getWidth() const override;
    int getHeight() const override;
    void setSrcMultiFrame(bool animated);
    bool isSrcMultiFrame() const;
    void appendFrame(std::unique_ptr<Gdiplus::Bitmap> bitmap, uint64_t durationMs);
    size_t frameCount() const;
    Gdiplus::Bitmap* frameBitmap(size_t index) const;
    uint64_t frameDurationMs(size_t index) const;

protected:
    Gdiplus::Image* bm_;
    std::unique_ptr<uint8_t[]> data_;
    bool takeOwnership_;
    bool isSrcMultiFrame_;
    void init();
    DISALLOW_COPY_AND_ASSIGN(GdiPlusImage);

private:
    struct Frame {
        std::unique_ptr<Gdiplus::Bitmap> Bitmap;
        uint64_t DurationMs;
    };
    uint64_t firstFrameDurationMs_ = 0;
    std::vector<Frame> additionalFrames_;
};

#endif
