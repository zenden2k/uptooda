#ifndef QIMAGEUPLOADER_GUI_IMAGEVIEWERWINDOW_H
#define QIMAGEUPLOADER_GUI_IMAGEVIEWERWINDOW_H

#include <QQuickView>

#include <memory>

#include "IImageViewerSource.h"

class ViewerImageProvider;
class QCloseEvent;

class ImageViewerWindow : public QQuickView {
    Q_OBJECT
    Q_PROPERTY(QString imageSource READ imageSource NOTIFY viewerStateChanged)
    Q_PROPERTY(QString fileName READ fileName NOTIFY viewerStateChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY viewerStateChanged)
    Q_PROPERTY(bool loading READ loading NOTIFY viewerStateChanged)
    Q_PROPERTY(bool hasNext READ hasNext NOTIFY viewerStateChanged)
    Q_PROPERTY(bool hasPrevious READ hasPrevious NOTIFY viewerStateChanged)

public:
    explicit ImageViewerWindow(QWindow* parent = nullptr);
    ~ImageViewerWindow() override;

    void setImageViewerSource(std::unique_ptr<IImageViewerSource> source);
    void open();

    QString imageSource() const;
    QString fileName() const;
    QString errorText() const;
    bool loading() const;
    bool hasNext() const;
    bool hasPrevious() const;

    Q_INVOKABLE void showNext();
    Q_INVOKABLE void showPrevious();
    Q_INVOKABLE void minimizeViewer();
    Q_INVOKABLE void toggleFullScreen();
    Q_INVOKABLE void closeViewer();

signals:
    void viewerStateChanged();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    void loadFile(const QString& fileName);
    void showViewer();

    std::unique_ptr<IImageViewerSource> Source_;
    ViewerImageProvider* ImageProvider_;
    QString ImageSource_;
    QString FileName_;
    QString ErrorText_;
    quint64 LoadRequest_ = 0;
    bool Loading_ = false;
    bool OpenWhenReady_ = false;
};

#endif
