#ifndef QIMAGEUPLOADER_GUI_IMAGEVIEWERWINDOW_H
#define QIMAGEUPLOADER_GUI_IMAGEVIEWERWINDOW_H

#include <QImage>
#include <QWidget>

#include <memory>

#include "IImageViewerSource.h"

class QAbstractButton;
class QLabel;
class QMovie;
class QScreen;
class QTimer;
class QWindow;
class ViewerBusyIndicator;

class ImageViewerWindow : public QWidget {
    Q_OBJECT

public:
    explicit ImageViewerWindow(QWidget* parent = nullptr);
    ~ImageViewerWindow() override;

    static bool isSupportedImageFile(const QString& fileName);
    void setImageViewerSource(std::unique_ptr<IImageViewerSource> source);
    void setTransientParent(QWindow* parent);
    void open();

public slots:
    void showNext();
    void showPrevious();
    void minimizeViewer();
    void toggleFullScreen();
    void closeViewer();

protected:
    void closeEvent(QCloseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    QString findImageFile(bool forward, bool includeCurrent);
    void loadFile(const QString& fileName);
    void showViewer();
    void setViewerGeometry(const QRect& geometry);
    void updateControls();
    QScreen* viewerScreen() const;
    QRect imageRect() const;

    std::unique_ptr<IImageViewerSource> source_;
    std::unique_ptr<QMovie> movie_;
    QImage image_;
    QLabel* errorLabel_ = nullptr;
    QLabel* fileNameLabel_ = nullptr;
    ViewerBusyIndicator* loadingIndicator_ = nullptr;
    QTimer* loadingIndicatorDelayTimer_ = nullptr;
    QAbstractButton* previousButton_ = nullptr;
    QAbstractButton* nextButton_ = nullptr;
    QAbstractButton* minimizeButton_ = nullptr;
    QAbstractButton* fullScreenButton_ = nullptr;
    QAbstractButton* closeButton_ = nullptr;
    QString fileName_;
    quint64 loadRequest_ = 0;
    bool loading_ = false;
    bool openWhenReady_ = false;
    bool fillsScreen_ = true;
};

#endif
