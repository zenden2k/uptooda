#include "ImageViewerWindow.h"

#include <QCloseEvent>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QImageReader>
#include <QMutex>
#include <QMutexLocker>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickImageProvider>
#include <QtConcurrentRun>

class ViewerImageProvider final : public QQuickImageProvider {
public:
    ViewerImageProvider() : QQuickImageProvider(QQuickImageProvider::Image) { }

    QImage requestImage(const QString&, QSize* size, const QSize&) override {
        QMutexLocker locker(&Mutex_);
        if (size) {
            *size = Image_.size();
        }
        return Image_;
    }

    void setImage(const QImage& image) {
        QMutexLocker locker(&Mutex_);
        Image_ = image;
    }

private:
    QMutex Mutex_;
    QImage Image_;
};

ImageViewerWindow::ImageViewerWindow(QWindow* parent) : QQuickView(parent), ImageProvider_(new ViewerImageProvider) {
    setTitle(tr("Image viewer"));
    setFlags(Qt::Window | Qt::FramelessWindowHint);
    setColor(Qt::transparent);
    setResizeMode(QQuickView::SizeRootObjectToView);
    setPersistentGraphics(true);
    setPersistentSceneGraph(true);
    engine()->addImageProvider(QStringLiteral("imageviewer"), ImageProvider_);
    rootContext()->setContextProperty(QStringLiteral("imageViewerController"), this);
    setSource(QUrl(QStringLiteral("qrc:/qt/qml/Uptooda/Ui/ImageViewerWindow.qml")));
}

ImageViewerWindow::~ImageViewerWindow() = default;

void ImageViewerWindow::setImageViewerSource(std::unique_ptr<IImageViewerSource> source) {
    hide();
    OpenWhenReady_ = false;
    ImageSource_.clear();
    ImageProvider_->setImage({ });
    Source_ = std::move(source);
    loadFile(Source_ ? Source_->currentFile() : QString());
}

void ImageViewerWindow::open() {
    OpenWhenReady_ = true;
    if (!Loading_) {
        showViewer();
    }
}

void ImageViewerWindow::showViewer() {
    OpenWhenReady_ = false;
    showFullScreen();
    raise();
    requestActivate();
}

QString ImageViewerWindow::imageSource() const { return ImageSource_; }

QString ImageViewerWindow::fileName() const { return FileName_; }

QString ImageViewerWindow::errorText() const { return ErrorText_; }

bool ImageViewerWindow::loading() const { return Loading_; }

bool ImageViewerWindow::hasNext() const { return Source_ && Source_->hasNext(); }

bool ImageViewerWindow::hasPrevious() const { return Source_ && Source_->hasPrevious(); }

void ImageViewerWindow::showNext() {
    if (Source_ && Source_->hasNext()) {
        loadFile(Source_->nextFile());
    }
}

void ImageViewerWindow::showPrevious() {
    if (Source_ && Source_->hasPrevious()) {
        loadFile(Source_->previousFile());
    }
}

void ImageViewerWindow::minimizeViewer() { showMinimized(); }

void ImageViewerWindow::toggleFullScreen() {
    if (visibility() == QWindow::FullScreen) {
        showMaximized();
    } else {
        showFullScreen();
    }
}

void ImageViewerWindow::closeViewer() {
    OpenWhenReady_ = false;
    hide();
}

void ImageViewerWindow::closeEvent(QCloseEvent* event) {
    event->ignore();
    closeViewer();
}

void ImageViewerWindow::loadFile(const QString& fileName) {
    const quint64 request = ++LoadRequest_;
    FileName_ = QFileInfo(fileName).fileName();
    ErrorText_.clear();
    Loading_ = !fileName.isEmpty();
    emit viewerStateChanged();

    if (fileName.isEmpty()) {
        Loading_ = false;
        ErrorText_ = tr("Image is unavailable");
        emit viewerStateChanged();
        return;
    }

    auto* watcher = new QFutureWatcher<QImage>(this);
    connect(watcher, &QFutureWatcher<QImage>::finished, this, [this, watcher, request] {
        const QImage image = watcher->result();
        watcher->deleteLater();
        if (request != LoadRequest_) {
            return;
        }
        Loading_ = false;
        if (image.isNull()) {
            ErrorText_ = tr("Unable to load image");
        } else {
            ImageProvider_->setImage(image);
            ImageSource_ = QStringLiteral("image://imageviewer/current?request=%1").arg(request);
        }
        emit viewerStateChanged();
        if (OpenWhenReady_) {
            showViewer();
        }
    });
    watcher->setFuture(QtConcurrent::run([fileName] {
        QImageReader reader(fileName);
        reader.setAutoTransform(true);
        return reader.read();
    }));
}
