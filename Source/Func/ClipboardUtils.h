#ifndef FUNC_SYSTEM_UTILS_H
#define FUNC_SYSTEM_UTILS_H

#include <vector>
#include "atlheaders.h"
#include "3rdpart/GdiPlusH.h"

namespace ClipboardUtils {

bool CopyFilesToClipboard(const std::vector<CString>& fileNames, HWND hwnd, bool clearClipboard = true);
bool CopyFileAndImageToClipboard(LPCTSTR fileName, HWND hwnd);
bool CopyImageToClipboard(LPCTSTR fileName, HWND hwnd);
bool CopyBitmapToClipboard(Gdiplus::Bitmap* image, HWND hwnd, bool preserveAlpha = true);

};

#endif;
