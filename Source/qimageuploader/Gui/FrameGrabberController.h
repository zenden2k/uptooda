#ifndef QIMAGEUPLOADER_GUI_FRAMEGRABBERCONTROLLER_H
#define QIMAGEUPLOADER_GUI_FRAMEGRABBERCONTROLLER_H

#include <QAbstractItemModel>
#include <QObject>
#include <QVariantList>

#include <memory>

#include "Video/VideoGrabber.h"

class QWidget;
class QImage;
class QQuickImageProvider;
class FrameGrabberFrameModel;
class FrameGrabberImageStore;

class FrameGrabberController final : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString fileName READ fileName WRITE setFileName NOTIFY stateChanged)
    Q_PROPERTY(QStringList videoEngines READ videoEngines CONSTANT)
    Q_PROPERTY(QString selectedEngine READ selectedEngine WRITE setSelectedEngine NOTIFY stateChanged)
    Q_PROPERTY(int frameCount READ frameCount WRITE setFrameCount NOTIFY stateChanged)
    Q_PROPERTY(QAbstractItemModel* frames READ frames CONSTANT)
    Q_PROPERTY(int extractedFrameCount READ extractedFrameCount NOTIFY stateChanged)
    Q_PROPERTY(bool running READ running NOTIFY stateChanged)
    Q_PROPERTY(bool canStart READ canStart NOTIFY stateChanged)
    Q_PROPERTY(bool canAccept READ canAccept NOTIFY stateChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY stateChanged)

public:
    explicit FrameGrabberController(QWidget* dialogParent, QObject* parent = nullptr);
    ~FrameGrabberController() override;

    QString fileName() const;
    void setFileName(const QString& fileName);
    QStringList videoEngines() const;
    QString selectedEngine() const;
    void setSelectedEngine(const QString& selectedEngine);
    int frameCount() const;
    void setFrameCount(int frameCount);
    QAbstractItemModel* frames() const;
    int extractedFrameCount() const;
    bool running() const;
    bool canStart() const;
    bool canAccept() const;
    QString errorText() const;

    bool prepareFile(const QString& fileName);
    QStringList frameFileNames() const;
    QQuickImageProvider* createImageProvider() const;

    Q_INVOKABLE void browseFile();
    Q_INVOKABLE void start();
    Q_INVOKABLE void stop();
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void acceptFrames();
    Q_INVOKABLE void openFrame(int index);
    Q_INVOKABLE void removeFrames(const QVariantList& indices);

signals:
    void stateChanged();
    void framesAccepted(const QStringList& fileNames);
    void frameViewerRequested(int index);

private:
    void appendFrame(const QString& timeText, const QString& fileName, const QImage& image);
    void grabFinished(bool success);
    VideoGrabber::VideoEngine videoEngine() const;

    QWidget* DialogParent_;
    std::unique_ptr<VideoGrabber> Grabber_;
    QString FileName_;
    QStringList VideoEngines_;
    QString SelectedEngine_;
    std::shared_ptr<FrameGrabberImageStore> ImageStore_;
    std::unique_ptr<FrameGrabberFrameModel> Frames_;
    QString ErrorText_;
    int FrameCount_ = 10;
    bool Running_ = false;
    bool CancelRequested_ = false;
    quint64 NextFrameId_ = 0;
};

#endif
