#include "MainWindow.h"

#include "ui_MainWindow.h"

#include <QClipboard>
#include <QCoreApplication>
#include <QDebug>
#include <QDesktopServices>
#include <QDragEnterEvent>
#include <QDragLeaveEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QFileDialog>
#include <QFileInfo>
#include <QMessageBox>
#include <QMimeData>
#include <QMimeDatabase>
#include <QPainter>
#include <QResizeEvent>
#include <QSystemTrayIcon>
#include <QTemporaryFile>
#include <QThread>
#include <QTimer>
#include <QToolButton>
#include <QWidgetAction>
#include <vector>

#include "AboutDialog.h"
#include "Core/AppRuntimeInfo.h"
#include "Core/BasicConstants.h"
#include "Core/CommonDefs.h"
#include "Core/Network/NetworkClientFactory.h"
#include "Core/OutputGenerator/AbstractOutputGenerator.h"
#include "Core/QtServerIconCache.h"
#include "Core/Scripting/ScriptsManager.h"
#include "Core/ServiceLocator.h"
#include "Core/Settings/QtGuiSettings.h"
#include "Core/Upload/FileUploadTask.h"
#include "Core/Upload/UploadEngineManager.h"
#include "Core/Upload/UploadManager.h"
#include "Core/Video/VideoUtils.h"
#include "Gui/ImageViewerWindow.h"
#include "Gui/LogWindow.h"
#include "Gui/MediaDlg.h"
#include "Gui/RegionSelect.h"
#include "Gui/VirtualFileDrop.h"
#include "Gui/controls/MainWindowTabsWidget.h"
#include "Gui/controls/UploadSessionListWidget.h"
#include "Gui/models/ThumbnailListModel.h"
#include "ResultsWindow.h"
#include "controls/AddedFilesTabWidget.h"
#include "controls/UploadSettingsTabWidget.h"
#include "controls/UploadsTabWidget.h"

using namespace Uptooda::Core::OutputGenerator;

class FileDropHighlight final : public QWidget {
public:
    enum class Action { AddFiles, ExtractFrames, FileInformation };

    explicit FileDropHighlight(QWidget* parent = nullptr) : QWidget(parent) { }

    void setMediaFile(bool video, bool audio) {
        video_ = video;
        media_ = video || audio;
        hoveredAction_ = Action::AddFiles;
        update();
    }

    void setDragPosition(const QPoint& position) {
        const Action action = actionAt(position);
        if (hoveredAction_ != action) {
            hoveredAction_ = action;
            update();
        }
    }

    Action actionAt(const QPoint& position) const {
        if (video_ && extractFramesRect().contains(position)) {
            return Action::ExtractFrames;
        }
        if (media_ && fileInformationRect().contains(position)) {
            return Action::FileInformation;
        }
        return Action::AddFiles;
    }

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter painter(this);
        painter.setRenderHint(QPainter::Antialiasing);
        painter.fillRect(rect(), QColor(232, 246, 255, 235));

        const QRectF borderRect = QRectF(rect()).adjusted(10.5, 10.5, -10.5, -10.5);
        QPen borderPen(QColor(QStringLiteral("#399bd8")), 3, Qt::DashLine);
        borderPen.setDashPattern({ 7, 5 });
        painter.setPen(borderPen);
        painter.setBrush(Qt::NoBrush);
        painter.drawRoundedRect(borderRect, 12, 12);

        QFont font = painter.font();
        font.setPointSize(qMax(20, font.pointSize() + 10));
        font.setBold(true);
        painter.setFont(font);
        painter.setPen(QColor(QStringLiteral("#197db8")));
        painter.drawText(rect().adjusted(24, 24, -24, -24), Qt::AlignCenter | Qt::TextWordWrap,
                         QCoreApplication::translate("MainWindow", "Drop files to add"));

        if (video_) {
            drawAction(painter, extractFramesRect(), QCoreApplication::translate("MainWindow", "Extract frames"),
                       Action::ExtractFrames);
        }
        if (media_) {
            drawAction(painter, fileInformationRect(), QCoreApplication::translate("MainWindow", "File information"),
                       Action::FileInformation);
        }
    }

private:
    static constexpr int ACTION_WIDTH = 220;
    static constexpr int ACTION_HEIGHT = 62;
    static constexpr int ACTION_GAP = 14;
    static constexpr int ACTION_TOP = 28;

    QRect extractFramesRect() const {
        if (!video_) {
            return { };
        }
        const int totalWidth = media_ ? ACTION_WIDTH * 2 + ACTION_GAP : ACTION_WIDTH;
        return { (width() - totalWidth) / 2, ACTION_TOP, ACTION_WIDTH, ACTION_HEIGHT };
    }

    QRect fileInformationRect() const {
        if (!media_) {
            return { };
        }
        if (!video_) {
            return { (width() - ACTION_WIDTH) / 2, ACTION_TOP, ACTION_WIDTH, ACTION_HEIGHT };
        }
        const QRect framesRect = extractFramesRect();
        return { framesRect.right() + 1 + ACTION_GAP, ACTION_TOP, ACTION_WIDTH, ACTION_HEIGHT };
    }

    void drawAction(QPainter& painter, const QRect& actionRect, const QString& text, Action action) const {
        const bool hovered = hoveredAction_ == action;
        painter.setBrush(hovered ? QColor(QStringLiteral("#cceafb")) : QColor(QStringLiteral("#ffffff")));
        QPen borderPen(QColor(QStringLiteral("#399bd8")), hovered ? 2.0 : 1.5, Qt::DashLine);
        borderPen.setDashPattern({ 5, 4 });
        painter.setPen(borderPen);
        painter.drawRoundedRect(actionRect, 11, 11);

        QFont font = QWidget::font();
        font.setPointSize(qMax(9, font.pointSize() - 1));
        font.setBold(true);
        painter.setFont(font);
        painter.setPen(QColor(QStringLiteral("#197db8")));
        painter.drawText(actionRect.adjusted(12, 8, -12, -8), Qt::AlignCenter | Qt::TextWordWrap, text);
    }

    Action hoveredAction_ = Action::AddFiles;
    bool video_ = false;
    bool media_ = false;
};

namespace {

bool IsFileOfType(const QString& fileName, const std::set<std::string>& extensions) {
    return extensions.find(QFileInfo(fileName).suffix().toLower().toStdString()) != extensions.end();
}

QStringList LocalFilesFromMimeData(const QMimeData* mimeData) {
    QStringList result;
    if (!mimeData || !mimeData->hasUrls()) {
        return result;
    }
    for (const QUrl& url : mimeData->urls()) {
        if (!url.isLocalFile()) {
            continue;
        }
        const QString fileName = QFileInfo(url.toLocalFile()).absoluteFilePath();
        if (QFileInfo(fileName).isFile()) {
            result.append(fileName);
        }
    }
    return result;
}

bool IsThumbnailListInternalDrag(const QMimeData* mimeData) {
    return mimeData && mimeData->hasFormat(ThumbnailListModel::internalMimeType());
}

} // namespace

MainWindow::MainWindow(CUploadEngineList* engineList, LogWindow* logWindow, QWidget* parent) :
    QMainWindow(parent), ui(new Ui::MainWindow), logWindow_(logWindow) {
    ui->setupUi(this);
    setMinimumSize(820, 560);
    ui->verticalLayout->setContentsMargins(0, 0, 0, 0);
    ui->verticalLayout->setSpacing(0);
    setAcceptDrops(true);

    dropHighlightOverlay_ = new FileDropHighlight(this);
    dropHighlightOverlay_->setAttribute(Qt::WA_TransparentForMouseEvents);
    dropHighlightOverlay_->setGeometry(rect());
    dropHighlightOverlay_->hide();

    ui->menuBar->setFixedHeight(40);

    auto* fileMenu = ui->menuBar->addMenu(tr("&File"));
    fileMenu->addAction(ui->actionAdd_files);
    fileMenu->addAction(ui->actionGrab_frames);
    fileMenu->addSeparator();
    QAction* exitAction = fileMenu->addAction(tr("Exit"));
    connect(exitAction, &QAction::triggered, this, &MainWindow::quitApp);

    auto* toolsMenu = ui->menuBar->addMenu(tr("&Tools"));
    toolsMenu->addAction(ui->actionScreenshot);
    toolsMenu->addAction(ui->actionMedia_info);
    QAction* showLogAction = toolsMenu->addAction(tr("Show log"));
    connect(showLogAction, &QAction::triggered, this, &MainWindow::onShowLog);

    auto* helpMenu = ui->menuBar->addMenu(tr("&Help"));
    helpMenu->addAction(ui->actionAboutProgram);
    ui->actionAdd_files->setIconVisibleInMenu(false);
    ui->actionGrab_frames->setIconVisibleInMenu(false);
    ui->actionMedia_info->setIconVisibleInMenu(false);
    ui->actionScreenshot->setIconVisibleInMenu(false);
    ui->actionAboutProgram->setIconVisibleInMenu(false);

    ui->mainToolBar->setFixedHeight(50);
    ui->mainToolBar->setIconSize(QSize(20, 20));
    const QList<QAction*> toolbarActions = ui->mainToolBar->actions();
    for (QAction* action : toolbarActions) {
        if (auto* button = qobject_cast<QToolButton*>(ui->mainToolBar->widgetForAction(action))) {
            button->setText(QStringLiteral("\u00a0\u00a0") + action->text());
        }
    }
    for (int index = 1; index < toolbarActions.size(); ++index) {
        auto* spacerAction = new QWidgetAction(ui->mainToolBar);
        auto* spacer = new QWidget(ui->mainToolBar);
        spacer->setFixedWidth(14);
        spacerAction->setDefaultWidget(spacer);
        ui->mainToolBar->insertAction(toolbarActions[index], spacerAction);
    }
    setWindowTitle(APP_NAME_A + QStringLiteral(" (Qt GUI)"));
    auto* serviceLocator = ServiceLocator::instance();
    auto* settings = serviceLocator->settings<QtGuiSettings>();
    engineList_ = engineList;
    serviceLocator->setProgramWindow(this);
    auto networkClientFactory = std::make_shared<NetworkClientFactory>();
    scriptsManager_ = std::make_unique<ScriptsManager>(networkClientFactory);
    auto uploadErrorHandler = serviceLocator->uploadErrorHandler();
    uploadEngineManager_ = std::make_unique<UploadEngineManager>(engineList, uploadErrorHandler, networkClientFactory);
    uploadManager_ = std::make_unique<UploadManager>(uploadEngineManager_.get(), scriptsManager_.get(),
                                                     uploadErrorHandler, networkClientFactory, settings, 3);
    std::string dataDirectory = AppRuntimeInfo::instance()->dataDirectory();
    std::string iconsDir = dataDirectory + "Favicons/";
    serverIconCache_ = std::make_unique<QtServerIconCache>(engineList, iconsDir);
    serviceLocator->setServerIconCache(serverIconCache_.get());

    std::string scriptsDirectory = dataDirectory + "/Scripts/";
    uploadEngineManager_->setScriptsDirectory(scriptsDirectory);

    uploadSessionList()->setUploadManager(uploadManager_.get());
    connect(uploadSessionList(), &UploadSessionListWidget::showCodesRequested, this,
            qOverload<UploadSession*, UploadTask*>(&MainWindow::showCodes));
    connect(uploadSessionList(), &UploadSessionListWidget::imageViewerRequested, this, &MainWindow::openImageViewer);
    connect(uploadSessionList(), &UploadSessionListWidget::removeTaskRequested, this, &MainWindow::removeTask);
    connect(uploadSessionList(), &UploadSessionListWidget::contextMenuRequested, this,
            &MainWindow::onCustomContextMenu);

    copyDirectLinkAction_ = new QAction(tr("Copy direct link"), this);
    copyDirectLinkAction_->setShortcut(QKeySequence::Copy);
    connect(copyDirectLinkAction_, &QAction::triggered, this, &MainWindow::onCopyDirectLinkTriggered);

    copyFilePathAction_ = new QAction(tr("Copy file path to clipboard"), this);
    copyFilePathAction_->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_C));
    connect(copyFilePathAction_, &QAction::triggered, this, &MainWindow::onCopyFilePathTriggered);

    uploadSessionList()->addAction(copyDirectLinkAction_);
    uploadSessionList()->addAction(copyFilePathAction_);

    const ServerProfile& imageProfile = settings->imageServer.getByIndex(0);
    const ServerProfile& fileServerProfile = settings->fileServer.getByIndex(0);
    pendingFilesModel_ = std::make_unique<ThumbnailListModel>(this);
    ui->mainTabs->setPendingFilesModel(pendingFilesModel_.get());
    ui->mainTabs->configureUploadSettings(uploadEngineManager_.get(), imageProfile, fileServerProfile);
    ui->mainTabs->setPendingFilesCount(0);
    connect(ui->mainTabs->addedFilesTab(), &AddedFilesTabWidget::clearRequested, this, &MainWindow::clearPendingFiles);
    connect(ui->mainTabs->addedFilesTab(), &AddedFilesTabWidget::removeRequested, this,
            &MainWindow::removePendingFiles);
    connect(ui->mainTabs->addedFilesTab(), &AddedFilesTabWidget::nextRequested, this, &MainWindow::showUploadSettings);
    connect(ui->mainTabs->uploadSettingsTab(), &UploadSettingsTabWidget::backRequested, this,
            [this] { ui->mainTabs->setCurrentIndex(3); });
    connect(ui->mainTabs->uploadSettingsTab(), &UploadSettingsTabWidget::uploadRequested, this,
            &MainWindow::startPendingUpload);

    iconsLoadingThread_ = new QThread(this);
    auto* timer = new QTimer(nullptr);
    timer->moveToThread(iconsLoadingThread_);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, [this, serviceLocator]() {
        serviceLocator->serverIconCache()->preLoadIcons(96);
        QMetaObject::invokeMethod(this, "fillServerIcons", Qt::AutoConnection);
        iconsLoadingThread_->quit();
    });
    connect(iconsLoadingThread_, SIGNAL(started()), timer, SLOT(start()));
    connect(iconsLoadingThread_, &QThread::destroyed, timer, &QTimer::deleteLater);

    iconsLoadingThread_->start();

    QMenu* trayContextMenu = new QMenu(this);
    QAction* trayExitAction = trayContextMenu->addAction(tr("Exit"));
    connect(trayExitAction, &QAction::triggered, this, &MainWindow::quitApp);
    systemTrayIcon_ = new QSystemTrayIcon(this);
    systemTrayIcon_->setIcon(QIcon(":/res/icon_main.ico"));
    systemTrayIcon_->setContextMenu(trayContextMenu);
    connect(systemTrayIcon_, &QSystemTrayIcon::activated, [&](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick) {
            setWindowState(windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
            show();
            activateWindow();
        }
    });
    systemTrayIcon_->show();
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::ChildRemoved) {
        // ui->listWidget->setFlow(QListWidget::LeftToRight);
    }
    return false;
}

MainWindow::~MainWindow() {
    uploadSessionList()->detach();
    uploadManager_.reset(); // Must be destroyed first
    iconsLoadingThread_->wait();
}

void MainWindow::updateView() { }

void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (IsThumbnailListInternalDrag(event->mimeData())) {
        dragContainsFiles_ = false;
        dropHighlightOverlay_->hide();
        event->ignore();
        return;
    }
    const bool hasVirtualFiles = VirtualFileDrop::hasFiles(event->mimeData());
    const QStringList fileNames = LocalFilesFromMimeData(event->mimeData());
    const QStringList virtualFileNames
        = hasVirtualFiles ? VirtualFileDrop::fileNames(event->mimeData()) : QStringList { };
    const QStringList& detectedFileNames = hasVirtualFiles ? virtualFileNames : fileNames;
    dragContainsFiles_ = hasVirtualFiles || !fileNames.isEmpty();
    if (!dragContainsFiles_) {
        event->ignore();
        return;
    }
    const bool singleFile = detectedFileNames.size() == 1;
    const bool video = singleFile && IsFileOfType(detectedFileNames.first(), VideoUtils::videoFilesExtensions);
    const bool audio
        = singleFile && !video && IsFileOfType(detectedFileNames.first(), VideoUtils::audioFilesExtensions);
    dropHighlightOverlay_->setMediaFile(video, audio);
    dropHighlightOverlay_->setDragPosition(event->position().toPoint());
    dropHighlightOverlay_->setGeometry(rect());
    dropHighlightOverlay_->raise();
    dropHighlightOverlay_->show();
    event->acceptProposedAction();
}

void MainWindow::dragMoveEvent(QDragMoveEvent* event) {
    if (dragContainsFiles_) {
        dropHighlightOverlay_->setDragPosition(event->position().toPoint());
        event->acceptProposedAction();
    } else {
        event->ignore();
    }
}

void MainWindow::dragLeaveEvent(QDragLeaveEvent* event) {
    dragContainsFiles_ = false;
    dropHighlightOverlay_->hide();
    event->accept();
}

void MainWindow::dropEvent(QDropEvent* event) {
    dragContainsFiles_ = false;
    if (IsThumbnailListInternalDrag(event->mimeData())) {
        dropHighlightOverlay_->hide();
        event->ignore();
        return;
    }
    const FileDropHighlight::Action action = dropHighlightOverlay_->actionAt(event->position().toPoint());
    dropHighlightOverlay_->hide();
    QStringList fileNames = LocalFilesFromMimeData(event->mimeData());
    if (fileNames.isEmpty()) {
        fileNames = VirtualFileDrop::materializeFiles(event->mimeData());
    }
    if (fileNames.isEmpty()) {
        event->ignore();
        return;
    }
    if (action == FileDropHighlight::Action::ExtractFrames && fileNames.size() == 1
        && IsFileOfType(fileNames.first(), VideoUtils::videoFilesExtensions)) {
        event->acceptProposedAction();
        openMediaDialog(fileNames.first());
        return;
    }
    if (action == FileDropHighlight::Action::FileInformation && fileNames.size() == 1
        && (IsFileOfType(fileNames.first(), VideoUtils::videoFilesExtensions)
            || IsFileOfType(fileNames.first(), VideoUtils::audioFilesExtensions))) {
        event->acceptProposedAction();
        openMediaDialog(fileNames.first(), true);
        return;
    }
    addMultipleFilesToList(fileNames);
    event->acceptProposedAction();
}

void MainWindow::resizeEvent(QResizeEvent* event) {
    QMainWindow::resizeEvent(event);
    if (dropHighlightOverlay_) {
        dropHighlightOverlay_->setGeometry(rect());
    }
}

void MainWindow::on_actionGrab_frames_triggered() {
    QFileDialog fd(this, "Open multimedia file", QString(), tr("All files (*.*)"));
    fd.setModal(true);
    fd.setWindowModality(Qt::WindowModal);
    if (fd.exec() != QDialog::Accepted) {
        return;
    }
    auto files = fd.selectedFiles();

    if (files.empty()) {
        return;
    }
    QString fileName = files.first();

    openMediaDialog(fileName);
}

void MainWindow::on_actionMedia_info_triggered() {
    QFileDialog fileDialog(this, tr("Open multimedia file"), QString(), tr("All files (*.*)"));
    fileDialog.setModal(true);
    fileDialog.setWindowModality(Qt::WindowModal);
    if (fileDialog.exec() != QDialog::Accepted || fileDialog.selectedFiles().isEmpty()) {
        return;
    }

    openMediaDialog(fileDialog.selectedFiles().first(), true);
}

void MainWindow::openMediaDialog(const QString& fileName, bool showMediaInfo) {
    MediaDlg dlg(fileName, this, showMediaInfo);
    dlg.setModal(true);
    dlg.setWindowModality(Qt::WindowModal);
    if (dlg.exec() == QDialog::Accepted) {
        QStringList frames;
        dlg.getGrabbedFrames(frames);
        addMultipleFilesToList(frames);
    }
}

void MainWindow::on_actionScreenshot_triggered() {

    CScreenCaptureEngine eng;
    eng.setDelay(450);
    hide();
    eng.captureScreen();

    std::unique_ptr<CScreenshotRegion> r;
    std::unique_ptr<RegionSelect> selector = std::make_unique<RegionSelect>(nullptr, eng.capturedBitmap());

    if (selector->exec() == QDialog::Accepted) {
        r.reset(selector->selectedRegion());
    } else {
        show();
        return;
    }

    if (!r) {
        r = std::make_unique<CActiveWindowRegion>();
    }

    eng.setSource(*eng.capturedBitmap());
    if (!eng.captureRegion(r.get())) {
        show();
        return;
    }

    QPixmap* screen = eng.capturedBitmap(); // QPixmap::grabWindow(QApplication::desktop()->winId());
    QTemporaryFile f(U2Q(AppRuntimeInfo::instance()->tempDirectory()) + "/screenshot_XXXXXX.png");
    f.setAutoRemove(false);
    QString uniqueFileName;
    if (f.open()) {
        uniqueFileName = f.fileName();
        f.close();
    }
    if (!uniqueFileName.isEmpty()) {
        if (screen->save(uniqueFileName)) {
            addFileToList(uniqueFileName);
        }
    }
    show();
}

void MainWindow::on_actionAdd_files_triggered() {
    QFileDialog fd(this, "Open files", QString(), tr("All files (*.*)"));
    fd.setModal(true);
    fd.setWindowModality(Qt::WindowModal);
    if (fd.exec() == QDialog::Accepted) {
        auto fileNames = fd.selectedFiles();

        if (fileNames.empty()) {
            return;
        }

        addMultipleFilesToList(fileNames);
    }
}

bool MainWindow::addFileToList(QString fileName) { return addMultipleFilesToList({ std::move(fileName) }); }

bool MainWindow::addMultipleFilesToList(QStringList fileNames) {
    if (fileNames.isEmpty()) {
        return false;
    }
    const int firstAddedRow = pendingFilesModel_->addFiles(fileNames);
    if (firstAddedRow < 0) {
        return false;
    }
    ui->mainTabs->setPendingFilesCount(pendingFilesModel_->rowCount());
    ui->mainTabs->addedFilesTab()->revealRow(firstAddedRow);
    ui->mainTabs->setCurrentIndex(3);
    return true;
}

void MainWindow::clearPendingFiles() {
    pendingFilesModel_->clear();
    ui->mainTabs->setPendingFilesCount(0);
}

void MainWindow::removePendingFiles(const QList<int>& rows) {
    pendingFilesModel_->removeItems(rows);
    ui->mainTabs->setPendingFilesCount(pendingFilesModel_->rowCount());
}

void MainWindow::showUploadSettings() {
    if (pendingFilesModel_->rowCount() > 0) {
        ui->mainTabs->setCurrentIndex(4);
    }
}

void MainWindow::startPendingUpload() {
    const QStringList fileNames = pendingFilesModel_->filePaths();
    if (fileNames.isEmpty()) {
        return;
    }

    auto* settingsTab = ui->mainTabs->uploadSettingsTab();
    const ServerProfile imageProfile = settingsTab->imageServerProfile();
    const ServerProfile fileProfile = settingsTab->fileServerProfile();
    if (imageProfile.serverName().empty() || fileProfile.serverName().empty()) {
        return;
    }

    auto uploadSession = std::make_shared<UploadSession>();
    QMimeDatabase mimeDatabase;
    for (const QString& fileName : fileNames) {
        auto task = std::make_shared<FileUploadTask>(Q2U(fileName), IuCoreUtils::ExtractFileName(Q2U(fileName)));
        const bool isImage = mimeDatabase.mimeTypeForFile(fileName, QMimeDatabase::MatchExtension)
                                 .name()
                                 .startsWith(QStringLiteral("image/"));
        ServerProfile profile = isImage ? imageProfile : fileProfile;
        if (profile.useDefaultSettings()) {
            auto* settings = ServiceLocator::instance()->settings<CommonGuiSettings>();
            profile.setImageUploadParams(settings->DefaultImageUploadParams);
        }
        task->setIsImage(isImage);
        task->setServerProfile(profile);
        task->setIndex(uploadSession->taskCount());
        uploadSession->addTask(task);
    }

    uploadManager_->addSession(uploadSession);
    uploadSessionList()->selectSession(uploadSession.get());
    clearPendingFiles();
    saveOptions();
    ui->mainTabs->setCurrentIndex(0);
}

void MainWindow::showCodes(UploadSession* session, UploadTask* task) {
    showCodes(findSession(session), findTask(task));
}

void MainWindow::uploadTaskToUploadObject(UploadTask* task, UploadObject& obj) {
    obj.fillFromUploadResult(task->uploadResult(), task);
}

void MainWindow::showCodes(const std::shared_ptr<UploadSession>& session, const std::shared_ptr<UploadTask>& task) {
    std::vector<UploadObject> uploadObjects;
    if (task && task->uploadSuccess(false)) {
        UploadObject obj;
        uploadTaskToUploadObject(task.get(), obj);
        uploadObjects.push_back(obj);
    } else if (session && !task) {
        int count = session->taskCount();
        for (int i = 0; i < count; i++) {
            auto sessionTask = session->getTask(i);
            if (sessionTask && sessionTask->uploadSuccess(false)) {
                UploadObject obj;
                uploadTaskToUploadObject(sessionTask.get(), obj);
                uploadObjects.push_back(obj);
            }
        }
    }
    ResultsWindow dlg(uploadObjects, this);
    dlg.setModal(true);
    dlg.setWindowModality(Qt::WindowModal);
    dlg.exec();
}

void MainWindow::onCustomContextMenu(UploadSession* sessionPtr, UploadTask* taskPtr, const QPoint& globalPosition) {
    const auto session = findSession(sessionPtr);
    const auto task = findTask(taskPtr);
    if (!session) {
        return;
    }

    QMenu contextMenu(this);
    QAction* viewCodeAction = contextMenu.addAction(tr("View HTML/BBCode"));
    connect(viewCodeAction, &QAction::triggered, this, [this, session, task] { showCodes(session, task); });
    contextMenu.setDefaultAction(viewCodeAction);

    if (task) {
        auto* uploadResult = task->uploadResult();
        QString directUrl = QString::fromStdString(uploadResult->getDirectUrl());
        QString viewUrl = QString::fromStdString(uploadResult->getDownloadUrl());

        if (!directUrl.isEmpty()) {
            contextMenu.addAction(copyDirectLinkAction_);
        }

        if (!viewUrl.isEmpty()) {
            QAction* copyViewLinkAction = contextMenu.addAction(tr("Copy view link"));
            if (directUrl.isEmpty()) {
                copyViewLinkAction->setShortcut(QKeySequence::Copy);
            }
            connect(copyViewLinkAction, &QAction::triggered,
                    [viewUrl] { QApplication::clipboard()->setText(viewUrl); });
        }
        contextMenu.addSeparator();
        QString url = directUrl.isEmpty() ? viewUrl : directUrl;
        if (!url.isEmpty()) {
            QAction* viewInBrowser = contextMenu.addAction(tr("Open in browser"));
            connect(viewInBrowser, &QAction::triggered, [url] { QDesktopServices::openUrl(QUrl(url)); });
        }
        auto fileTask = std::dynamic_pointer_cast<FileUploadTask>(task);

        if (fileTask) {
            QString fileName = U2Q(fileTask->getFileName());
            QAction* openInProgram = contextMenu.addAction(tr("Open file in default program"));
            connect(openInProgram, &QAction::triggered,
                    [fileName] { QDesktopServices::openUrl(QUrl::fromLocalFile(fileName)); });
            contextMenu.addAction(copyFilePathAction_);
        }
        contextMenu.addSeparator();
        QAction* removeAction = contextMenu.addAction(tr("Remove"));
        connect(removeAction, &QAction::triggered, this,
                [this, session, task] { removeTask(session.get(), task.get()); });
    } else {
        QAction* retryAction = contextMenu.addAction(tr("Retry failed"));
        connect(retryAction, &QAction::triggered, this, [this, session] {
            uploadManager_->retrySession(session);
            uploadSessionList()->refresh();
        });
        contextMenu.addSeparator();
        QAction* removeAction = contextMenu.addAction(tr("Remove session"));
        connect(removeAction, &QAction::triggered, this, [this, session] {
            session->stop(true);
            uploadSessionList()->hideSession(session.get());
        });
    }
    contextMenu.exec(globalPosition);
}

void MainWindow::openImageViewer(UploadSession* session, UploadTask* task) {
    if (!findSession(session) || !findTask(task)) {
        return;
    }

    QStringList imageFiles;
    int currentIndex = -1;
    for (int sessionIndex = 0; sessionIndex < uploadManager_->sessionCount(); ++sessionIndex) {
        const auto uploadSession = uploadManager_->session(sessionIndex);
        for (int taskIndex = 0; taskIndex < uploadSession->taskCount(); ++taskIndex) {
            const auto uploadTask = uploadSession->getTask(taskIndex);
            const auto fileTask = std::dynamic_pointer_cast<FileUploadTask>(uploadTask);
            if (!fileTask || !fileTask->isImage()) {
                continue;
            }
            if (uploadTask.get() == task) {
                currentIndex = imageFiles.size();
            }
            imageFiles.append(U2Q(fileTask->getFileName()));
        }
    }
    if (currentIndex < 0) {
        return;
    }

    if (!imageViewerWindow_) {
        imageViewerWindow_ = std::make_unique<ImageViewerWindow>();
        imageViewerWindow_->setTransientParent(windowHandle());
    }
    imageViewerWindow_->setImageViewerSource(
        std::make_unique<FileListImageViewerSource>(std::move(imageFiles), currentIndex));
    imageViewerWindow_->open();
}

void MainWindow::removeTask(UploadSession* session, UploadTask* task) {
    if (!findSession(session) || !findTask(task)) {
        return;
    }
    task->stop(true);
    uploadSessionList()->hideTask(task);
}

void MainWindow::onShowLog() {
    logWindow_->show();
    logWindow_->activateWindow();
}

void MainWindow::fillServerIcons() { ui->mainTabs->uploadSettingsTab()->fillServerIcons(); }

void MainWindow::onCopyDirectLinkTriggered(bool checked) {
    Q_UNUSED(checked);
    const auto task = uploadSessionList()->selectedTask();
    if (!task) {
        return;
    }
    QApplication::clipboard()->setText(U2Q(task->uploadResult()->getDirectUrl()));
}

void MainWindow::onCopyFilePathTriggered(bool checked) {
    Q_UNUSED(checked);
    const auto task = uploadSessionList()->selectedTask();
    if (!task) {
        return;
    }
    auto fileTask = std::dynamic_pointer_cast<FileUploadTask>(task);

    if (fileTask) {
        QString fileName = U2Q(fileTask->getFileName());
        QApplication::clipboard()->setText(fileName);
    }
}

void MainWindow::on_actionAboutProgram_triggered() {
    AboutDialog dlg(this);
    dlg.setModal(true);
    dlg.setWindowModality(Qt::WindowModal);
    dlg.exec();
}

void MainWindow::saveOptions() {
    auto settings = ServiceLocator::instance()->settings<QtGuiSettings>();
    settings->imageServer = ui->mainTabs->uploadSettingsTab()->imageServerProfile();
    settings->fileServer = ui->mainTabs->uploadSettingsTab()->fileServerProfile();
}

void MainWindow::quitApp() {
    close();
    QApplication::quit();
}

WindowHandle MainWindow::getHandle() { return this; }

WindowNativeHandle MainWindow::getNativeHandle() { return reinterpret_cast<WindowNativeHandle>(effectiveWinId()); }

void MainWindow::setServersChanged(bool changed) {
    // TODO:
}

void MainWindow::closeEvent(QCloseEvent* event) { saveOptions(); }

std::shared_ptr<UploadSession> MainWindow::findSession(UploadSession* session) const {
    if (!session) {
        return { };
    }
    for (int i = 0; i < uploadManager_->sessionCount(); ++i) {
        auto candidate = uploadManager_->session(i);
        if (candidate.get() == session) {
            return candidate;
        }
    }
    return { };
}

std::shared_ptr<UploadTask> MainWindow::findTask(UploadTask* task) const {
    if (!task) {
        return { };
    }
    for (int i = 0; i < uploadManager_->sessionCount(); ++i) {
        auto session = uploadManager_->session(i);
        for (int j = 0; j < session->taskCount(); ++j) {
            auto candidate = session->getTask(j);
            if (candidate.get() == task) {
                return candidate;
            }
        }
    }
    return { };
}

UploadSessionListWidget* MainWindow::uploadSessionList() const {
    return ui->mainTabs->uploadsTab()->sessionList();
}
