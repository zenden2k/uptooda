#ifndef FRAMEGRABBERDLG_H
#define FRAMEGRABBERDLG_H

#include <memory>

#include <QDialog>
#include <QIcon>
#include "Core/Images/AbstractImage.h"
#include "Video/VideoGrabber.h"
class VideoGrabber;
class ThumbnailListModel;

namespace Ui {
class FrameGrabberDlg;
}

class FrameGrabberDlg : public QDialog {
    Q_OBJECT

public:
    explicit FrameGrabberDlg(QString fileName, QWidget* parent = 0);
    ~FrameGrabberDlg();

    void frameGrabbed(const std::string&, int64_t, const std::shared_ptr<AbstractImage>&);
    void getGrabbedFrames(QStringList& fileNames) const;
private slots:
    void on_grabButton_clicked();

    void on_browseButton_clicked();
    void onStopButtonClicked();
    void frameGrabbedSlot(QString timeStr, QString fileName, QIcon image);
    void grabFinishedSlot();
    void onFinished();

protected:
    void onGrabFinished();

private:
    Ui::FrameGrabberDlg* ui;
    std::unique_ptr<VideoGrabber> grabber_;
    std::unique_ptr<ThumbnailListModel> frameModel_;
    VideoGrabber::VideoEngine getVideoEngine() const;
    // QWidget interface
protected:
    void closeEvent(QCloseEvent* event);
};

#endif // FRAMEGRABBERDLG_H
