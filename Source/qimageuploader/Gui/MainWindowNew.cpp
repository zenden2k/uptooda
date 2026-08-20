#include "MainWindowNew.h"

#include <QAbstractListModel>
#include <QApplication>
#include <QClipboard>
#include <QCloseEvent>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QMenu>
#include <QMessageBox>
#include <QMimeDatabase>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickImageProvider>
#include <QQuickWidget>
#include <QSystemTrayIcon>
#include <QTemporaryFile>

#include <algorithm>

#include "AboutDialog.h"
#include "Core/AppRuntimeInfo.h"
#include "Core/BasicConstants.h"
#include "Core/CommonDefs.h"
#include "Core/Network/NetworkClientFactory.h"
#include "Core/OutputGenerator/AbstractOutputGenerator.h"
#include "Core/Scripting/ScriptsManager.h"
#include "Core/ServiceLocator.h"
#include "Core/Settings/BasicSettings.h"
#include "Core/Settings/CommonGuiSettings.h"
#include "Core/Settings/QtGuiSettings.h"
#include "Core/Upload/FileUploadTask.h"
#include "Core/Upload/UploadEngineManager.h"
#include "Core/Upload/UploadManager.h"
#include "Gui/FrameGrabberController.h"
#include "Gui/ImageViewerWindow.h"
#include "Gui/LogWindow.h"
#include "Gui/LoginDialog.h"
#include "Gui/RegionSelect.h"
#include "Gui/ResultsWindow.h"
#include "Gui/models/QmlUploadSessionModel.h"

using Uptooda::Core::OutputGenerator::UploadObject;

class PendingFilesModel final : public QAbstractListModel {
public:
    enum Roles { TIME_ROLE = Qt::UserRole + 1, SOURCE_ROLE };

    explicit PendingFilesModel(QObject* parent = nullptr) : QAbstractListModel(parent) { }

    int rowCount(const QModelIndex& parent = { }) const override { return parent.isValid() ? 0 : FileNames_.size(); }

    QVariant data(const QModelIndex& index, int role) const override {
        if (!index.isValid() || index.row() < 0 || index.row() >= FileNames_.size()) {
            return { };
        }
        const QString& fileName = FileNames_[index.row()];
        switch (role) {
        case TIME_ROLE:
            return QFileInfo(fileName).fileName();
        case SOURCE_ROLE:
            return QUrl::fromLocalFile(fileName);
        default:
            return { };
        }
    }

    QHash<int, QByteArray> roleNames() const override { return { { TIME_ROLE, "time" }, { SOURCE_ROLE, "source" } }; }

    void setFileNames(const QStringList& fileNames) {
        beginResetModel();
        FileNames_ = fileNames;
        endResetModel();
    }

private:
    QStringList FileNames_;
};

namespace {
class UploadSessionsImageSource final : public IImageViewerSource {
public:
    UploadSessionsImageSource(QmlUploadSessionModel* model, const QString& currentTaskId) : Model_(model) {
        if (Model_) {
            for (int session = 0; session < Model_->rowCount(); ++session) {
                const QVariantList tasks = tasksAt(session);
                for (int task = 0; task < tasks.size(); ++task) {
                    if (tasks[task].toMap().value(QStringLiteral("taskId")).toString() == currentTaskId) {
                        Current_ = { session, task };
                        return;
                    }
                }
            }
        }
    }

    QString currentFile() const override { return imageFileAt(Current_); }

    QString nextFile() override {
        const Position next = findImage(Current_, 1);
        if (next.isValid()) {
            Current_ = next;
        }
        return currentFile();
    }

    QString previousFile() override {
        const Position previous = findImage(Current_, -1);
        if (previous.isValid()) {
            Current_ = previous;
        }
        return currentFile();
    }

    bool hasNext() const override { return findImage(Current_, 1).isValid(); }

    bool hasPrevious() const override { return findImage(Current_, -1).isValid(); }

private:
    struct Position {
        int Session = -1;
        int Task = -1;

        bool isValid() const { return Session >= 0 && Task >= 0; }
    };

    QVariantList tasksAt(int session) const {
        if (!Model_ || session < 0 || session >= Model_->rowCount()) {
            return { };
        }
        return Model_->data(Model_->index(session, 0), QmlUploadSessionModel::TasksRole).toList();
    }

    QString imageFileAt(const Position& position) const {
        const QVariantList tasks = tasksAt(position.Session);
        if (position.Task < 0 || position.Task >= tasks.size()) {
            return { };
        }
        const QVariantMap task = tasks[position.Task].toMap();
        if (!task.value(QStringLiteral("isImage")).toBool()) {
            return { };
        }
        return task.value(QStringLiteral("filePath")).toString();
    }

    Position findImage(const Position& current, int direction) const {
        if (!Model_ || !current.isValid() || Model_->rowCount() == 0) {
            return { };
        }

        Position result = findImageLinear(current, direction);
        if (!result.isValid()) {
            const int edgeSession = direction > 0 ? 0 : Model_->rowCount() - 1;
            const int edgeTask = direction > 0 ? -1 : tasksAt(edgeSession).size();
            result = findImageLinear({ edgeSession, edgeTask }, direction);
        }

        if (result.Session == current.Session && result.Task == current.Task) {
            return { };
        }
        return result;
    }

    Position findImageLinear(const Position& current, int direction) const {
        if (!Model_ || current.Session < 0 || current.Session >= Model_->rowCount()) {
            return { };
        }
        for (int session = current.Session; session >= 0 && session < Model_->rowCount(); session += direction) {
            const QVariantList tasks = tasksAt(session);
            int task = direction > 0 ? 0 : tasks.size() - 1;
            if (session == current.Session) {
                task = current.Task + direction;
            }
            for (; task >= 0 && task < tasks.size(); task += direction) {
                const Position candidate { session, task };
                if (!imageFileAt(candidate).isEmpty()) {
                    return candidate;
                }
            }
        }
        return { };
    }

    QmlUploadSessionModel* Model_;
    Position Current_;
};

class FrameGrabberImageSource final : public IImageViewerSource {
public:
    FrameGrabberImageSource(QStringList fileNames, int currentIndex) :
        FileNames_(std::move(fileNames)), CurrentIndex_(currentIndex) { }

    QString currentFile() const override {
        return CurrentIndex_ >= 0 && CurrentIndex_ < FileNames_.size() ? FileNames_[CurrentIndex_] : QString { };
    }

    QString nextFile() override {
        if (!FileNames_.isEmpty()) {
            CurrentIndex_ = (CurrentIndex_ + 1) % FileNames_.size();
        }
        return currentFile();
    }

    QString previousFile() override {
        if (!FileNames_.isEmpty()) {
            CurrentIndex_ = (CurrentIndex_ - 1 + FileNames_.size()) % FileNames_.size();
        }
        return currentFile();
    }

    bool hasNext() const override { return FileNames_.size() > 1; }
    bool hasPrevious() const override { return FileNames_.size() > 1; }

private:
    QStringList FileNames_;
    int CurrentIndex_;
};
}

MainWindowNew::MainWindowNew(CUploadEngineList* engineList, LogWindow* logWindow, QWidget* parent) :
    QMainWindow(parent), EngineList_(engineList), LogWindow_(logWindow), QuickWidget_(new QQuickWidget(this)),
    SystemTrayIcon_(nullptr) {
    setWindowTitle(APP_NAME_A + tr(" (Qt Quick GUI)"));
    setWindowIcon(QIcon(QStringLiteral(":/res/icon_main.ico")));
    resize(1080, 720);
    setMinimumSize(820, 560);

    auto* serviceLocator = ServiceLocator::instance();
    auto* settings = serviceLocator->settings<QtGuiSettings>();
    serviceLocator->setProgramWindow(this);
    auto networkClientFactory = std::make_shared<NetworkClientFactory>();
    ScriptsManager_ = std::make_unique<ScriptsManager>(networkClientFactory);
    UploadEngineManager_
        = std::make_unique<UploadEngineManager>(engineList, serviceLocator->uploadErrorHandler(), networkClientFactory);
    UploadManager_
        = std::make_unique<UploadManager>(UploadEngineManager_.get(), ScriptsManager_.get(),
                                          serviceLocator->uploadErrorHandler(), networkClientFactory, settings, 3);

    const std::string dataDirectory = AppRuntimeInfo::instance()->dataDirectory();
    ServerIconCache_ = std::make_unique<QtServerIconCache>(engineList, dataDirectory + "Favicons/");
    serviceLocator->setServerIconCache(ServerIconCache_.get());
    UploadEngineManager_->setScriptsDirectory(dataDirectory + "/Scripts/");
    UploadSessionModel_ = std::make_unique<QmlUploadSessionModel>(UploadManager_.get(), this);
    PendingFilesModel_ = std::make_unique<PendingFilesModel>(this);
    FrameGrabberController_ = std::make_unique<FrameGrabberController>(this, this);
    connect(FrameGrabberController_.get(), &FrameGrabberController::framesAccepted, this,
            [this](const QStringList& fileNames) {
                if (fileNames.isEmpty()) {
                    return;
                }
                const int firstAddedIndex = addPendingFiles(fileNames);
                emit uploadSelectionRequested(firstAddedIndex, false);
            });
    connect(FrameGrabberController_.get(), &FrameGrabberController::frameViewerRequested, this, [this](int index) {
        auto source = std::make_unique<FrameGrabberImageSource>(FrameGrabberController_->frameFileNames(), index);
        if (source->currentFile().isEmpty()) {
            return;
        }
        if (!ImageViewerWindow_) {
            ImageViewerWindow_ = std::make_unique<ImageViewerWindow>();
            ImageViewerWindow_->setTransientParent(windowHandle());
        }
        ImageViewerWindow_->setImageViewerSource(std::move(source));
        ImageViewerWindow_->open();
    });

    QuickWidget_->setResizeMode(QQuickWidget::SizeRootObjectToView);
    QuickWidget_->setAcceptDrops(true);
    QuickWidget_->setClearColor(QColor(QStringLiteral("#f5f7fb")));
    QuickWidget_->engine()->addImageProvider(QStringLiteral("framegrabber"),
                                             FrameGrabberController_->createImageProvider());
    QuickWidget_->rootContext()->setContextProperty(QStringLiteral("mainWindowController"), this);
    QuickWidget_->rootContext()->setContextProperty(QStringLiteral("uploadSessions"), UploadSessionModel_.get());
    QuickWidget_->rootContext()->setContextProperty(QStringLiteral("frameGrabberController"),
                                                    FrameGrabberController_.get());
    QuickWidget_->setSource(QUrl(QStringLiteral("qrc:/qt/qml/Uptooda/Ui/MainWindowNew.qml")));
    setCentralWidget(QuickWidget_);

    connect(qApp, &QGuiApplication::focusWindowChanged, this, [this] { emit shortcutScopeActiveChanged(); });

    auto* trayMenu = new QMenu(this);
    trayMenu->addAction(tr("Exit"), this, &MainWindowNew::quitApp);
    SystemTrayIcon_ = new QSystemTrayIcon(QIcon(QStringLiteral(":/res/icon_main.ico")), this);
    SystemTrayIcon_->setContextMenu(trayMenu);
    connect(SystemTrayIcon_, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            showNormal();
            raise();
            activateWindow();
        }
    });
    SystemTrayIcon_->show();
}

MainWindowNew::~MainWindowNew() {
    UploadSessionModel_->detach();
    UploadManager_.reset();
}

QVariantList MainWindowNew::imageServers() const { return buildServerList(CUploadEngineData::TypeImageServer); }

QVariantList MainWindowNew::fileServers() const { return buildServerList(CUploadEngineData::TypeFileServer); }

QAbstractItemModel* MainWindowNew::pendingFiles() const { return PendingFilesModel_.get(); }

int MainWindowNew::pendingFileCount() const { return PendingFiles_.size(); }

QString MainWindowNew::defaultImageServer() const {
    return U2Q(ServiceLocator::instance()->settings<QtGuiSettings>()->imageServer.getByIndex(0).serverName());
}

QString MainWindowNew::defaultImageAccount() const {
    return U2Q(ServiceLocator::instance()->settings<QtGuiSettings>()->imageServer.getByIndex(0).profileName());
}

QString MainWindowNew::defaultFileServer() const {
    return U2Q(ServiceLocator::instance()->settings<QtGuiSettings>()->fileServer.getByIndex(0).serverName());
}

QString MainWindowNew::defaultFileAccount() const {
    return U2Q(ServiceLocator::instance()->settings<QtGuiSettings>()->fileServer.getByIndex(0).profileName());
}

bool MainWindowNew::shortcutScopeActive() const { return QApplication::activeWindow() == this; }

void MainWindowNew::chooseFiles() {
    QFileDialog dialog(this, tr("Open files"), QString(), tr("All files (*.*)"));
    dialog.setFileMode(QFileDialog::ExistingFiles);
    if (dialog.exec() == QDialog::Accepted && !dialog.selectedFiles().isEmpty()) {
        const int firstAddedIndex = addPendingFiles(dialog.selectedFiles());
        emit uploadSelectionRequested(firstAddedIndex, false);
    }
}

void MainWindowNew::importVideo() {
    QFileDialog dialog(this, tr("Open video file"), QString(),
                       tr("Video files (*.avi *.mkv *.mov *.mp4 *.mpeg *.mpg *.webm *.wmv);;All files (*.*)"));
    dialog.setFileMode(QFileDialog::ExistingFile);
    if (dialog.exec() == QDialog::Accepted && !dialog.selectedFiles().isEmpty()
        && FrameGrabberController_->prepareFile(dialog.selectedFiles().first())) {
        emit videoImportRequested();
    }
}

void MainWindowNew::addDroppedFiles(const QVariantList& urls) {
    QStringList fileNames;
    for (const QVariant& value : urls) {
        const QUrl url = value.toUrl();
        if (!url.isLocalFile()) {
            continue;
        }
        const QFileInfo fileInfo(url.toLocalFile());
        if (fileInfo.exists() && fileInfo.isFile() && !fileNames.contains(fileInfo.absoluteFilePath())) {
            fileNames.append(fileInfo.absoluteFilePath());
        }
    }
    if (!fileNames.isEmpty()) {
        const int firstAddedIndex = addPendingFiles(fileNames);
        emit uploadSelectionRequested(firstAddedIndex, false);
    }
}

void MainWindowNew::clearPendingFiles() {
    PendingFilesUploaded_ = false;
    setPendingFiles({ });
}

void MainWindowNew::removePendingFiles(const QVariantList& indices) {
    QList<int> rows;
    rows.reserve(indices.size());
    for (const QVariant& index : indices) {
        rows.append(index.toInt());
    }
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    rows.erase(std::unique(rows.begin(), rows.end()), rows.end());
    for (const int row : rows) {
        if (row >= 0 && row < PendingFiles_.size()) {
            PendingFiles_.removeAt(row);
        }
    }
    PendingFilesModel_->setFileNames(PendingFiles_);
    emit pendingFilesChanged();
}

void MainWindowNew::openPendingFile(int index) {
    auto source = std::make_unique<FrameGrabberImageSource>(PendingFiles_, index);
    if (source->currentFile().isEmpty()) {
        return;
    }
    if (!ImageViewerWindow_) {
        ImageViewerWindow_ = std::make_unique<ImageViewerWindow>();
        ImageViewerWindow_->setTransientParent(windowHandle());
    }
    ImageViewerWindow_->setImageViewerSource(std::move(source));
    ImageViewerWindow_->open();
}

void MainWindowNew::reusePendingFiles() { PendingFilesUploaded_ = false; }

void MainWindowNew::openImageViewer(const QString& sessionId, const QString& taskId) {
    Q_UNUSED(sessionId);
    auto source = std::make_unique<UploadSessionsImageSource>(UploadSessionModel_.get(), taskId);
    if (source->currentFile().isEmpty()) {
        return;
    }
    if (!ImageViewerWindow_) {
        ImageViewerWindow_ = std::make_unique<ImageViewerWindow>();
        ImageViewerWindow_->setTransientParent(windowHandle());
    }
    ImageViewerWindow_->setImageViewerSource(std::move(source));
    ImageViewerWindow_->open();
}

void MainWindowNew::captureScreenshot() {
    CScreenCaptureEngine captureEngine;
    captureEngine.setDelay(450);
    hide();
    captureEngine.captureScreen();
    std::unique_ptr<RegionSelect> selector = std::make_unique<RegionSelect>(nullptr, captureEngine.capturedBitmap());
    if (selector->exec() != QDialog::Accepted) {
        show();
        return;
    }
    std::unique_ptr<CScreenshotRegion> region(selector->selectedRegion());
    if (!region) {
        region = std::make_unique<CActiveWindowRegion>();
    }
    captureEngine.setSource(*captureEngine.capturedBitmap());
    if (!captureEngine.captureRegion(region.get())) {
        show();
        return;
    }
    QTemporaryFile file(U2Q(AppRuntimeInfo::instance()->tempDirectory()) + QStringLiteral("/screenshot_XXXXXX.png"));
    file.setAutoRemove(false);
    if (file.open()) {
        const QString fileName = file.fileName();
        file.close();
        if (captureEngine.capturedBitmap()->save(fileName)) {
            const int firstAddedIndex = addPendingFiles({ fileName });
            emit uploadSelectionRequested(firstAddedIndex, true);
        }
    }
    show();
}

bool MainWindowNew::confirmUpload(const QString& imageServer, const QString& imageAccount, const QString& fileServer,
                                  const QString& fileAccount) {
    const bool uploadStarted = addFiles(PendingFiles_, imageServer, imageAccount, fileServer, fileAccount);
    if (uploadStarted) {
        auto* settings = ServiceLocator::instance()->settings<QtGuiSettings>();
        ServerProfile imageProfile(Q2U(imageServer));
        imageProfile.setProfileName(Q2U(imageAccount));
        settings->imageServer = imageProfile;
        ServerProfile fileProfile(Q2U(fileServer));
        fileProfile.setProfileName(Q2U(fileAccount));
        settings->fileServer = fileProfile;
        PendingFilesUploaded_ = true;
    }
    return uploadStarted;
}

void MainWindowNew::cancelUpload() { clearPendingFiles(); }

QString MainWindowNew::addAccount(const QString& serverName) {
    ServerProfile profile(Q2U(serverName));
    LoginDialog dialog(profile, true, this);
    if (dialog.exec() != QDialog::Accepted) {
        return { };
    }
    emit serverListsChanged();
    return dialog.accountName();
}

void MainWindowNew::showCodes(const QString& sessionId, const QString& taskId) {
    std::vector<UploadObject> objects;
    auto appendTask = [&objects](const std::shared_ptr<UploadTask>& task) {
        if (task && task->uploadSuccess(false)) {
            UploadObject object;
            object.fillFromUploadResult(task->uploadResult(), task.get());
            objects.push_back(object);
        }
    };
    if (taskId.isEmpty()) {
        auto session = UploadSessionModel_->findSession(sessionId);
        if (session) {
            for (int i = 0; i < session->taskCount(); ++i) {
                appendTask(session->getTask(i));
            }
        }
    } else {
        appendTask(findTask(sessionId, taskId));
    }
    ResultsWindow dialog(objects, this);
    dialog.exec();
}

void MainWindowNew::copyDirectLink(const QString& sessionId, const QString& taskId) {
    if (auto task = findTask(sessionId, taskId)) {
        QApplication::clipboard()->setText(U2Q(task->uploadResult()->getDirectUrl()));
    }
}

void MainWindowNew::copyViewLink(const QString& sessionId, const QString& taskId) {
    if (auto task = findTask(sessionId, taskId)) {
        QApplication::clipboard()->setText(U2Q(task->uploadResult()->getDownloadUrl()));
    }
}

void MainWindowNew::copyFilePath(const QString& sessionId, const QString& taskId) {
    if (auto task = std::dynamic_pointer_cast<FileUploadTask>(findTask(sessionId, taskId))) {
        QApplication::clipboard()->setText(QDir::toNativeSeparators(U2Q(task->getFileName())));
    }
}

void MainWindowNew::openTaskUrl(const QString& sessionId, const QString& taskId) {
    if (auto task = findTask(sessionId, taskId)) {
        const auto* result = task->uploadResult();
        const QString url = U2Q(result->getDirectUrl().empty() ? result->getDownloadUrl() : result->getDirectUrl());
        if (!url.isEmpty()) {
            QDesktopServices::openUrl(QUrl(url));
        }
    }
}

void MainWindowNew::openTaskFile(const QString& sessionId, const QString& taskId) {
    if (auto task = std::dynamic_pointer_cast<FileUploadTask>(findTask(sessionId, taskId))) {
        QDesktopServices::openUrl(QUrl::fromLocalFile(U2Q(task->getFileName())));
    }
}

void MainWindowNew::removeTask(const QString& sessionId, const QString& taskId) {
    if (auto task = findTask(sessionId, taskId)) {
        task->stop(true);
        UploadSessionModel_->hideTask(taskId);
    }
}

void MainWindowNew::removeSession(const QString& sessionId) {
    if (auto session = UploadSessionModel_->findSession(sessionId)) {
        session->stop(true);
        UploadSessionModel_->hideSession(sessionId);
    }
}

void MainWindowNew::retrySession(const QString& sessionId) {
    if (auto session = UploadSessionModel_->findSession(sessionId)) {
        UploadManager_->retrySession(session);
        UploadSessionModel_->refresh();
    }
}

void MainWindowNew::showLog() {
    LogWindow_->show();
    LogWindow_->activateWindow();
}

void MainWindowNew::showAbout() {
    AboutDialog dialog(this);
    dialog.exec();
}

void MainWindowNew::quitApp() {
    close();
    QApplication::quit();
}

WindowHandle MainWindowNew::getHandle() { return this; }

WindowNativeHandle MainWindowNew::getNativeHandle() { return reinterpret_cast<WindowNativeHandle>(effectiveWinId()); }

void MainWindowNew::setServersChanged(bool) { emit serverListsChanged(); }

void MainWindowNew::closeEvent(QCloseEvent* event) { QMainWindow::closeEvent(event); }

QVariantList MainWindowNew::buildServerList(int serverTypeMask) const {
    QVariantList result;
    const auto* settings = ServiceLocator::instance()->basicSettings();
    const QString iconsDirectory = U2Q(AppRuntimeInfo::instance()->dataDirectory()) + QStringLiteral("Favicons/");
    for (int i = 0; i < EngineList_->count(); ++i) {
        const auto* server = EngineList_->byIndex(i);
        if (!server->hasType(static_cast<CUploadEngineData::ServerType>(serverTypeMask))) {
            continue;
        }
        QVariantMap item;
        item[QStringLiteral("name")] = U2Q(server->Name);
        item[QStringLiteral("displayName")] = U2Q(CUploadEngineListBase::getServerDisplayName(server));
        const QString iconPath = iconsDirectory + U2Q(server->Name).toLower() + QStringLiteral(".ico");
        item[QStringLiteral("icon")] = QFileInfo::exists(iconPath) ? QUrl::fromLocalFile(iconPath).toString()
                                                                   : QStringLiteral("qrc:/res/server.png");
        QVariantList accounts;
        accounts.append(QVariantMap { { QStringLiteral("name"), QString() },
                                      { QStringLiteral("displayName"), tr("<without account>") } });
        auto serverIt = settings->ServersSettings.find(server->Name);
        if (serverIt != settings->ServersSettings.end()) {
            for (const auto& account : serverIt->second) {
                if (!account.first.empty()) {
                    accounts.append(QVariantMap { { QStringLiteral("name"), U2Q(account.first) },
                                                  { QStringLiteral("displayName"), U2Q(account.first) } });
                }
            }
        }
        item[QStringLiteral("accounts")] = accounts;
        result.append(item);
    }
    return result;
}

bool MainWindowNew::addFiles(const QStringList& fileNames, const QString& imageServer, const QString& imageAccount,
                             const QString& fileServer, const QString& fileAccount) {
    if (fileNames.isEmpty() || imageServer.isEmpty() || fileServer.isEmpty()) {
        return false;
    }
    auto session = std::make_shared<UploadSession>();
    QMimeDatabase mimeDatabase;
    for (const QString& fileName : fileNames) {
        auto task = std::make_shared<FileUploadTask>(Q2U(fileName), IuCoreUtils::ExtractFileName(Q2U(fileName)));
        const bool isImage = mimeDatabase.mimeTypeForFile(fileName, QMimeDatabase::MatchContent)
                                 .name()
                                 .startsWith(QStringLiteral("image/"));
        const QString selectedServer = isImage ? imageServer : fileServer;
        const QString selectedAccount = isImage ? imageAccount : fileAccount;
        ServerProfile profile(Q2U(selectedServer));
        profile.setProfileName(Q2U(selectedAccount));
        if (profile.useDefaultSettings()) {
            profile.setImageUploadParams(
                ServiceLocator::instance()->settings<CommonGuiSettings>()->DefaultImageUploadParams);
        }
        task->setIsImage(isImage);
        task->setServerProfile(profile);
        task->setIndex(session->taskCount());
        session->addTask(task);
    }
    UploadManager_->addSession(session);
    return true;
}

void MainWindowNew::setPendingFiles(const QStringList& fileNames) {
    PendingFiles_ = fileNames;
    PendingFilesModel_->setFileNames(PendingFiles_);
    emit pendingFilesChanged();
}

int MainWindowNew::addPendingFiles(const QStringList& fileNames) {
    QStringList combined = PendingFilesUploaded_ ? QStringList { } : PendingFiles_;
    PendingFilesUploaded_ = false;
    int firstAddedIndex = -1;
    for (const QString& fileName : fileNames) {
        const QString absoluteFileName = QFileInfo(fileName).absoluteFilePath();
        if (!absoluteFileName.isEmpty() && !combined.contains(absoluteFileName)) {
            if (firstAddedIndex < 0) {
                firstAddedIndex = combined.size();
            }
            combined.append(absoluteFileName);
        }
    }
    setPendingFiles(combined);
    return firstAddedIndex;
}

std::shared_ptr<UploadTask> MainWindowNew::findTask(const QString& sessionId, const QString& taskId) const {
    return UploadSessionModel_->findTask(sessionId, taskId);
}
