#include "MainWindow.h"

#include "ui_MainWindow.h"

#include <vector>
#include <QDebug>
#include <QFileDialog>
#include <QToolButton>
#include <QMessageBox>
#include <QDesktopServices>
#include <QTemporaryFile>
#include <QClipboard>
#include <QSystemTrayIcon>
#include <QTimer>
#include <QThread>

#include "Core/BasicConstants.h"
#include "Gui/FrameGrabberDlg.h"
#include "Gui/RegionSelect.h"
#include "Gui/controls/ServerSelectorWidget.h"
#include "models/uploadtreemodel.h"
#include "Core/CommonDefs.h"
#include "Core/Upload/UploadManager.h"
#include "Core/Upload/UploadEngineManager.h"
#include "Core/ServiceLocator.h"
#include "Core/Upload/FileUploadTask.h"
#include "Core/Scripting/ScriptsManager.h"
#include "Core/AppRuntimeInfo.h"
#include "Core/Settings/QtGuiSettings.h"
#include "Core/Network/NetworkClientFactory.h"
#include "ResultsWindow.h"
#include "Gui/LogWindow.h"
#include "Core/OutputGenerator/AbstractOutputGenerator.h"
#include "AboutDialog.h"
#include "Core/QtServerIconCache.h"

using namespace Uptooda::Core::OutputGenerator;

MainWindow::MainWindow(CUploadEngineList* engineList, LogWindow* logWindow, QWidget* parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    logWindow_(logWindow) {
    ui->setupUi(this);
    setWindowTitle(APP_NAME_A + QStringLiteral(" (Qt GUI)"));
    auto* serviceLocator = ServiceLocator::instance();
    auto* settings = serviceLocator->settings<QtGuiSettings>();
    engineList_ = engineList;
    serviceLocator->setProgramWindow(this);
    auto networkClientFactory = std::make_shared<NetworkClientFactory>();
    scriptsManager_ = std::make_unique<ScriptsManager>(networkClientFactory);
    auto uploadErrorHandler = serviceLocator->uploadErrorHandler();
    uploadEngineManager_ = std::make_unique<UploadEngineManager>(engineList, uploadErrorHandler, networkClientFactory);
    uploadManager_ = std::make_unique<UploadManager>(uploadEngineManager_.get(), scriptsManager_.get(), uploadErrorHandler,
                                       networkClientFactory, settings, 3);
    std::string dataDirectory = AppRuntimeInfo::instance()->dataDirectory();
    std::string iconsDir = dataDirectory + "Favicons/";
    serverIconCache_ = std::make_unique<QtServerIconCache>(engineList, iconsDir);
    serviceLocator->setServerIconCache(serverIconCache_.get());

    std::string scriptsDirectory = dataDirectory + "/Scripts/";
    uploadEngineManager_->setScriptsDirectory(scriptsDirectory);

    uploadTreeModel_ = new UploadTreeModel(this, uploadManager_.get());

    ui->treeView->setModel(uploadTreeModel_);
    ui->treeView->setContextMenuPolicy(Qt::CustomContextMenu);
    ui->treeView->setColumnWidth(0, 150); // Filename column
    ui->treeView->setColumnWidth(1, 150); // Status column
    ui->treeView->setColumnWidth(2, 150); // Progress column
    ui->treeView->setSelectionBehavior(QAbstractItemView::SelectRows);

    connect(ui->treeView, &QTreeView::doubleClicked, this, &MainWindow::itemDoubleClicked);
    connect(ui->treeView, &QTreeView::customContextMenuRequested, this, &MainWindow::onCustomContextMenu);

    copyDirectLinkAction_ = new QAction(tr("Copy direct link"), this);
    copyDirectLinkAction_->setShortcut(QKeySequence::Copy);
    connect(copyDirectLinkAction_, &QAction::triggered, this, &MainWindow::onCopyDirectLinkTriggered);

    copyFilePathAction_ = new QAction(tr("Copy file path to clipboard"), this);
    copyFilePathAction_->setShortcut(QKeySequence(Qt::CTRL + Qt::SHIFT + Qt::Key_C));
    connect(copyFilePathAction_, &QAction::triggered, this, &MainWindow::onCopyFilePathTriggered);

    ui->treeView->addAction(copyDirectLinkAction_);
    ui->treeView->addAction(copyFilePathAction_);

    const ServerProfile& imageProfile = settings->imageServer.getByIndex(0);

    imageServerWidget_ = new ServerSelectorWidget(uploadEngineManager_.get(), false, this);
    imageServerWidget_->setTitle(tr("Server for images:"));
    imageServerWidget_->setServerProfile(imageProfile);
    ui->verticalLayout->insertWidget(1, imageServerWidget_);
    fileServerWidget_ = new ServerSelectorWidget(uploadEngineManager_.get(), false, this);
    fileServerWidget_->setTitle(tr("Server for other file types:"));
    fileServerWidget_->setServersMask(ServerSelectorWidget::smFileServers);
    fileServerWidget_->updateServerList();

    const ServerProfile& fileServerProfile = settings->fileServer.getByIndex(0);
    fileServerWidget_->setServerProfile(fileServerProfile);
    ui->verticalLayout->insertWidget(2, fileServerWidget_);

    iconsLoadingThread_ = new QThread(this);
    auto* timer = new QTimer(nullptr);
    timer->moveToThread(iconsLoadingThread_);
    timer->setSingleShot(true);
    connect(timer, &QTimer::timeout, [this, serviceLocator](){
        serviceLocator->serverIconCache()->preLoadIcons(96);
        QMetaObject::invokeMethod(this, "fillServerIcons", Qt::AutoConnection);
        iconsLoadingThread_->quit();
    });
    connect(iconsLoadingThread_, SIGNAL(started()), timer, SLOT(start()));
    connect(iconsLoadingThread_, &QThread::destroyed, timer, &QTimer::deleteLater);

    iconsLoadingThread_->start();

    QMenu* trayContextMenu = new QMenu(this);
    QAction* exitAction = trayContextMenu->addAction(tr("Exit"));
    connect(exitAction, &QAction::triggered, this, &MainWindow::quitApp);
    systemTrayIcon_ = new QSystemTrayIcon(this);
    systemTrayIcon_->setIcon(QIcon(":/res/icon_main.ico"));
    systemTrayIcon_->setContextMenu(trayContextMenu);
    connect(systemTrayIcon_, &QSystemTrayIcon::activated, [&](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger || reason == QSystemTrayIcon::DoubleClick){
            setWindowState(windowState() & ~Qt::WindowMinimized | Qt::WindowActive);
            show();
            activateWindow();
        }
    });
    systemTrayIcon_->show();

    connect(ui->showLogButton, &QPushButton::clicked, this, &MainWindow::onShowLog);
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::ChildRemoved) {
        // ui->listWidget->setFlow(QListWidget::LeftToRight);
    }
    return false;
}

MainWindow::~MainWindow() {
    uploadTreeModel_->reset();
    uploadManager_.reset(); // Must be destroyed first
    iconsLoadingThread_->wait();
}

void MainWindow::updateView() {

}

void MainWindow::on_actionGrab_frames_triggered() {
    QFileDialog fd(this,"Open multimedia file", QString(), tr("All files (*.*)"));
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

    FrameGrabberDlg dlg(fileName, this);
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

    QPixmap* screen = eng.capturedBitmap(); //QPixmap::grabWindow(QApplication::desktop()->winId());
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
    QFileDialog fd(this,"Open files", QString(), tr("All files (*.*)"));
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

bool MainWindow::addFileToList(QString fileName) {
    if (fileName.isEmpty()) {
        return false;
    }
    auto uploadSession = std::make_shared<UploadSession>();
    auto task = std::make_shared<FileUploadTask>(Q2U(fileName), IuCoreUtils::ExtractFileName(Q2U(fileName)));
    ServerProfile serverProfile = imageServerWidget_->serverProfile();
    task->setServerProfile(serverProfile);
    uploadSession->addTask(task);
    uploadManager_->addSession(uploadSession);

    // Select and expand newly created tree item
    ui->treeView->clearSelection();
    QModelIndex index = uploadTreeModel_->index(uploadTreeModel_->rowCount() - 1, 0);
    QModelIndex indexLast = uploadTreeModel_->index(uploadTreeModel_->rowCount() - 1, uploadTreeModel_->columnCount() - 1);
    ui->treeView->selectionModel()->select(QItemSelection(index, indexLast), QItemSelectionModel::Select);
    ui->treeView->expand(index);
    return true;
}

bool MainWindow::addMultipleFilesToList(QStringList fileNames) {
    if (fileNames.isEmpty()) {
        return false;
    }
    auto uploadSession = std::make_shared<UploadSession>();
    for (const auto& fileName : fileNames) {
        auto task = std::make_shared<FileUploadTask>(Q2U(fileName), IuCoreUtils::ExtractFileName(Q2U(fileName)));
        ServerProfile serverProfile = imageServerWidget_->serverProfile();
        task->setServerProfile(serverProfile);
        uploadSession->addTask(task);
    }

    uploadManager_->addSession(uploadSession);

    // Select and expand newly created tree item
    ui->treeView->clearSelection();
    QModelIndex index = uploadTreeModel_->index(uploadTreeModel_->rowCount() - 1, 0);
    QModelIndex indexLast = uploadTreeModel_->index(uploadTreeModel_->rowCount() - 1, uploadTreeModel_->columnCount() - 1);
    ui->treeView->selectionModel()->select(QItemSelection(index, indexLast), QItemSelectionModel::Select);
    ui->treeView->expand(index);
        
        
    return true;
}

void MainWindow::itemDoubleClicked(const QModelIndex& index) {
    showCodeForIndex(index);
}

void MainWindow::uploadTaskToUploadObject(UploadTask* task, UploadObject& obj) {
    obj.fillFromUploadResult(task->uploadResult(), task);
}

void MainWindow::showCodeForIndex(const QModelIndex& index) {
    std::vector<UploadObject> uploadObjects;
    auto item = uploadTreeModel_->getInternalItem(index);
    if (item->session) {
        int count = item->session->taskCount();
        for (int i = 0; i < count; i++) {
            auto task = item->session->getTask(i);
            if (task && task->uploadSuccess(false)) {
                UploadObject obj;
                uploadTaskToUploadObject(task.get(), obj);
                uploadObjects.push_back(obj);
            }
        }
    }
    else if (item->task && item->task->uploadSuccess(false)) {
        UploadObject obj;
        uploadTaskToUploadObject(item->task.get(), obj);
        uploadObjects.push_back(obj);
    }
    ResultsWindow dlg(uploadObjects, this);
    dlg.setModal(true);
    dlg.setWindowModality(Qt::WindowModal);
    dlg.exec();
}

void MainWindow::onCustomContextMenu(const QPoint& point) {
    QModelIndex index = ui->treeView->indexAt(point);
    if (index.isValid()) {
        UploadTreeModel::InternalItem* internalItem = uploadTreeModel_->getInternalItem(index);
        QMenu* contextMenu = new QMenu(ui->treeView);

        QAction* viewCodeAction = new QAction(tr("View HTML/BBCode"), contextMenu);

        contextMenu->addAction(viewCodeAction);
        connect(viewCodeAction, &QAction::triggered, [index, this](bool checked)
        {
            showCodeForIndex(index);
        });
        contextMenu->setDefaultAction(viewCodeAction);

        if (internalItem->task) {
            auto* uploadResult = internalItem->task->uploadResult();
            QString directUrl = QString::fromStdString(uploadResult->getDirectUrl());
            QString viewUrl = QString::fromStdString(uploadResult->getDownloadUrl());

            if (!directUrl.isEmpty()) {
                contextMenu->addAction(copyDirectLinkAction_);
            }

            if (!viewUrl.isEmpty()) {
                QAction* copyViewLinkAction = new QAction(tr("Copy view link"), contextMenu);
                if (directUrl.isEmpty()) {
                    copyViewLinkAction->setShortcut(QKeySequence::Copy);
                }
                connect(copyViewLinkAction, &QAction::triggered, [viewUrl](bool checked) {
                    QClipboard* clipboard = QApplication::clipboard();
                    clipboard->setText(viewUrl);
                });
                contextMenu->addAction(copyViewLinkAction);
            }
            contextMenu->addSeparator();
            QString url = directUrl.isEmpty() ? viewUrl : directUrl;
            if (!url.isEmpty()) {
                QAction* viewInBrowser = new QAction(tr("Open in browser"), contextMenu);
                connect(viewInBrowser, &QAction::triggered, [url](bool checked)
                {
                    QDesktopServices::openUrl(url);
                });
                contextMenu->addAction(viewInBrowser);
            }
            auto fileTask = std::dynamic_pointer_cast<FileUploadTask>(internalItem->task);
           
            if (fileTask) {
                QString fileName = U2Q(fileTask->getFileName());
                QAction* openInProgram = new QAction(tr("Open file in default program"), contextMenu);
                connect(openInProgram, &QAction::triggered, [fileName](bool checked)
                {
                    QDesktopServices::openUrl("file:///" + fileName);
                });
                contextMenu->addAction(openInProgram);
                contextMenu->addAction(copyFilePathAction_);
            }

        }

        contextMenu->exec(ui->treeView->viewport()->mapToGlobal(point));
    }
}

void MainWindow::onShowLog() {
    logWindow_->show();
    logWindow_->activateWindow();
}

void MainWindow::fillServerIcons() {
    imageServerWidget_->fillServerIcons();
    fileServerWidget_->fillServerIcons();
}

void MainWindow::onCopyDirectLinkTriggered(bool checked) {
    QModelIndex index = getUploadTreeViewSelectedIndex();

    if (!index.isValid()) {
        return;
    }

    QString url = ui->treeView->model()->data(index, Qt::UserRole).toString();
    QClipboard* clipboard = QApplication::clipboard();
    clipboard->setText(url);
}

void MainWindow::onCopyFilePathTriggered(bool checked) {
    QModelIndex index = getUploadTreeViewSelectedIndex();

    if (!index.isValid()) {
        return;
    }

    UploadTreeModel::InternalItem* internalItem = uploadTreeModel_->getInternalItem(index);

    if (!internalItem || !internalItem->task) {
        return;

    }
    auto fileTask = std::dynamic_pointer_cast<FileUploadTask>(internalItem->task);

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

void  MainWindow::saveOptions() {
    auto settings = ServiceLocator::instance()->settings<QtGuiSettings>();
    settings->imageServer = imageServerWidget_->serverProfile();
    settings->fileServer = fileServerWidget_->serverProfile();
}

void MainWindow::quitApp() {
    close();
    QApplication::quit();
}

WindowHandle MainWindow::getHandle() {
    return this;
}

WindowNativeHandle MainWindow::getNativeHandle() {
    return reinterpret_cast<WindowNativeHandle>(effectiveWinId());
}

void MainWindow::setServersChanged(bool changed) {
    // TODO:
}

void MainWindow::closeEvent(QCloseEvent *event) {
    saveOptions();
}

QModelIndex MainWindow::getUploadTreeViewSelectedIndex() {
    QItemSelectionModel * selection = ui->treeView->selectionModel();
    QModelIndexList indexes = selection->selectedIndexes();

    if(indexes.size() < 1) {
        return {};
    }
    std::sort(indexes.begin(), indexes.end());
    QModelIndex index = indexes.first();
    return index;
}

