#include "FrameGrabberController.h"

#include <QAbstractListModel>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QImageWriter>
#include <QPointer>
#include <QQuickImageProvider>
#include <QReadWriteLock>
#include <QTemporaryFile>
#include <QUrl>
#include <QWidget>

#include <algorithm>

#include "Core/AppRuntimeInfo.h"
#include "Core/CommonDefs.h"
#include "Core/ServiceLocator.h"
#include "Core/Settings/CommonGuiSettings.h"
#include "Core/Settings/QtGuiSettings.h"
#include "Video/QtImage.h"

class FrameGrabberImageStore final {
public:
    void insert(const QString& id, const QImage& image) {
        QWriteLocker locker(&Lock_);
        Images_.insert(id, image);
    }

    QImage image(const QString& id) const {
        QReadLocker locker(&Lock_);
        return Images_.value(id);
    }

    void clear() {
        QWriteLocker locker(&Lock_);
        Images_.clear();
    }

    void remove(const QString& id) {
        QWriteLocker locker(&Lock_);
        Images_.remove(id);
    }

private:
    mutable QReadWriteLock Lock_;
    QHash<QString, QImage> Images_;
};

class FrameGrabberImageProvider final : public QQuickImageProvider {
public:
    explicit FrameGrabberImageProvider(std::shared_ptr<FrameGrabberImageStore> imageStore) :
        QQuickImageProvider(QQuickImageProvider::Image), ImageStore_(std::move(imageStore)) { }

    QImage requestImage(const QString& id, QSize* size, const QSize& requestedSize) override {
        QImage image = ImageStore_->image(id);
        if (size) {
            *size = image.size();
        }
        if (!image.isNull() && requestedSize.isValid()) {
            image = image.scaled(requestedSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        }
        return image;
    }

private:
    std::shared_ptr<FrameGrabberImageStore> ImageStore_;
};

class FrameGrabberFrameModel final : public QAbstractListModel {
public:
    enum Roles { TIME_ROLE = Qt::UserRole + 1, FILE_PATH_ROLE, SOURCE_ROLE };

    explicit FrameGrabberFrameModel(QObject* parent = nullptr) : QAbstractListModel(parent) { }

    int rowCount(const QModelIndex& parent = { }) const override { return parent.isValid() ? 0 : Frames_.size(); }

    QVariant data(const QModelIndex& index, int role) const override {
        if (!index.isValid() || index.row() < 0 || index.row() >= Frames_.size()) {
            return { };
        }
        const Frame& frame = Frames_[index.row()];
        switch (role) {
        case TIME_ROLE:
            return frame.Time;
        case FILE_PATH_ROLE:
            return frame.FilePath;
        case SOURCE_ROLE:
            return frame.Source;
        default:
            return { };
        }
    }

    QHash<int, QByteArray> roleNames() const override {
        return { { TIME_ROLE, "time" }, { FILE_PATH_ROLE, "filePath" }, { SOURCE_ROLE, "source" } };
    }

    void append(const QString& timeText, const QString& fileName, const QUrl& source, const QString& imageId) {
        const int row = Frames_.size();
        beginInsertRows({ }, row, row);
        Frames_.append({ timeText, fileName, source, imageId });
        endInsertRows();
    }

    void clear() {
        if (Frames_.isEmpty()) {
            return;
        }
        beginResetModel();
        Frames_.clear();
        endResetModel();
    }

    QList<QPair<QString, QString>> removeIndices(QList<int> indices) {
        std::sort(indices.begin(), indices.end(), std::greater<int>());
        indices.erase(std::unique(indices.begin(), indices.end()), indices.end());

        QList<QPair<QString, QString>> removedFrames;
        for (const int index : indices) {
            if (index < 0 || index >= Frames_.size()) {
                continue;
            }
            beginRemoveRows({ }, index, index);
            const Frame frame = Frames_.takeAt(index);
            endRemoveRows();
            removedFrames.append({ frame.FilePath, frame.ImageId });
        }
        return removedFrames;
    }

    QString fileNameAt(int index) const {
        return index >= 0 && index < Frames_.size() ? Frames_[index].FilePath : QString { };
    }

    QStringList fileNames() const {
        QStringList result;
        result.reserve(Frames_.size());
        for (const Frame& frame : Frames_) {
            result.append(frame.FilePath);
        }
        return result;
    }

private:
    struct Frame {
        QString Time;
        QString FilePath;
        QUrl Source;
        QString ImageId;
    };

    QList<Frame> Frames_;
};

FrameGrabberController::FrameGrabberController(QWidget* dialogParent, QObject* parent) :
    QObject(parent), DialogParent_(dialogParent), ImageStore_(std::make_shared<FrameGrabberImageStore>()),
    Frames_(std::make_unique<FrameGrabberFrameModel>(this)) {
    auto* settings = ServiceLocator::instance()->settings<QtGuiSettings>();
    FrameCount_ = std::clamp(settings->VideoSettings.NumOfFrames, 1, 10000);
    for (const std::string& engine : CommonGuiSettings::VideoEngines) {
        VideoEngines_.append(U2Q(engine));
    }

    SelectedEngine_ = U2Q(settings->VideoSettings.Engine);
    if (!VideoEngines_.contains(SelectedEngine_) && !VideoEngines_.isEmpty()) {
        SelectedEngine_ = VideoEngines_.first();
    }
}

FrameGrabberController::~FrameGrabberController() {
    if (Grabber_ && (Running_ || Grabber_->isRunning())) {
        Grabber_->abort();
        Grabber_.release();
    }
}

QString FrameGrabberController::fileName() const { return QDir::toNativeSeparators(FileName_); }

void FrameGrabberController::setFileName(const QString& fileName) {
    const QString normalizedFileName = QDir::fromNativeSeparators(fileName);
    if (FileName_ == normalizedFileName || Running_) {
        return;
    }
    FileName_ = normalizedFileName;
    ErrorText_.clear();
    emit stateChanged();
}

QStringList FrameGrabberController::videoEngines() const { return VideoEngines_; }

QString FrameGrabberController::selectedEngine() const { return SelectedEngine_; }

void FrameGrabberController::setSelectedEngine(const QString& selectedEngine) {
    if (SelectedEngine_ == selectedEngine || Running_ || !VideoEngines_.contains(selectedEngine)) {
        return;
    }
    SelectedEngine_ = selectedEngine;
    ServiceLocator::instance()->settings<QtGuiSettings>()->VideoSettings.Engine = Q2U(SelectedEngine_);
    emit stateChanged();
}

int FrameGrabberController::frameCount() const { return FrameCount_; }

void FrameGrabberController::setFrameCount(int frameCount) {
    const int boundedFrameCount = std::clamp(frameCount, 1, 10000);
    if (FrameCount_ == boundedFrameCount || Running_) {
        return;
    }
    FrameCount_ = boundedFrameCount;
    ServiceLocator::instance()->settings<QtGuiSettings>()->VideoSettings.NumOfFrames = FrameCount_;
    emit stateChanged();
}

QAbstractItemModel* FrameGrabberController::frames() const { return Frames_.get(); }

QStringList FrameGrabberController::frameFileNames() const { return Frames_->fileNames(); }

int FrameGrabberController::extractedFrameCount() const { return Frames_->rowCount(); }

bool FrameGrabberController::running() const { return Running_; }

bool FrameGrabberController::canStart() const { return !Running_ && !FileName_.isEmpty(); }

bool FrameGrabberController::canAccept() const { return !Running_ && Frames_->rowCount() > 0; }

QString FrameGrabberController::errorText() const { return ErrorText_; }

bool FrameGrabberController::prepareFile(const QString& fileName) {
    if (Running_) {
        return false;
    }
    FileName_ = QDir::fromNativeSeparators(fileName);
    ImageStore_->clear();
    Frames_->clear();
    ErrorText_.clear();
    emit stateChanged();
    return !FileName_.isEmpty();
}

QQuickImageProvider* FrameGrabberController::createImageProvider() const {
    return new FrameGrabberImageProvider(ImageStore_);
}

void FrameGrabberController::browseFile() {
    if (Running_) {
        return;
    }
    const QString fileName = QFileDialog::getOpenFileName(
        DialogParent_, tr("Open video file"), FileName_,
        tr("Video files (*.avi *.mkv *.mov *.mp4 *.mpeg *.mpg *.webm *.wmv);;All files (*.*)"));
    if (!fileName.isEmpty()) {
        setFileName(fileName);
    }
}

void FrameGrabberController::start() {
    if (!canStart()) {
        return;
    }
    if (!QFileInfo::exists(FileName_)) {
        ErrorText_ = tr("The selected video file does not exist");
        emit stateChanged();
        return;
    }
    if (Grabber_ && Grabber_->isRunning()) {
        ErrorText_ = tr("The previous operation is still stopping");
        emit stateChanged();
        return;
    }

    ErrorText_.clear();
    CancelRequested_ = false;
    Running_ = true;
    emit stateChanged();

    Grabber_ = std::make_unique<VideoGrabber>();
    Grabber_->setVideoEngine(videoEngine());
    Grabber_->setFrameCount(FrameCount_);

    const QPointer<FrameGrabberController> guard(this);
    Grabber_->setOnFrameGrabbed(
        [guard](const std::string& timeText, int64_t t, const std::shared_ptr<AbstractImage>& image) {
            qDebug() << t;
            if (!guard || !image) {
                return;
            }
            const auto* qtImage = dynamic_cast<QtImage*>(image.get());
            if (!qtImage) {
                return;
            }
            const QImage frame = qtImage->toQImage();
            if (frame.isNull()) {
                return;
            }

            const QString tempDirectory = U2Q(AppRuntimeInfo::instance()->tempDirectory());
            QTemporaryFile outputFile(tempDirectory + QStringLiteral("/grab_XXXXXX.png"));
            outputFile.setAutoRemove(false);
            if (!outputFile.open()) {
                return;
            }
            const QString outputFileName = outputFile.fileName();
            outputFile.close();

            qDebug() << outputFileName;

            QImageWriter writer(outputFileName);
            writer.setCompression(1);
            writer.setQuality(90);
            if (!writer.write(frame) || !guard) {
                return;
            }

            const QString time = U2Q(timeText);
            const QImage thumbnail
                = frame.scaled(QSize(360, 200), Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
            QMetaObject::invokeMethod(
                guard.data(),
                [guard, time, outputFileName, thumbnail] {
                    if (guard) {
                        guard->appendFrame(time, outputFileName, thumbnail);
                    }
                },
                Qt::QueuedConnection);
        });
    Grabber_->setOnFinished([guard](bool success) {
        if (!guard) {
            return;
        }
        QMetaObject::invokeMethod(
            guard.data(),
            [guard, success] {
                if (guard) {
                    guard->grabFinished(success);
                }
            },
            Qt::QueuedConnection);
    });
    Grabber_->grab(Q2U(FileName_));
}

void FrameGrabberController::stop() {
    if (!Running_ || !Grabber_) {
        return;
    }
    CancelRequested_ = true;
    Grabber_->abort();
}

void FrameGrabberController::cancel() { stop(); }

void FrameGrabberController::acceptFrames() {
    if (!canAccept()) {
        return;
    }
    emit framesAccepted(Frames_->fileNames());
}

void FrameGrabberController::openFrame(int index) {
    if (Frames_->fileNameAt(index).isEmpty()) {
        return;
    }
    emit frameViewerRequested(index);
}

void FrameGrabberController::removeFrames(const QVariantList& indices) {
    QList<int> rows;
    rows.reserve(indices.size());
    for (const QVariant& index : indices) {
        rows.append(index.toInt());
    }

    const QList<QPair<QString, QString>> removedFrames = Frames_->removeIndices(std::move(rows));
    for (const auto& [fileName, imageId] : removedFrames) {
        ImageStore_->remove(imageId);
        QFile::remove(fileName);
    }
    if (!removedFrames.isEmpty()) {
        emit stateChanged();
    }
}

void FrameGrabberController::appendFrame(const QString& timeText, const QString& fileName, const QImage& image) {
    const QString imageId = QString::number(++NextFrameId_);
    ImageStore_->insert(imageId, image);
    Frames_->append(timeText, fileName, QUrl(QStringLiteral("image://framegrabber/") + imageId), imageId);
    emit stateChanged();
}

void FrameGrabberController::grabFinished(bool success) {
    Running_ = false;
    if (!success && !CancelRequested_) {
        ErrorText_ = tr("Unable to extract frames from the video");
    }
    CancelRequested_ = false;
    emit stateChanged();
}

VideoGrabber::VideoEngine FrameGrabberController::videoEngine() const {
    auto* settings = ServiceLocator::instance()->settings<QtGuiSettings>();
    std::string videoEngine = Q2U(SelectedEngine_);

    if (videoEngine == QtGuiSettings::VideoEngineAuto) {
        if (!settings->IsFFmpegAvailable()) {
            videoEngine = QtGuiSettings::VideoEngineDirectshow;
        } else {
            videoEngine = QtGuiSettings::VideoEngineFFmpeg;
            const QString fileExtension = QFileInfo(FileName_).suffix().toLower();
            if (fileExtension == QStringLiteral("wmv") || fileExtension == QStringLiteral("asf")) {
                videoEngine = QtGuiSettings::VideoEngineDirectshow;
            }
        }
    }

    VideoGrabber::VideoEngine engine = VideoGrabber::veAuto;
#ifdef IU_ENABLE_FFMPEG
    if (videoEngine == QtGuiSettings::VideoEngineFFmpeg) {
        engine = VideoGrabber::veAvcodec;
    } else
#endif
        if (videoEngine == QtGuiSettings::VideoEngineDirectshow) {
        engine = VideoGrabber::veDirectShow;
    } else if (videoEngine == QtGuiSettings::VideoEngineDirectshow2) {
        engine = VideoGrabber::veDirectShow2;
    } else if (videoEngine == QtGuiSettings::VideoEngineMediaFoundation) {
        engine = VideoGrabber::veMediaFoundation;
    }
    return engine;
}
