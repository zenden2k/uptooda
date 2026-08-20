#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QThread>

#include <memory>

#include "Core/ProgramWindow.h"
#include "Core/QtServerIconCache.h"
#include "Core/UploadEngineList.h"
class UploadManager;
class UploadEngineManager;
class ImageViewerWindow;
class ThumbnailListModel;
class ScriptsManager;
class UploadSession;
class UploadSessionListWidget;
class UploadTask;

namespace Uptooda::Core::OutputGenerator {
struct UploadObject;
}
class LogWindow;
class QDragEnterEvent;
class QDragLeaveEvent;
class QDragMoveEvent;
class QDropEvent;
class QMimeData;
class QResizeEvent;
class QSystemTrayIcon;

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow, public IProgramWindow {
    Q_OBJECT

public:
    explicit MainWindow(CUploadEngineList*, LogWindow* logWindow, QWidget* parent = 0);
    ~MainWindow();

    bool eventFilter(QObject*, QEvent*);

    WindowHandle getHandle() override;
    WindowNativeHandle getNativeHandle() override;
    void setServersChanged(bool changed) override;
private slots:
    void updateView();

    void on_actionGrab_frames_triggered();
    void on_actionScreenshot_triggered();
    void on_actionAdd_files_triggered();
    void on_actionAboutProgram_triggered();
    void showCodes(UploadSession* session, UploadTask* task);
    void openImageViewer(UploadSession* session, UploadTask* task);
    void removeTask(UploadSession* session, UploadTask* task);
    void onCustomContextMenu(UploadSession* session, UploadTask* task, const QPoint& globalPosition);
    void onShowLog();
    void fillServerIcons();
    void onCopyDirectLinkTriggered(bool checked);
    void onCopyFilePathTriggered(bool checked);
    void clearPendingFiles();
    void removePendingFiles(const QList<int>& rows);
    void showUploadSettings();
    void startPendingUpload();

protected:
    bool addFileToList(QString fileName);
    bool addMultipleFilesToList(QStringList fileNames);
    void uploadTaskToUploadObject(UploadTask* task, Uptooda::Core::OutputGenerator::UploadObject& obj);
    void showCodes(const std::shared_ptr<UploadSession>& session, const std::shared_ptr<UploadTask>& task = { });
    void quitApp();
    void saveOptions();
    void closeEvent(QCloseEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dragLeaveEvent(QDragLeaveEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    std::shared_ptr<UploadSession> findSession(UploadSession* session) const;
    std::shared_ptr<UploadTask> findTask(UploadTask* task) const;
    UploadSessionListWidget* uploadSessionList() const;

private:
    std::unique_ptr<Ui::MainWindow> ui;
    std::unique_ptr<UploadManager> uploadManager_;
    std::unique_ptr<UploadEngineManager> uploadEngineManager_;
    std::unique_ptr<ScriptsManager> scriptsManager_;
    std::unique_ptr<ThumbnailListModel> pendingFilesModel_;
    std::unique_ptr<ImageViewerWindow> imageViewerWindow_;
    std::unique_ptr<QtServerIconCache> serverIconCache_;
    QSystemTrayIcon* systemTrayIcon_;
    LogWindow* logWindow_;
    CUploadEngineList* engineList_;
    QThread* iconsLoadingThread_ { };
    QAction* copyDirectLinkAction_ = nullptr;
    QAction* copyFilePathAction_ = nullptr;
    QWidget* dropHighlightOverlay_ = nullptr;
    bool dragContainsFiles_ = false;
};

#endif // MAINWINDOW_H
