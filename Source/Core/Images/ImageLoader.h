#ifndef IU_CORE_IMAGES_IMAGELOADER_H
#define IU_CORE_IMAGES_IMAGELOADER_H

#pragma once

#include <memory>
#include <string>
#include <unordered_set>

#include "3rdpart/GdiPlusH.h"
#include "Core/Images/GdiPlusImage.h"

class AbstractImageReader;

enum class ImageFormat { Unknown, WebP, Heif };

// Controls how many frames of an animation are decoded and retained.
enum class ImageFrameMode { FIRST_FRAME, ALL_FRAMES };

class ImageLoader {
public:
    std::unique_ptr<GdiPlusImage> loadFromFile(const wchar_t* fileName,
                                               ImageFrameMode frameMode = ImageFrameMode::FIRST_FRAME);
    std::unique_ptr<GdiPlusImage> loadFromMemory(uint8_t* data, size_t size,
                                                 ImageFrameMode frameMode = ImageFrameMode::FIRST_FRAME);
    std::unique_ptr<GdiPlusImage> loadFromResource(HINSTANCE hInstance, LPCTSTR szResName, LPCTSTR szResType,
                                                   ImageFrameMode frameMode = ImageFrameMode::FIRST_FRAME);
    std::wstring getLastError() const;
    static ImageFormat getImageFormatFromData(uint8_t* data, size_t size);
    static std::unique_ptr<AbstractImageReader>
    createReaderForFormat(ImageFormat format, ImageFrameMode frameMode = ImageFrameMode::FIRST_FRAME);
    static std::unordered_set<std::string> SupportedImageExtensions;
    static bool IsImage(LPCTSTR szFileName);
    static bool IsImage(const std::string& fileName);

protected:
    std::wstring lastError_;
};
#endif
