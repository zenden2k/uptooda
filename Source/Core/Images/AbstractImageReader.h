#ifndef IU_CORE_IMAGES_IMAGEREADER_H
#define IU_CORE_IMAGES_IMAGEREADER_H

#pragma once

#include <memory>
#include <string>

#include "3rdpart/GdiPlusH.h"
#include "Core/Images/ImageLoader.h"


class AbstractImageReader {
public:
    explicit AbstractImageReader(ImageFrameMode frameMode = ImageFrameMode::FIRST_FRAME) : frameMode_(frameMode) {
    }
    virtual ~AbstractImageReader() = default;
    virtual std::unique_ptr<GdiPlusImage> readFromFile(const wchar_t* fileName) = 0;
    virtual std::unique_ptr<GdiPlusImage> readFromMemory(uint8_t* data, size_t size) = 0;
    virtual std::unique_ptr<GdiPlusImage> readFromStream(IStream* stream) = 0;
    virtual std::wstring getLastError() = 0;

protected:
    ImageFrameMode frameMode_;
};

#endif
