#include "WebpImageReader.h"

#include <cstring>
#include <limits>

#include <webp/decode.h>
#include <webp/demux.h>

#include "Utils.h"

using namespace Gdiplus;

namespace {

std::unique_ptr<Bitmap> CopyBgraBitmap(const uint8_t* data, int width, int height) {
    auto bitmap = std::make_unique<Bitmap>(width, height, PixelFormat32bppARGB);
    BitmapData dstData;
    Rect rect(0, 0, width, height);
    if (bitmap->LockBits(&rect, ImageLockModeWrite, PixelFormat32bppARGB, &dstData) != Ok) {
        return { };
    }
    const auto rowSize = static_cast<size_t>(width) * 4;
    for (int y = 0; y < height; ++y) {
        auto* dstRow = static_cast<uint8_t*>(dstData.Scan0) + static_cast<ptrdiff_t>(y) * dstData.Stride;
        std::memcpy(dstRow, data + y * rowSize, rowSize);
    }
    bitmap->UnlockBits(&dstData);
    return bitmap;
}

} // namespace

std::unique_ptr<GdiPlusImage> WebpImageReader::readFromFile(const wchar_t* fileName) {
    lastError_.clear();
    try {
        auto [data, size] = ImageUtils::ExUtilReadFile(fileName);
        return readFromMemory(data.get(), size);
    }
    catch (const std::exception&) {
        lastError_ = L"Could not load WebP file";
        return { };
    }
}

std::unique_ptr<GdiPlusImage> WebpImageReader::readFromMemory(uint8_t* data, size_t size) {
    lastError_.clear();
    WebPBitstreamFeatures features;
    if (WebPGetFeatures(data, size, &features) != VP8_STATUS_OK) {
        lastError_ = L"Could not read WebP image features";
        return { };
    }

    if (features.has_animation) {
        return readAnimation(data, size);
    }

    std::unique_ptr<uint8_t, decltype(&WebPFree)> pixels(WebPDecodeBGRA(data, size, nullptr, nullptr), WebPFree);
    auto bitmap = pixels ? CopyBgraBitmap(pixels.get(), features.width, features.height) : nullptr;
    if (!bitmap) {
        lastError_ = L"Could not decode WebP image";
        return { };
    }
    auto result = std::make_unique<GdiPlusImage>();
    result->appendFrame(std::move(bitmap), 0);
    result->setSrcFormat("webp");
    return result;
}

std::unique_ptr<GdiPlusImage> WebpImageReader::readAnimation(const uint8_t* data, size_t size) {
    WebPAnimDecoderOptions options;
    if (!WebPAnimDecoderOptionsInit(&options)) {
        lastError_ = L"Could not initialize WebP animation options";
        return { };
    }
    options.color_mode = MODE_BGRA;
    const WebPData webpData { data, size };
    std::unique_ptr<WebPAnimDecoder, decltype(&WebPAnimDecoderDelete)> decoder(WebPAnimDecoderNew(&webpData, &options),
                                                                               WebPAnimDecoderDelete);
    WebPAnimInfo info;
    if (!decoder || !WebPAnimDecoderGetInfo(decoder.get(), &info) || info.frame_count == 0) {
        lastError_ = L"Could not read WebP animation";
        return { };
    }

    auto result = std::make_unique<GdiPlusImage>();
    const auto frameCount = frameMode_ == ImageFrameMode::ALL_FRAMES ? info.frame_count : 1;
    int previousTimestamp = 0;
    for (uint32_t index = 0; index < frameCount; ++index) {
        uint8_t* pixels;
        int timestamp;
        if (!WebPAnimDecoderGetNext(decoder.get(), &pixels, &timestamp) || timestamp < previousTimestamp) {
            lastError_ = L"Could not decode WebP animation frame";
            return { };
        }
        auto bitmap = CopyBgraBitmap(pixels, info.canvas_width, info.canvas_height);
        if (!bitmap) {
            lastError_ = L"Could not create a bitmap from the WebP animation frame";
            return { };
        }
        result->appendFrame(std::move(bitmap), timestamp - previousTimestamp);
        previousTimestamp = timestamp;
    }
    result->setSrcMultiFrame(true);
    result->setSrcFormat("webp");
    return result;
}

std::unique_ptr<GdiPlusImage> WebpImageReader::readFromStream(IStream* stream) {
    lastError_.clear();
    LARGE_INTEGER start { };
    if (FAILED(stream->Seek(start, STREAM_SEEK_SET, nullptr))) {
        return { };
    }
    STATSTG stats;
    if (FAILED(stream->Stat(&stats, STATFLAG_NONAME))) {
        return { };
    }
    if (stats.cbSize.QuadPart > std::numeric_limits<ULONG>::max()) {
        lastError_ = L"WebP stream is too large";
        return { };
    }
    const auto size = static_cast<ULONG>(stats.cbSize.QuadPart);
    try {
        auto buffer = std::make_unique<uint8_t[]>(size);
        ULONG bytesRead;
        if (FAILED(stream->Read(buffer.get(), size, &bytesRead)) || bytesRead != size) {
            lastError_ = L"Failed to read WebP stream";
            return { };
        }
        return readFromMemory(buffer.get(), size);
    }
    catch (const std::bad_alloc&) {
        lastError_ = L"Could not allocate memory for WebP image";
        return { };
    }
}

std::wstring WebpImageReader::getLastError() {
    return lastError_;
}
