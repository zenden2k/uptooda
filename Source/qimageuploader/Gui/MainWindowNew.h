#ifndef QIMAGEUPLOADER_GUI_MAINWINDOWNEW_H
#define QIMAGEUPLOADER_GUI_MAINWINDOWNEW_H

#include <QMainWindow>
#include <QVariantList>

#include <memory>

#include "Core/ProgramWindow.h"
#include "Core/QtServerIconCache.h"
#include "Core/UploadEngineList.h"

class LogWindow;
class FrameGrabberController;
class ImageViewerWindow;
class QmlUploadSessionModel;
class QQuickWidget;
class QSystemTrayIcon;
class ScriptsManager;
class UploadEngineManager;
class UploadManager;
class UploadTask;

class MainWindowNew : public QMainWindow, public IProgramWindow {
    Q_OBJECT
    Q_PROPERTY(QVariantList imageServers READ imageServers NOTIFY serverListsChanged)
    Q_PROPERTY(QVariantList fileServers READ fileServers NOTIFY serverListsChanged)
    Q_PROPERTY(int pendingFileCount READ pendingFileCount NOTIFY pendingFilesChanged)
    Q_PROPERTY(QString defaultImageServer READ defaultImageServer CONSTANT)
    Q_PROPERTY(QString defaultImageAccount READ defaultImageAccount CONSTANT)
    Q_PROPERTY(QString defaultFileServer READ defaultFileServer CONSTANT)
    Q_PROPERTY(QString defaultFileAccount READ defaultFileAccount CONSTANT)

public:
    explicit MainWindowNew(CUploadEngineList* engineList, LogWindow* logWindow, QWidget* parent = nullptr);
    ~MainWindowNew() override;

    QVariantList imageServers() const;
    QVariantList fileServers() const;
    int pendingFileCount() const;
    QString defaultImageServer() const;
    QString defaultImageAccount() const;
    QString defaultFileServer() const;
    QString defaultFileAccount() const;

    Q_INVOKABLE void chooseFiles();
    Q_INVOKABLE void importVideo();
    Q_INVOKABLE void addDroppedFiles(const QVariantList& urls);
    Q_INVOKABLE void openImageViewer(const QString& sessionId, const QString& taskId);
    Q_INVOKABLE void captureScreenshot();
    Q_INVOKABLE void confirmUpload(const QString& imageServer, const QString& imageAccount, const QString& fileServer,
                                   const QString& fileAccount);
    Q_INVOKABLE void cancelUpload();
    Q_INVOKABLE QString addAccount(const QString& serverName);
    Q_INVOKABLE void showCodes(const QString& sessionId, const QString& taskId = { });
    Q_INVOKABLE void copyDirectLink(const QString& sessionId, const QString& taskId);
    Q_INVOKABLE void copyViewLink(const QString& sessionId, const QString& taskId);
    Q_INVOKABLE void copyFilePath(const QString& sessionId, const QString& taskId);
    Q_INVOKABLE void openTaskUrl(const QString& sessionId, const QString& taskId);
    Q_INVOKABLE void openTaskFile(const QString& sessionId, const QString& taskId);
    Q_INVOKABLE void removeTask(const QString& sessionId, const QString& taskId);
    Q_INVOKABLE void removeSession(const QString& sessionId);
    Q_INVOKABLE void retrySession(const QString& sessionId);
    Q_INVOKABLE void showLog();
    Q_INVOKABLE void showAbout();
    Q_INVOKABLE void quitApp();

    WindowHandle getHandle() override;
    WindowNativeHandle getNativeHandle() override;
    void setServersChanged(bool changed) override;

signals:
    void pendingFilesChanged();
    void serverListsChanged();
    void uploadSelectionRequested();
    void videoImportRequested();

protected:
    void closeEvent(QCloseEvent* event) override;

private:
    QVariantList buildServerList(int serverTypeMask) const;
    bool addFiles(const QStringList& fileNames, const QString& imageServer, const QString& imageAccount,
                  const QString& fileServer, const QString& fileAccount);
    void setPendingFiles(const QStringList& fileNames);
    std::shared_ptr<UploadTask> findTask(const QString& sessionId, const QString& taskId) const;

    CUploadEngineList* EngineList_;
    LogWindow* LogWindow_;
    QQuickWidget* QuickWidget_;
    std::unique_ptr<ScriptsManager> ScriptsManager_;
    std::unique_ptr<UploadEngineManager> UploadEngineManager_;
    std::unique_ptr<UploadManager> UploadManager_;
    std::unique_ptr<QtServerIconCache> ServerIconCache_;
    std::unique_ptr<QmlUploadSessionModel> UploadSessionModel_;
    std::unique_ptr<FrameGrabberController> FrameGrabberController_;
    std::unique_ptr<ImageViewerWindow> ImageViewerWindow_;
    QSystemTrayIcon* SystemTrayIcon_;
    QStringList PendingFiles_;
};

#endif
