
#pragma once
#include <memory>

#include <windows.h>
#include "3rdpart/GdiplusH.h"

class ColorQuantizer {
public:
    static std::unique_ptr<Gdiplus::Bitmap> getQuantized(Gdiplus::Bitmap* pSource, Gdiplus::Color backgroundColor, UINT nMaxColors = 256);
};
