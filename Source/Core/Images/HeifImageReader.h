#ifndef IU_CORE_IMAGES_HeifImageReader_H
#define IU_CORE_IMAGES_HeifImageReader_H

#pragma once

#include "AbstractImageReader.h"

struct heif_image;
struct heif_context;

class HeifImageReader : public AbstractImageReader {
public:
    using AbstractImageReader::AbstractImageReader;
    std::unique_ptr<GdiPlusImage> readFromFile(const wchar_t* fileName) override;
    std::unique_ptr<GdiPlusImage> readFromMemory(uint8_t* data, size_t size) override;
    std::unique_ptr<GdiPlusImage> readFromStream(IStream* stream) override;
    std::wstring getLastError() override;
    static bool canRead(const uint8_t* data, size_t size);

private:
    std::wstring lastError_;
    std::unique_ptr<GdiPlusImage> decode(heif_context* context);
    std::unique_ptr<Gdiplus::Bitmap> readHeif(heif_image* img);
};

#endif
