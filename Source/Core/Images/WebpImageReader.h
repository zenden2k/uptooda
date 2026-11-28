#ifndef IU_CORE_IMAGES_WEBPIMAGEREADER_H
#define IU_CORE_IMAGES_WEBPIMAGEREADER_H

#pragma once

#include "AbstractImageReader.h"

class WebpImageReader : public AbstractImageReader {
public:
    using AbstractImageReader::AbstractImageReader;
    std::unique_ptr<GdiPlusImage> readFromFile(const wchar_t* fileName) override;
    std::unique_ptr<GdiPlusImage> readFromMemory(uint8_t* data, size_t size) override;
    std::unique_ptr<GdiPlusImage> readFromStream(IStream* stream) override;
    std::wstring getLastError() override;

private:
    std::wstring lastError_;
    std::unique_ptr<GdiPlusImage> readAnimation(const uint8_t* data, size_t size);
};

#endif
