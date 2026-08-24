#include "FileThumbnailCache.h"

#include <QDateTime>
#include <QFileIconProvider>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QHash>
#include <QImageReader>
#include <QPointer>
#include <QtConcurrentRun>

#include <algorithm>

#include "Core/CommonDefs.h"
#include "Core/Settings/CommonGuiSettings.h"
#include "Core/Video/VideoUtils.h"
#include "Video/QtImage.h"
#include "Video/VideoGrabber.h"

#ifdef Q_OS_WIN
#include <windows.h>
#include <shellapi.h>
#endif

namespace {

constexpr QSize CACHED_IMAGE_SIZE(256, 172);

struct LoadResult {
    QImage Image;
    bool IsImage = false;
};

bool IsVideoFile(const QString& fileName) {
    const std::string extension = QFileInfo(fileName).suffix().toLower().toStdString();
    return VideoUtils::videoFilesExtensions.find(extension) != VideoUtils::videoFilesExtensions.end();
}

QImage LoadVideoThumbnail(const QString& fileName) {
    if (!CommonGuiSettings::IsFFmpegAvailable()) {
        return { };
    }

    QImage result;
    VideoGrabber grabber(false, false);
    grabber.setVideoEngine(VideoGrabber::veAvcodec);
    grabber.setFrameCount(1);
    grabber.setOnFrameGrabbed([&result](const std::string&, int64_t, const std::shared_ptr<AbstractImage>& frame) {
        const auto qtImage = std::dynamic_pointer_cast<QtImage>(frame);
        if (qtImage) {
            result = qtImage->toQImage();
        }
    });
    try {
        grabber.grab(Q2U(fileName));
    }
    catch (const std::exception&) {
        return { };
    }
    return result.isNull() ? QImage { }
                           : result.scaled(CACHED_IMAGE_SIZE, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

#ifdef Q_OS_WIN
QImage LoadFileIcon(const QString& fileName) {
    SHFILEINFOW fileInfo { };
    if (!SHGetFileInfoW(reinterpret_cast<LPCWSTR>(fileName.utf16()), 0, &fileInfo, sizeof(fileInfo),
                        SHGFI_ICON | SHGFI_LARGEICON)) {
        return { };
    }

    ICONINFO iconInfo { };
    if (!GetIconInfo(fileInfo.hIcon, &iconInfo)) {
        DestroyIcon(fileInfo.hIcon);
        return { };
    }

    BITMAP bitmap { };
    const HBITMAP sizeBitmap = iconInfo.hbmColor ? iconInfo.hbmColor : iconInfo.hbmMask;
    GetObjectW(sizeBitmap, sizeof(bitmap), &bitmap);
    const int width = bitmap.bmWidth;
    const int height = iconInfo.hbmColor ? bitmap.bmHeight : bitmap.bmHeight / 2;

    BITMAPINFO bitmapInfo { };
    bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = -height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    void* pixels = nullptr;
    const HDC deviceContext = CreateCompatibleDC(nullptr);
    const HBITMAP dib = CreateDIBSection(deviceContext, &bitmapInfo, DIB_RGB_COLORS, &pixels, nullptr, 0);
    QImage result;
    if (dib && pixels) {
        const HGDIOBJ oldBitmap = SelectObject(deviceContext, dib);
        std::fill_n(static_cast<QRgb*>(pixels), width * height, QRgb { 0 });
        if (DrawIconEx(deviceContext, 0, 0, fileInfo.hIcon, width, height, 0, nullptr, DI_NORMAL)) {
            result
                = QImage(static_cast<const uchar*>(pixels), width, height, QImage::Format_ARGB32_Premultiplied).copy();
        }
        SelectObject(deviceContext, oldBitmap);
    }

    if (dib) {
        DeleteObject(dib);
    }
    DeleteDC(deviceContext);
    DeleteObject(iconInfo.hbmColor);
    DeleteObject(iconInfo.hbmMask);
    DestroyIcon(fileInfo.hIcon);
    return result;
}
#endif

LoadResult LoadThumbnail(const QString& fileName) {
    if (IsVideoFile(fileName)) {
        const QImage videoThumbnail = LoadVideoThumbnail(fileName);
        if (!videoThumbnail.isNull()) {
            return { videoThumbnail, false };
        }
    }

    QImageReader reader(fileName);
    reader.setAutoTransform(true);
    const QSize sourceSize = reader.size();
    if (sourceSize.isValid()) {
        reader.setScaledSize(sourceSize.scaled(CACHED_IMAGE_SIZE, Qt::KeepAspectRatio));
    }
    QImage image = reader.read();
    if (!image.isNull()) {
        return { image, true };
    }
#ifdef Q_OS_WIN
    return { LoadFileIcon(fileName), false };
#else
    return { };
#endif
}

QString CacheKey(const QString& fileName) { return QFileInfo(fileName).absoluteFilePath(); }

} // namespace

class FileThumbnailCache::Impl {
public:
    struct CacheEntry {
        QDateTime LastModified;
        Thumbnail Data;
    };

    struct Waiter {
        QPointer<QObject> Context;
        Callback Function;
    };

    struct PendingRequest {
        QDateTime LastModified;
        quint64 Request = 0;
        QList<Waiter> Waiters;
    };

    QHash<QString, CacheEntry> Cache;
    QHash<QString, PendingRequest> Pending;
    quint64 NextRequest = 0;
};

FileThumbnailCache& FileThumbnailCache::instance() {
    static FileThumbnailCache cache;
    return cache;
}

FileThumbnailCache::FileThumbnailCache() : impl_(std::make_unique<Impl>()) {

}

FileThumbnailCache::~FileThumbnailCache() = default;

std::optional<FileThumbnailCache::Thumbnail> FileThumbnailCache::cached(const QString& fileName) {
    const QString key = CacheKey(fileName);
    const auto cached = impl_->Cache.constFind(key);
    if (cached == impl_->Cache.cend() || cached->LastModified != QFileInfo(key).lastModified()) {
        return std::nullopt;
    }
    return cached->Data;
}

void FileThumbnailCache::request(const QString& fileName, QObject* context, Callback callback) {
    const QString key = CacheKey(fileName);
    const QDateTime lastModified = QFileInfo(key).lastModified();
    const auto cached = impl_->Cache.constFind(key);
    if (cached != impl_->Cache.cend() && cached->LastModified == lastModified) {
        callback(cached->Data);
        return;
    }

    bool startNeeded = false;
    auto pending = impl_->Pending.find(key);
    if (pending == impl_->Pending.end()) {
        pending = impl_->Pending.insert(key, { lastModified, ++impl_->NextRequest, { } });
        startNeeded = true;
    } else if (pending->LastModified != lastModified) {
        pending->LastModified = lastModified;
        pending->Request = ++impl_->NextRequest;
        startNeeded = true;
    }
    pending->Waiters.append({ context, std::move(callback) });
    if (startNeeded) {
        startLoad(key, pending->LastModified, pending->Request);
    }
}

void FileThumbnailCache::startLoad(const QString& fileName, const QDateTime& lastModified, quint64 request) {
    auto* watcher = new QFutureWatcher<LoadResult>(this);
    connect(watcher, &QFutureWatcher<LoadResult>::finished, this, [this, watcher, fileName, lastModified, request] {
        LoadResult result = watcher->result();
        watcher->deleteLater();

        auto pending = impl_->Pending.find(fileName);
        if (pending == impl_->Pending.end() || pending->Request != request) {
            return;
        }

        const QDateTime currentLastModified = QFileInfo(fileName).lastModified();
        if (currentLastModified != lastModified) {
            pending->LastModified = currentLastModified;
            pending->Request = ++impl_->NextRequest;
            startLoad(fileName, pending->LastModified, pending->Request);
            return;
        }

#ifndef Q_OS_WIN
        if (result.Image.isNull()) {
            QFileIconProvider iconProvider;
            result.Image = iconProvider.icon(QFileInfo(fileName)).pixmap(64, 64).toImage();
        }
#endif

        const Thumbnail thumbnail { result.Image, result.IsImage };
        impl_->Cache.insert(fileName, { lastModified, thumbnail });
        const QList<Impl::Waiter> waiters = pending->Waiters;
        impl_->Pending.erase(pending);
        for (const Impl::Waiter& waiter : waiters) {
            if (waiter.Context) {
                waiter.Function(thumbnail);
            }
        }
    });
    watcher->setFuture(QtConcurrent::run([fileName] { return LoadThumbnail(fileName); }));
}
