#include "DirectshowVideoFrame.h"

DirectshowVideoFrame::DirectshowVideoFrame(unsigned char *data, size_t dataSize, int64_t time, int width, int height) {
    time_ = time;

    BITMAPFILEHEADER bfh;
    memset(&bfh, 0, sizeof(bfh));
    bfh.bfType = ('M' << 8) | 'B';
    bfh.bfSize = sizeof(bfh) + dataSize + sizeof(BITMAPINFOHEADER);
    bfh.bfOffBits = sizeof(BITMAPINFOHEADER) + sizeof(BITMAPFILEHEADER);

    BITMAPINFOHEADER bih;
    memset(&bih, 0, sizeof(bih));
    bih.biSize = sizeof(bih);
    bih.biWidth = width;
    bih.biHeight = height;
    bih.biPlanes = 1;
    bih.biBitCount = 24;

    BITMAPINFO bi;
    bi.bmiHeader = bih;

    int dataOffset = sizeof(bfh) + sizeof(bih);
    dataSize_ = dataSize + dataOffset;
    data_ = std::make_unique<unsigned char[]>(dataSize + dataOffset);
    memcpy(data_.get(), &bfh, sizeof(bfh));
    memcpy(data_.get() + sizeof(bfh), &bih, sizeof(bih));
    memcpy(data_.get() + dataOffset, data, dataSize);

    width_ = width;
    height_ = height;
}
DirectshowVideoFrame::~DirectshowVideoFrame() = default;

bool DirectshowVideoFrame::saveToFile(const std::string& fileName) const {
    std::unique_ptr<AbstractImage> image(AbstractImage::createImage());
    if (!image) {
        return false;
    }
    if (!image->loadFromRawData(AbstractImage::dfBitmapRgb, width_, height_, data_.get(), dataSize_, nullptr)) {
        return false;
    }
    return image->saveToFile(fileName);
}

std::unique_ptr<AbstractImage> DirectshowVideoFrame::toImage() const {
    std::unique_ptr<AbstractImage> image(AbstractImage::createImage());
    if (!image) {
        return nullptr;
    }
    if (!image->loadFromRawData(AbstractImage::dfBitmapRgb, width_, height_, data_.get(), dataSize_, nullptr)) {
        image.reset();
    }
    return image;
}