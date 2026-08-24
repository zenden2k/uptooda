#ifndef QIMAGEUPLOADER_GUI_FILETHUMBNAILCACHE_H
#define QIMAGEUPLOADER_GUI_FILETHUMBNAILCACHE_H

#include <QImage>
#include <QObject>

#include <functional>
#include <memory>
#include <optional>

class QDateTime;

class FileThumbnailCache final : public QObject {
public:
    struct Thumbnail {
        QImage Image;
        bool IsImage = false;
    };

    using Callback = std::function<void(const Thumbnail&)>;

    static FileThumbnailCache& instance();

    std::optional<Thumbnail> cached(const QString& fileName) const;
    void request(const QString& fileName, QObject* context, Callback callback);

private:
    class Impl;

    FileThumbnailCache();
    ~FileThumbnailCache() override;

    void startLoad(const QString& fileName, const QDateTime& lastModified, quint64 request);

    std::unique_ptr<Impl> impl_;
};

#endif
