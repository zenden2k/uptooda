#include "ImageLoader.h"

#include <Shlwapi.h>
#include <boost/format.hpp>

#include "Core/Upload/CommonTypes.h"
#include "WebpImageReader.h"
#include "GdiplusImageReader.h"
#include "HeifImageReader.h"
#include "Core/Utils/StringUtils.h"
#include "Func/WinUtils.h"

std::unordered_set<std::string> ImageLoader::SupportedImageExtensions = {
    "jpg", "jpeg", "jpe", "jif", "jfif", "png", "bmp", "gif", "tif", "tiff", "webp", "heic", "heif", "avif", "emf",
    "wmf"
};

bool ImageLoader::IsImage(LPCTSTR szFileName) {
    if (!szFileName) {
        return false;
    }
    LPCTSTR szExt = WinUtils::GetFileExt(szFileName);
    if (!lstrlen(szExt)) {
        return false;
    }
    CString find = szExt;
    find.MakeLower();
    return ImageLoader::SupportedImageExtensions.find(W2U(find)) != ImageLoader::SupportedImageExtensions.end();
}

bool ImageLoader::IsImage(const std::string& fileName) {
    if (fileName.empty()) {
        return false;
    }
    std::string ext = IuCoreUtils::ExtractFileExt(fileName);
    if (ext.empty()) {
        return false;
    }
    ext = IuStringUtils::ToLower(ext);
    return ImageLoader::SupportedImageExtensions.find(ext) != ImageLoader::SupportedImageExtensions.end();
}

std::unique_ptr<GdiPlusImage> ImageLoader::loadFromFile(const wchar_t* fileName) {
    FILE* f = _wfopen(fileName, L"rb");
    if (!f) {
        lastError_ = str(boost::wformat(L"Cannot open file %s for reading.") % std::wstring(fileName));
        return nullptr;
    }
    uint8_t buffer[16];
    fread(buffer, 1, sizeof(buffer), f);
    fclose(f);
    ImageFormat format = getImageFormatFromData(buffer, sizeof(buffer));
    auto reader = createReaderForFormat(format);
    auto result = reader->readFromFile(fileName);
    if (!result) {
        lastError_ = reader->getLastError();
    }
    return result;
}

std::unique_ptr<GdiPlusImage> ImageLoader::loadFromMemory(uint8_t* data, size_t size) {
    ImageFormat format = getImageFormatFromData(data, sizeof(size));
    auto reader = createReaderForFormat(format);
    auto result = reader->readFromMemory(data, size);
    if (!result) {
        lastError_ = reader->getLastError();
    }
    return result;
}

std::unique_ptr<GdiPlusImage> ImageLoader::loadFromResource(HINSTANCE hInstance, LPCTSTR szResName,
    LPCTSTR szResType) {
    using namespace Gdiplus;
    HRSRC hrsrc = FindResource(hInstance, szResName, szResType);
    if (!hrsrc)
        return nullptr;
    // "Fake" HGLOBAL - look at MSDN
    HGLOBAL hg1 = LoadResource(hInstance, hrsrc);
    DWORD sz = SizeofResource(hInstance, hrsrc);
    void* ptr1 = LockResource(hg1);

    ImageFormat format = getImageFormatFromData(static_cast<uint8_t*>(ptr1), sz);

    IStream* pStream = ::SHCreateMemStream(static_cast<LPBYTE>(ptr1), sz);
    if (!pStream) {
        return nullptr;
    }

    // use load from IStream
    auto reader = createReaderForFormat(format);
    auto result = reader->readFromStream(pStream);
    if (!result) {
        lastError_ = reader->getLastError();
    }
    pStream->Release();
    // MSDN: It is not necessary to unlock resources because the system automatically deletes
    // them when the process that created them terminates.
    return result;
}

std::wstring ImageLoader::getLastError() const {
    return lastError_;
}

ImageFormat ImageLoader::getImageFormatFromData(uint8_t* data, size_t size) {
    if (size < 12) {
        return ImageFormat::Unknown;
    }
    if (HeifImageReader::canRead(data, size)) {
        return ImageFormat::Heif;
    }
    if (data[0] == 'R' && data[1] == 'I' &&data[2] == 'F' &&data[3] == 'F'
        && data[8] == 'W' && data[9] == 'E' && data[10] == 'B'&& data[11] == 'P'
        ) {
        return ImageFormat::WebP;
    }
    return ImageFormat::Unknown;
}

std::unique_ptr<AbstractImageReader> ImageLoader::createReaderForFormat(ImageFormat format) {
    if (format == ImageFormat::Heif) {
        return std::make_unique<HeifImageReader>();
    }
    if (format == ImageFormat::WebP) {
        return std::make_unique<WebpImageReader>();
    }
    return std::make_unique<GdiplusImageReader>();
}
