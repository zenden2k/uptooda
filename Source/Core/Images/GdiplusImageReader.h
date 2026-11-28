#ifndef IU_CORE_IMAGES_GDIPLUSIMAGEREADER_H
#define IU_CORE_IMAGES_GDIPLUSIMAGEREADER_H

#pragma once

#include "AbstractImageReader.h"

class GdiplusImageReader : public AbstractImageReader {
public:
    using AbstractImageReader::AbstractImageReader;
    std::unique_ptr<GdiPlusImage> readFromFile(const wchar_t* fileName) override;
    std::unique_ptr<GdiPlusImage> readFromMemory(uint8_t* data, size_t size) override;
    std::unique_ptr<GdiPlusImage> readFromStream(IStream* stream) override;
    std::wstring getLastError() override;

private:
    std::wstring lastError_;
    bool checkLastStatus(Gdiplus::Bitmap* bm);
    std::unique_ptr<GdiPlusImage> finishLoad(std::unique_ptr<Gdiplus::Bitmap> bitmap);
};

#endif
