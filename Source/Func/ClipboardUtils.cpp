#include "ClipboardUtils.h"

#include <vector>

#include <strsafe.h>

#include "IuCommonFunctions.h"
#include "Core/Images/Utils.h"
#include "Core/Logging.h"

namespace ClipboardUtils {

bool CopyFileAndImageToClipboard(LPCTSTR fileName, HWND hwnd) {
    if (IuCommonFunctions::IsImage(fileName) ) {
        CopyImageToClipboard(fileName, hwnd);
    }
    std::vector<CString> fileNames;
    fileNames.emplace_back(fileName);
    return CopyFilesToClipboard(fileNames, hwnd, false);
}

bool CopyFilesToClipboard(const std::vector<CString>& fileNames, HWND hwnd, bool clearClipboard) {
    size_t argc = fileNames.size();
    std::unique_ptr<WCHAR[]> pFullNames = std::make_unique<WCHAR[]>(argc * MAX_PATH + 1);
    WCHAR *p = pFullNames.get();
    for (size_t i = 0; i < argc; i++ ) {
        LPTSTR end = nullptr;
        StringCchCopyEx(p, MAX_PATH, fileNames[i], &end, nullptr, 0);
        if (!end) {
            break;
        }
        p = end + 1;
    }
    *p++ = 0; 
    DWORD dwDataBytes = sizeof(WCHAR) * (p - pFullNames.get());
    DROPFILES df = {sizeof(DROPFILES), {0, 0}, 0, TRUE};
    HGLOBAL hMem = GlobalAlloc(GMEM_ZEROINIT|GMEM_MOVEABLE|GMEM_DDESHARE, sizeof(DROPFILES) + dwDataBytes);
    if (!hMem) {
        return false;
    }
    char *pGlobal = static_cast<char*>(GlobalLock(hMem));
    if (!pGlobal) {
        GlobalFree(hMem);
        return false;
    }
    CopyMemory(pGlobal, &df, sizeof(DROPFILES));
    CopyMemory(pGlobal + sizeof(DROPFILES), pFullNames.get(), dwDataBytes); // that's pGlobal + 20 bytes (the size of DROPFILES);
    GlobalUnlock(hMem);
    if ( OpenClipboard(hwnd) ) {
        if ( clearClipboard ) {
            EmptyClipboard();
        }
        SetClipboardData(CF_HDROP, hMem);
        CloseClipboard();
    }

    return true;
}

bool CopyImageToClipboard(LPCTSTR fileName, HWND hwnd) {
    std::unique_ptr<GdiPlusImage> img = ImageUtils::LoadImageFromFileExtended(fileName);
    if (!img) {
        LOG(ERROR) << "Unable to load image:" << std::endl << fileName;
        return false;
    }
    Gdiplus::Bitmap* src = img->getBitmap();
    if (!src || src->GetLastStatus() != Gdiplus::Ok) {
        LOG(ERROR) << "Unable to load image:" << std::endl << fileName;
        return false;
    }

    return CopyBitmapToClipboard(src, hwnd);
}

bool CopyBitmapToClipboard(Gdiplus::Bitmap* image, HWND hwnd, bool preserveAlpha) {
    if (OpenClipboard(hwnd)) {
        EmptyClipboard();
        UINT pngFormat = RegisterClipboardFormat(_T("PNG"));
        if (pngFormat) {
            CComPtr<IStream> pStream;
            HRESULT hr = CreateStreamOnHGlobal(nullptr, TRUE, &pStream);
            if (FAILED(hr)) {
                LOG(ERROR) << "Failed to create memory stream. HRESULT=" << hr;
            } else if (ImageUtils::SaveImageToFile(image, CString(), pStream, ImageUtils::sifPNG, 85)) {
                HGLOBAL hGlobal = nullptr;
                hr = GetHGlobalFromStream(pStream, &hGlobal);
                if (FAILED(hr)) {
                    LOG(ERROR) << "Failed to get HGLOBAL from stream. HRESULT=" << hr;
                } else {
                    SIZE_T dataSize = GlobalSize(hGlobal);
                    if (dataSize != 0) {
                        HGLOBAL hClipboardData = GlobalAlloc(GMEM_MOVEABLE, dataSize);
                        if (!hClipboardData) {
                            LOG(ERROR) << "Failed to allocate clipboard memory. ErrorCode=" << GetLastError();
                        } else {
                            void* pSource = GlobalLock(hGlobal);
                            void* pDest = GlobalLock(hClipboardData);
                            if (pSource && pDest) {
                                memcpy(pDest, pSource, dataSize);
                            }
                            GlobalUnlock(hGlobal);
                            GlobalUnlock(hClipboardData);

                            if (!pSource || !pDest) {
                                LOG(ERROR) << "Failed to lock memory for copying";
                                GlobalFree(hClipboardData);
                            } else {
                                HANDLE hResult = SetClipboardData(pngFormat, hClipboardData);
                                if (!hResult) {
                                    LOG(ERROR) << "Failed to set clipboard data. ErrorCode=" << GetLastError();
                                    GlobalFree(hClipboardData);
                                }
                            }
                        }
                    }
                }
            }
        }

        if (!preserveAlpha) {
            ImageUtils::RemoveAlphaFromBitmap(*image, Gdiplus::Color(255, 255, 255, 255));
        }
        HBITMAP out = nullptr;
        CClientDC dc(hwnd);
        image->GetHBITMAP(Gdiplus::Color(255, 255, 255, 255), &out);
        CDC origDC, destDC;
        origDC.CreateCompatibleDC(dc);
        CBitmap destBmp;
        destBmp.CreateCompatibleBitmap(dc, image->GetWidth(), image->GetHeight());
        HBITMAP oldOrigBmp = origDC.SelectBitmap(out);
        destDC.CreateCompatibleDC(dc);
        HBITMAP oldDestBmp = destDC.SelectBitmap(destBmp);
        destDC.BitBlt(0, 0, image->GetWidth(), image->GetHeight(), origDC, 0, 0, SRCCOPY);
        destDC.SelectBitmap(oldDestBmp);
        origDC.SelectBitmap(oldOrigBmp);
        SetClipboardData(CF_BITMAP, destBmp);
        CloseClipboard();
        DeleteObject(out);
        return true;
    }
    return false;
}

}
