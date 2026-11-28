#include "GdiplusImageReader.h"

#include <vector>

#include "Core/Utils/CoreUtils.h"
#include "Func/WinUtils.h"
#include "Utils.h"

std::unique_ptr<GdiPlusImage> GdiplusImageReader::readFromFile(const wchar_t* fileName) {
    return finishLoad(std::make_unique<Gdiplus::Bitmap>(fileName));
}

std::unique_ptr<GdiPlusImage> GdiplusImageReader::readFromMemory(uint8_t* data, size_t size) {
    IStream* pStream = SHCreateMemStream(data, size);
    if (pStream) {
        auto bitmap = readFromStream(pStream);
        pStream->Release();
        return bitmap;
    }

    return nullptr;
}

std::unique_ptr<GdiPlusImage> GdiplusImageReader::readFromStream(IStream* stream) {
    return finishLoad(std::unique_ptr<Gdiplus::Bitmap>(Gdiplus::Bitmap::FromStream(stream, FALSE)));
}

std::wstring GdiplusImageReader::getLastError() {
    return lastError_;
}

bool GdiplusImageReader::checkLastStatus(Gdiplus::Bitmap* bm) {
    if (!bm) {
        return false;
    }
    DWORD lastError = ::GetLastError();
    Gdiplus::Status status = bm->GetLastStatus();
    if (status != Gdiplus::Ok) {

        CString error = ImageUtils::GdiplusStatusToString(status);

        if (status == Gdiplus::Win32Error) {
            error += L"\r\n" + WinUtils::FormatWindowsErrorMessage(lastError);
        }
        lastError_ = error;
        return false;
    }
    return true;
}

std::unique_ptr<GdiPlusImage> GdiplusImageReader::finishLoad(std::unique_ptr<Gdiplus::Bitmap> bitmap) {
    using namespace Gdiplus;
    if (!checkLastStatus(bitmap.get())) {
        return { };
    }

    auto result = std::make_unique<GdiPlusImage>();
    GUID format { };
    if (bitmap->GetRawFormat(&format) == Ok) {
        result->setSrcFormat(W2U(ImageUtils::ImageFormatGUIDToString(format)));
    }
    short orient = ImageUtils::GetImageOrientation(bitmap.get());
    ImageUtils::RotateAccordingToOrientation(orient, bitmap.get(), true);
    result->setSrcMultiFrame(ImageUtils::IsImageMultiFrame(bitmap.get()));
    const auto frameCount = format == ImageFormatGIF ? bitmap->GetFrameCount(&FrameDimensionTime) : 0;
    if (frameCount <= 1) {
        result->appendFrame(std::move(bitmap), 0);
        return result;
    }

    const auto propertySize = bitmap->GetPropertyItemSize(PropertyTagFrameDelay);
    std::vector<uint8_t> propertyBuffer(propertySize);
    auto* property = reinterpret_cast<PropertyItem*>(propertyBuffer.data());
    const bool hasDelays = propertySize >= sizeof(PropertyItem)
        && bitmap->GetPropertyItem(PropertyTagFrameDelay, propertySize, property) == Ok
        && property->type == PropertyTagTypeLong && property->value;

    const auto width = bitmap->GetWidth();
    const auto height = bitmap->GetHeight();
    const Rect bounds(0, 0, width, height);
    const auto framesToLoad = frameMode_ == ImageFrameMode::ALL_FRAMES ? frameCount : 1;
    for (UINT index = 0; index < framesToLoad; ++index) {
        if (bitmap->SelectActiveFrame(&FrameDimensionTime, index) != Ok) {
            lastError_ = L"Could not select GIF animation frame";
            return { };
        }
        auto frame = std::make_unique<Bitmap>(width, height, PixelFormat32bppARGB);
        {
            Graphics graphics(frame.get());
            graphics.SetCompositingMode(CompositingModeSourceCopy);
            if (graphics.DrawImage(bitmap.get(), bounds, 0, 0, width, height, UnitPixel) != Ok) {
                lastError_ = L"Could not decode GIF animation frame";
                return { };
            }
        }
        const auto durationMs = hasDelays && index < property->length / sizeof(ULONG)
            ? 10ULL * static_cast<const ULONG*>(property->value)[index]
            : 0;
        result->appendFrame(std::move(frame), durationMs);
    }
    return result;
}
