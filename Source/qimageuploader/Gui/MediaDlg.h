#ifndef MEDIADLG_H
#define MEDIADLG_H

#include <cstdint>
#include <memory>

#include <QDialog>
#include <QIcon>

#include "Core/Images/AbstractImage.h"
#include "Video/VideoGrabber.h"

class QCloseEvent;
class QDragEnterEvent;
class QDragLeaveEvent;
class QDragMoveEvent;
class QDropEvent;
class QResizeEvent;
class MediaDropHighlight;
class QtImageGenerator;
class ThumbnailListModel;
class VideoGrabber;

namespace Ui {
class MediaDlg;
}

class MediaDlg : public QDialog {
    Q_OBJECT

public:
    explicit MediaDlg(QString fileName, QWidget* parent = nullptr, bool showMediaInfo = false);
    ~MediaDlg() override;

    void frameGrabbed(const std::string& timeString, int64_t time, const std::shared_ptr<AbstractImage>& image);
    void getGrabbedFrames(QStringList& fileNames) const;

private slots:
    void on_grabButton_clicked();
    void on_browseButton_clicked();
    void onStopButtonClicked();
    void frameGrabbedSlot(const QString& timeString, const QString& fileName, const QImage& image);
    void grabFinishedSlot(bool success);
    void onFinished();
    void onCurrentTabChanged(int index);
    void updateMediaInfoText();
    void reloadMediaInfo();
    void copyMediaInfo();
    void createMosaic();
    void cancelMosaic();

protected:
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void onGrabFinished(bool success);
    void reject() override;

private:
    static constexpr int EXTRACT_FRAMES_TAB = 0;
    static constexpr int MEDIA_INFO_TAB = 1;

    static bool isVideoFile(const QString& fileName);
    void setFileName(const QString& fileName);
    void startMediaInfoLoad();
    void updateMosaicControls();
    void mosaicFinished(bool success, bool canceled, const QString& outputFileName, const QString& errorMessage);
    VideoGrabber::VideoEngine getVideoEngine() const;

    Ui::MediaDlg* ui;
    std::unique_ptr<VideoGrabber> grabber_;
    std::unique_ptr<ThumbnailListModel> frameModel_;
    QtImageGenerator* mosaicGenerator_ = nullptr;
    MediaDropHighlight* dropHighlightOverlay_ = nullptr;
    QString fileName_;
    QString mediaInfoSummary_;
    QString mediaInfoFull_;
    int grabbedFramesCount_ = 0;
    uint64_t mediaInfoRequest_ = 0;
    bool mediaInfoLoading_ = false;
    bool mediaInfoLoaded_ = false;
    bool grabInProgress_ = false;
    bool grabCanceled_ = false;
    bool mosaicInProgress_ = false;
    bool dragContainsFiles_ = false;
};

#endif // MEDIADLG_H
