#include "HeifImageReader.h"

#include <libheif/heif.h>
#include <libheif/heif_sequences.h>

#include <cstring>
#include <limits>
#include <vector>

#include <boost/format.hpp>

#include "Core/Images/GdiPlusImage.h"
#include "Core/Utils/CoreUtils.h"
#include "Utils.h"

using namespace Gdiplus;

namespace {

using HeifContext = std::unique_ptr<heif_context, decltype(&heif_context_free)>;

constexpr int BRAND_HEADER_SIZE = 12;

uint64_t ReadBoxSize(const uint8_t* data, size_t size) {
    uint64_t value = 0;
    for (size_t i = 0; i < size; ++i) {
        value = (value << 8) | data[i];
    }
    return value;
}

// libheif requires a primary item when a top-level meta box describes images.
// An AVIF sequence can still provide its frames through the separate moov box.
// Treat meta as padding in the working copy to preserve all track data offsets.
size_t FindMetaBoxType(const uint8_t* data, size_t size) {
    for (size_t offset = 0; size - offset >= 8;) {
        const auto* box = data + offset;
        const auto remaining = size - offset;
        auto boxSize = ReadBoxSize(box, 4);
        size_t headerSize = 8;
        if (boxSize == 1) {
            headerSize = 16;
            if (remaining < headerSize) {
                break;
            }
            boxSize = ReadBoxSize(box + 8, 8);
        } else if (boxSize == 0) {
            boxSize = remaining;
        }
        if (boxSize < headerSize || boxSize > remaining) {
            break;
        }
        if (std::memcmp(box + 4, "meta", 4) == 0) {
            return offset + 4;
        }
        offset += static_cast<size_t>(boxSize);
    }
    return size;
}

} // namespace

bool HeifImageReader::canRead(const uint8_t* data, size_t size) {
    if (size < BRAND_HEADER_SIZE) {
        return false;
    }
    const auto type = heif_check_filetype(data, BRAND_HEADER_SIZE);
    return type != heif_filetype_no
        && (type != heif_filetype_yes_unsupported || heif_read_main_brand(data, BRAND_HEADER_SIZE) == heif_brand2_avis);
}

std::unique_ptr<GdiPlusImage> HeifImageReader::readFromFile(const wchar_t* fileName) {
    lastError_.clear();
    HeifContext context(heif_context_alloc(), heif_context_free);
    const auto err = heif_context_read_from_file(context.get(), IuCoreUtils::WstringToUtf8(fileName).c_str(), nullptr);
    if (err.code != heif_error_Ok) {
        lastError_ = boost::str(boost::wformat(L"Could not read HEIF/AVIF file: %s") % err.message);
        if (err.subcode == heif_suberror_No_pitm_box) {
            try {
                auto [data, size] = ImageUtils::ExUtilReadFile(fileName);
                return readFromMemory(data.get(), size);
            }
            catch (const std::exception&) {
                return { };
            }
        }
        return { };
    }
    return decode(context.get());
}

std::unique_ptr<GdiPlusImage> HeifImageReader::decode(heif_context* context) {
    std::unique_ptr<heif_track, decltype(&heif_track_release)> track(
        heif_context_has_sequence(context) ? heif_context_get_track(context, 0) : nullptr, heif_track_release);
    std::unique_ptr<heif_image_handle, decltype(&heif_image_handle_release)> handle(nullptr, heif_image_handle_release);
    if (!track) {
        heif_image_handle* primary = nullptr;
        const auto err = heif_context_get_primary_image_handle(context, &primary);
        handle.reset(primary);
        if (err.code != heif_error_Ok) {
            lastError_ = boost::str(boost::wformat(L"Could not get the primary HEIF/AVIF image: %s") % err.message);
            return { };
        }
    }

    const auto timescale = track ? heif_track_get_timescale(track.get()) : 0;
    std::unique_ptr<heif_decoding_options, decltype(&heif_decoding_options_free)> options(nullptr,
                                                                                          heif_decoding_options_free);
    if (track && frameMode_ == ImageFrameMode::ALL_FRAMES) {
        options.reset(heif_decoding_options_alloc());
        if (!options) {
            lastError_ = L"Could not allocate HEIF/AVIF decoding options";
            return { };
        }
        // Decode one cycle. The viewer handles repetition during playback.
        options->ignore_sequence_editlist = true;
    }

    auto result = std::make_unique<GdiPlusImage>();
    do {
        heif_image* image = nullptr;
        const auto err = track
            ? heif_track_decode_next_image(track.get(), &image, heif_colorspace_RGB, heif_chroma_interleaved_RGBA,
                                           options.get())
            : heif_decode_image(handle.get(), &image, heif_colorspace_RGB, heif_chroma_interleaved_RGBA, nullptr);
        std::unique_ptr<heif_image, decltype(&heif_image_release)> decoded(image, heif_image_release);
        if (err.code == heif_error_End_of_sequence && result->frameCount() != 0) {
            break;
        }
        if (err.code != heif_error_Ok) {
            lastError_ = boost::str(boost::wformat(L"Could not decode HEIF/AVIF file: %s") % err.message);
            return { };
        }
        auto bitmap = decoded ? readHeif(decoded.get()) : nullptr;
        if (!bitmap) {
            lastError_ = L"Could not create a bitmap from the HEIF/AVIF image";
            return { };
        }
        const auto durationMs
            = timescale ? (1000ULL * heif_image_get_duration(decoded.get()) + timescale / 2) / timescale : 0;
        result->appendFrame(std::move(bitmap), durationMs);
    }
    while (track && frameMode_ == ImageFrameMode::ALL_FRAMES);

    result->setSrcFormat("HEIF/AVIF");
    result->setSrcMultiFrame(static_cast<bool>(track));
    return result;
}

std::unique_ptr<Gdiplus::Bitmap> HeifImageReader::readHeif(heif_image* img) {
    int stride;
    const uint8_t* data = heif_image_get_plane_readonly(img, heif_channel_interleaved, &stride);

    int width = heif_image_get_width(img, heif_channel_interleaved);
    int height = heif_image_get_height(img, heif_channel_interleaved);

    if (!data || width <= 0 || height <= 0) {
        return { };
    }
    auto bm = std::make_unique<Gdiplus::Bitmap>(width, height, PixelFormat32bppARGB);

    BitmapData dstData;
    Rect rc(0, 0, width, height);
    constexpr auto colorSize = 4;
    if (bm->LockBits(&rc, ImageLockModeWrite, PixelFormat32bppARGB, &dstData) != Ok) {
        return { };
    }
    for (int y = 0; y < height; y++) {
        const auto* srcRow = data + static_cast<ptrdiff_t>(y) * stride;
        auto* dstRow = static_cast<uint8_t*>(dstData.Scan0) + static_cast<ptrdiff_t>(y) * dstData.Stride;
        for (int x = 0; x < width; x++) {
            const auto offset = static_cast<size_t>(x) * colorSize;
            dstRow[offset] = srcRow[offset + 2];
            dstRow[offset + 1] = srcRow[offset + 1];
            dstRow[offset + 2] = srcRow[offset];
            dstRow[offset + 3] = srcRow[offset + 3];
        }
    }
    bm->UnlockBits(&dstData);
    return bm;
}

std::unique_ptr<GdiPlusImage> HeifImageReader::readFromMemory(uint8_t* data, size_t size) {
    lastError_.clear();
    std::vector<uint8_t> sequenceData;
    HeifContext context(heif_context_alloc(), heif_context_free);
    auto err = heif_context_read_from_memory_without_copy(context.get(), data, size, nullptr);
    if (err.subcode == heif_suberror_No_pitm_box && size >= BRAND_HEADER_SIZE
        && heif_read_main_brand(data, BRAND_HEADER_SIZE) == heif_brand2_avis) {
        const auto metaTypeOffset = FindMetaBoxType(data, size);
        if (metaTypeOffset != size) {
            try {
                sequenceData.assign(data, data + size);
            }
            catch (const std::exception&) {
                lastError_ = L"Could not allocate memory for the AVIF sequence";
                return { };
            }
            std::memcpy(sequenceData.data() + metaTypeOffset, "free", 4);
            context.reset(heif_context_alloc());
            err = heif_context_read_from_memory_without_copy(context.get(), sequenceData.data(), sequenceData.size(),
                                                             nullptr);
        }
    }
    if (err.code != heif_error_Ok) {
        lastError_ = boost::str(boost::wformat(L"Could not read HEIF/AVIF file: %s") % err.message);
        return { };
    }
    return decode(context.get());
}

std::unique_ptr<GdiPlusImage> HeifImageReader::readFromStream(IStream* stream) {
    lastError_.clear();
    LARGE_INTEGER li;
    li.QuadPart = 0;

    if (FAILED(stream->Seek(li, STREAM_SEEK_SET, nullptr))) {
        return { };
    }

    STATSTG stats;
    if (FAILED(stream->Stat(&stats, STATFLAG_NONAME))) {
        return { };
    }
    if (stats.cbSize.QuadPart > std::numeric_limits<ULONG>::max()) {
        lastError_ = L"HEIF/AVIF stream is too large";
        return { };
    }
    const auto size = static_cast<ULONG>(stats.cbSize.QuadPart);
    ULONG bytesRead;
    std::unique_ptr<uint8_t[]> pBuffer;
    try {
        pBuffer = std::make_unique<uint8_t[]>(size);
    }
    catch (const std::exception&) {
        lastError_ = str(boost::wformat(L"Unable to allocate %d bytes") % size);
        return { };
    }

    if (FAILED(stream->Read(pBuffer.get(), size, &bytesRead)) || bytesRead != size) {
        lastError_ = L"Failed to read from IStream";
        return { };
    }

    return readFromMemory(pBuffer.get(), size);
}

std::wstring HeifImageReader::getLastError() {
    return lastError_;
}
