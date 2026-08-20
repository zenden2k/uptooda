#include "UploadSessionListWidget.h"

#include <QFileInfo>
#include <QFrame>
#include <QHBoxLayout>
#include <QIcon>
#include <QLabel>
#include <QMouseEvent>
#include <QPointer>
#include <QProgressBar>
#include <QPushButton>
#include <QScrollBar>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>

#include <algorithm>
#include <functional>
#include <utility>

#include "Core/AppRuntimeInfo.h"
#include "Core/CommonDefs.h"
#include "Core/Upload/FileUploadTask.h"
#include "Core/Upload/UploadManager.h"
#include "Core/Upload/UploadSession.h"
#include "Gui/FileThumbnailCache.h"

namespace {
constexpr QSize TASK_THUMBNAIL_SIZE(64, 64);

class InteractiveFrame final : public QFrame {
public:
    explicit InteractiveFrame(QWidget* parent = nullptr) : QFrame(parent) { }

    std::function<void(Qt::MouseButton, const QPoint&)> pressed;
    std::function<void()> doubleClicked;

protected:
    void mousePressEvent(QMouseEvent* event) override {
        if (pressed) {
            pressed(event->button(), event->globalPosition().toPoint());
        }
        QFrame::mousePressEvent(event);
    }

    void mouseDoubleClickEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton && doubleClicked) {
            doubleClicked();
        }
        QFrame::mouseDoubleClickEvent(event);
    }
};

class ThumbnailLabel final : public QLabel {
public:
    using QLabel::QLabel;
    std::function<void()> clicked;

protected:
    void mouseReleaseEvent(QMouseEvent* event) override {
        if (event->button() == Qt::LeftButton && clicked) {
            clicked();
        }
        QLabel::mouseReleaseEvent(event);
    }
};

QString statusText(UploadTask::Status status) {
    switch (status) {
    case UploadTask::StatusInQueue:
        return QCoreApplication::translate("UploadSessionListWidget", "In queue");
    case UploadTask::StatusRunning:
        return QCoreApplication::translate("UploadSessionListWidget", "Uploading");
    case UploadTask::StatusStopped:
        return QCoreApplication::translate("UploadSessionListWidget", "Stopped");
    case UploadTask::StatusFinished:
        return QCoreApplication::translate("UploadSessionListWidget", "Finished");
    case UploadTask::StatusFailure:
        return QCoreApplication::translate("UploadSessionListWidget", "Failed");
    case UploadTask::StatusPostponed:
        return QCoreApplication::translate("UploadSessionListWidget", "Postponed");
    case UploadTask::StatusWaitingChildren:
        return QCoreApplication::translate("UploadSessionListWidget", "Processing");
    }
    return { };
}

QString statusColor(UploadTask::Status status) {
    switch (status) {
    case UploadTask::StatusFinished:
        return QStringLiteral("#278a56");
    case UploadTask::StatusFailure:
        return QStringLiteral("#c43d4b");
    case UploadTask::StatusStopped:
        return QStringLiteral("#a54f55");
    case UploadTask::StatusRunning:
        return QStringLiteral("#197db8");
    case UploadTask::StatusWaitingChildren:
        return QStringLiteral("#7760b5");
    case UploadTask::StatusPostponed:
        return QStringLiteral("#a36a16");
    default:
        return QStringLiteral("#66768a");
    }
}

QString sizeText(qint64 bytes) {
    if (bytes < 1024) {
        return QStringLiteral("%1 B").arg(bytes);
    }
    if (bytes < 1024 * 1024) {
        return QStringLiteral("%1 KB").arg(bytes / 1024.0, 0, 'f', 1);
    }
    return QStringLiteral("%1 MB").arg(bytes / (1024.0 * 1024.0), 0, 'f', 1);
}

QIcon serverIcon(const std::string& serverName) {
    const QString iconPath = U2Q(AppRuntimeInfo::instance()->dataDirectory()) + QStringLiteral("Favicons/")
        + U2Q(serverName).toLower() + QStringLiteral(".ico");
    return QFileInfo::exists(iconPath) ? QIcon(iconPath) : QIcon(QStringLiteral(":/res/server.png"));
}

void clearLayout(QLayout* layout) {
    while (QLayoutItem* item = layout->takeAt(0)) {
        if (QWidget* widget = item->widget()) {
            delete widget;
        }
        delete item;
    }
}
}

UploadSessionListWidget::UploadSessionListWidget(QWidget* parent) : QScrollArea(parent) {
    setWidgetResizable(true);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    contents_ = new QWidget(this);
    contents_->setObjectName(QStringLiteral("uploadSessionListContents"));
    contentsLayout_ = new QVBoxLayout(contents_);
    contentsLayout_->setContentsMargins(8, 8, 8, 8);
    contentsLayout_->setSpacing(8);
    contentsLayout_->addStretch();
    setWidget(contents_);

    setStyleSheet(QStringLiteral(
        "QWidget#uploadSessionListContents { background: #f3f7fa; }"
        "QFrame#sessionCard { background: white; border: 1px solid #d8e1ea; border-radius: 9px; }"
        "QFrame#taskCard { background: #f8fafc; border: 1px solid #dce4ec; border-radius: 8px; }"
        "QFrame#taskCard:hover { background: #f1f7fb; border-color: #b8d3e5; }"
        "QPushButton { background: #e9f6fd; border: 1px solid #b8dcef; border-radius: 7px; padding: 5px 12px; }"
        "QPushButton:hover { background: #d8effd; }"
        "QToolButton { border: 0; border-radius: 8px; padding: 4px; }"
        "QToolButton:hover { background: #eaf3fa; }"
        "QProgressBar { background: #dfe8ef; border: 0; border-radius: 4px; }"
        "QProgressBar::chunk { background: #5eb5e8; border-radius: 4px; }"));
}

UploadSessionListWidget::~UploadSessionListWidget() { detach(); }

void UploadSessionListWidget::setUploadManager(UploadManager* uploadManager) {
    if (uploadManager_ == uploadManager) {
        return;
    }
    detach();
    uploadManager_ = uploadManager;
    if (!uploadManager_) {
        refresh();
        return;
    }
    uploadManager_->setOnSessionAddedCallback([this](UploadSession* session) {
        QMetaObject::invokeMethod(this, [this, session] {
            attachSession(session);
            refresh();
        });
    });
    uploadManager_->setOnTaskAddedCallback([this](UploadTask* task) {
        if (task) {
            task->setOnUploadProgressCallback([this](UploadTask* changedTask) { queueTaskUpdate(changedTask); });
            task->setOnStatusChangedCallback([this](UploadTask* changedTask) { queueTaskUpdate(changedTask); });
        }
    });
    for (int i = 0; i < uploadManager_->sessionCount(); ++i) {
        attachSession(uploadManager_->session(i).get());
    }
    refresh();
}

void UploadSessionListWidget::detach() {
    if (!uploadManager_) {
        return;
    }
    uploadManager_->setOnSessionAddedCallback({ });
    uploadManager_->setOnTaskAddedCallback({ });
    for (int i = 0; i < uploadManager_->sessionCount(); ++i) {
        auto session = uploadManager_->session(i);
        for (int j = 0; j < session->taskCount(); ++j) {
            session->getTask(j)->setOnUploadProgressCallback({ });
            session->getTask(j)->setOnStatusChangedCallback({ });
        }
    }
    uploadManager_ = nullptr;
}

void UploadSessionListWidget::refresh() {
    refreshQueued_ = false;
    taskWidgets_.clear();
    clearLayout(contentsLayout_);
    if (!uploadManager_) {
        contentsLayout_->addStretch();
        return;
    }

    for (int sessionIndex = uploadManager_->sessionCount() - 1; sessionIndex >= 0; --sessionIndex) {
        const auto session = uploadManager_->session(sessionIndex);
        if (hiddenSessions_.contains(session.get())) {
            continue;
        }

        auto* sessionCard = new QFrame(contents_);
        sessionCard->setObjectName(QStringLiteral("sessionCard"));
        auto* sessionLayout = new QVBoxLayout(sessionCard);
        sessionLayout->setContentsMargins(8, 8, 8, 8);
        sessionLayout->setSpacing(6);

        auto* header = new InteractiveFrame(sessionCard);
        header->setFrameShape(QFrame::NoFrame);
        auto* headerLayout = new QHBoxLayout(header);
        headerLayout->setContentsMargins(4, 1, 2, 1);
        headerLayout->setSpacing(10);

        QStringList servers;
        int visibleTaskCount = 0;
        for (int i = 0; i < session->taskCount(); ++i) {
            const auto task = session->getTask(i);
            if (hiddenTasks_.contains(task.get())) {
                continue;
            }
            ++visibleTaskCount;
            const QString server = U2Q(task->serverName());
            if (!server.isEmpty() && !servers.contains(server)) {
                servers.append(server);
            }
        }

        auto* iconLabel = new QLabel(header);
        iconLabel->setPixmap(serverIcon(servers.isEmpty() ? std::string() : Q2U(servers.first())).pixmap(30, 30));
        iconLabel->setFixedSize(30, 30);
        headerLayout->addWidget(iconLabel);

        auto* titleLayout = new QVBoxLayout;
        titleLayout->setSpacing(1);
        auto* title = new QLabel(servers.isEmpty() ? tr("Upload session") : servers.join(QStringLiteral(", ")), header);
        QFont titleFont = title->font();
        titleFont.setBold(true);
        title->setFont(titleFont);
        title->setStyleSheet(QStringLiteral("color: #263548; border: 0;"));
        titleLayout->addWidget(title);
        auto* countLabel = new QLabel(tr("%n file(s)", nullptr, visibleTaskCount), header);
        countLabel->setStyleSheet(QStringLiteral("color: #7c8999; border: 0;"));
        titleLayout->addWidget(countLabel);
        headerLayout->addLayout(titleLayout, 1);

        auto* sessionCodesButton = new QPushButton(tr("Codes"), header);
        connect(sessionCodesButton, &QPushButton::clicked, this,
                [this, session] { emit showCodesRequested(session.get(), nullptr); });
        headerLayout->addWidget(sessionCodesButton);
        auto* menuButton = new QToolButton(header);
        menuButton->setText(QStringLiteral("\u22ee"));
        menuButton->setFixedSize(32, 32);
        connect(menuButton, &QToolButton::clicked, this, [this, session, menuButton] {
            selectedSession_ = session.get();
            selectedTask_ = nullptr;
            emit contextMenuRequested(session.get(), nullptr, menuButton->mapToGlobal(menuButton->rect().bottomLeft()));
        });
        headerLayout->addWidget(menuButton);
        header->pressed = [this, session](Qt::MouseButton button, const QPoint& position) {
            selectedSession_ = session.get();
            selectedTask_ = nullptr;
            if (button == Qt::RightButton) {
                emit contextMenuRequested(session.get(), nullptr, position);
            }
        };
        sessionLayout->addWidget(header);

        for (int taskIndex = 0; taskIndex < session->taskCount(); ++taskIndex) {
            const auto task = session->getTask(taskIndex);
            if (hiddenTasks_.contains(task.get())) {
                continue;
            }

            auto* taskCard = new InteractiveFrame(sessionCard);
            taskCard->setObjectName(QStringLiteral("taskCard"));
            taskCard->setMinimumHeight(84);
            auto* taskLayout = new QHBoxLayout(taskCard);
            taskLayout->setContentsMargins(7, 7, 7, 7);
            taskLayout->setSpacing(8);

            auto* thumbnail = new ThumbnailLabel(taskCard);
            thumbnail->setFixedSize(64, 64);
            thumbnail->setAlignment(Qt::AlignCenter);
            thumbnail->setStyleSheet(QStringLiteral("background: #eaf0f5; border: 0; border-radius: 2px;"));
            if (auto fileTask = std::dynamic_pointer_cast<FileUploadTask>(task)) {
                const QString fileName = U2Q(fileTask->getFileName());
                FileThumbnailCache& cache = FileThumbnailCache::instance();
                if (const auto cached = cache.cached(fileName)) {
                    thumbnail->setPixmap(QPixmap::fromImage(cached->Image.scaled(
                        TASK_THUMBNAIL_SIZE, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation)));
                    if (cached->IsImage) {
                        thumbnail->setCursor(Qt::PointingHandCursor);
                        thumbnail->clicked
                            = [this, session, task] { emit imageViewerRequested(session.get(), task.get()); };
                    }
                } else {
                    const QPointer<ThumbnailLabel> guardedThumbnail(thumbnail);
                    cache.request(
                        fileName, this, [this, guardedThumbnail, session, task](FileThumbnailCache::Thumbnail result) {
                            if (!guardedThumbnail) {
                                return;
                            }
                            const QImage image = result.Image.scaled(
                                TASK_THUMBNAIL_SIZE, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
                            guardedThumbnail->setPixmap(QPixmap::fromImage(image));
                            if (result.IsImage) {
                                guardedThumbnail->setCursor(Qt::PointingHandCursor);
                                guardedThumbnail->clicked
                                    = [this, session, task] { emit imageViewerRequested(session.get(), task.get()); };
                            }
                        });
                }
            }
            taskLayout->addWidget(thumbnail);

            auto* detailsLayout = new QVBoxLayout;
            detailsLayout->setSpacing(4);
            auto* firstLine = new QHBoxLayout;
            auto* fileLabel = new QLabel(U2Q(task->title()), taskCard);
            QFont fileFont = fileLabel->font();
            fileFont.setBold(true);
            fileLabel->setFont(fileFont);
            fileLabel->setStyleSheet(QStringLiteral("color: #263548; border: 0;"));
            firstLine->addWidget(fileLabel, 1);
            auto* serverLabel = new QLabel(tr("Server: %1").arg(U2Q(task->serverName())), taskCard);
            serverLabel->setStyleSheet(QStringLiteral("color: #607086; border: 0;"));
            firstLine->addWidget(serverLabel);
            detailsLayout->addLayout(firstLine);

            auto* progressLayout = new QHBoxLayout;
            auto* progressBar = new QProgressBar(taskCard);
            progressBar->setRange(0, 100);
            progressBar->setTextVisible(false);
            progressBar->setFixedHeight(8);
            progressLayout->addWidget(progressBar, 1);
            auto* percentLabel = new QLabel(taskCard);
            percentLabel->setStyleSheet(QStringLiteral("color: #3f566d; border: 0;"));
            progressLayout->addWidget(percentLabel);
            auto* transferredLabel = new QLabel(taskCard);
            transferredLabel->setStyleSheet(QStringLiteral("color: #7c8999; border: 0;"));
            progressLayout->addWidget(transferredLabel);
            detailsLayout->addLayout(progressLayout);

            auto* statusLabel = new QLabel(taskCard);
            QFont statusFont = statusLabel->font();
            statusFont.setBold(true);
            statusLabel->setFont(statusFont);
            detailsLayout->addWidget(statusLabel);
            taskLayout->addLayout(detailsLayout, 1);

            taskWidgets_.insert(task.get(), { progressBar, percentLabel, transferredLabel, statusLabel });
            updateTask(task.get());

            auto* codesButton = new QPushButton(tr("Codes"), taskCard);
            connect(codesButton, &QPushButton::clicked, this,
                    [this, session, task] { emit showCodesRequested(session.get(), task.get()); });
            taskLayout->addWidget(codesButton);
            auto* removeButton = new QToolButton(taskCard);
            removeButton->setText(QString::fromUtf8("\xc3\x97"));
            removeButton->setToolTip(tr("Remove"));
            removeButton->setFixedSize(32, 32);
            removeButton->setStyleSheet(QStringLiteral("QToolButton { background: #fff1f2; font-size: 18px; }"
                                                       "QToolButton:hover { background: #ffe4e7; }"));
            connect(removeButton, &QToolButton::clicked, this,
                    [this, session, task] { emit removeTaskRequested(session.get(), task.get()); });
            taskLayout->addWidget(removeButton);

            taskCard->pressed = [this, session, task](Qt::MouseButton button, const QPoint& position) {
                selectedSession_ = session.get();
                selectedTask_ = task.get();
                if (button == Qt::RightButton) {
                    emit contextMenuRequested(session.get(), task.get(), position);
                }
            };
            taskCard->doubleClicked = [this, session, task] { emit showCodesRequested(session.get(), task.get()); };
            sessionLayout->addWidget(taskCard);
        }
        contentsLayout_->addWidget(sessionCard);
    }
    contentsLayout_->addStretch();
}

void UploadSessionListWidget::hideSession(UploadSession* session) {
    hiddenSessions_.insert(session);
    if (selectedSession_ == session) {
        selectedSession_ = nullptr;
        selectedTask_ = nullptr;
    }
    refresh();
}

void UploadSessionListWidget::hideTask(UploadTask* task) {
    hiddenTasks_.insert(task);
    if (selectedTask_ == task) {
        selectedTask_ = nullptr;
    }
    refresh();
}

void UploadSessionListWidget::selectSession(UploadSession* session) {
    selectedSession_ = session;
    selectedTask_ = nullptr;
    verticalScrollBar()->setValue(verticalScrollBar()->minimum());
}

std::shared_ptr<UploadSession> UploadSessionListWidget::selectedSession() const {
    return findSession(selectedSession_);
}

std::shared_ptr<UploadTask> UploadSessionListWidget::selectedTask() const { return findTask(selectedTask_); }

void UploadSessionListWidget::attachSession(UploadSession* session) {
    if (!session) {
        return;
    }
    for (int i = 0; i < session->taskCount(); ++i) {
        const auto task = session->getTask(i);
        task->setOnUploadProgressCallback([this](UploadTask* changedTask) { queueTaskUpdate(changedTask); });
        task->setOnStatusChangedCallback([this](UploadTask* changedTask) { queueTaskUpdate(changedTask); });
    }
}

void UploadSessionListWidget::queueRefresh() {
    QMetaObject::invokeMethod(
        this,
        [this] {
            if (!refreshQueued_) {
                refreshQueued_ = true;
                QMetaObject::invokeMethod(this, &UploadSessionListWidget::refresh, Qt::QueuedConnection);
            }
        },
        Qt::QueuedConnection);
}

void UploadSessionListWidget::queueTaskUpdate(UploadTask* task) {
    if (!task) {
        return;
    }
    QMetaObject::invokeMethod(
        this,
        [this, task] {
            pendingTaskUpdates_.insert(task);
            if (!taskUpdateQueued_) {
                taskUpdateQueued_ = true;
                QMetaObject::invokeMethod(this, &UploadSessionListWidget::flushTaskUpdates, Qt::QueuedConnection);
            }
        },
        Qt::QueuedConnection);
}

void UploadSessionListWidget::flushTaskUpdates() {
    taskUpdateQueued_ = false;
    const QSet<UploadTask*> tasks = std::exchange(pendingTaskUpdates_, { });
    for (UploadTask* task : tasks) {
        if (findTask(task)) {
            updateTask(task);
        }
    }
}

void UploadSessionListWidget::updateTask(UploadTask* task) {
    const auto widgets = taskWidgets_.constFind(task);
    if (widgets == taskWidgets_.cend()) {
        return;
    }

    const auto* progress = task->progress();
    const qint64 total = std::max<qint64>(0, progress->totalUpload > 0 ? progress->totalUpload : task->getDataLength());
    const qint64 uploaded = std::max<qint64>(0, progress->uploaded);
    const int percent = task->status() == UploadTask::StatusFinished
        ? 100
        : (total > 0 ? std::clamp<int>(static_cast<int>(uploaded * 100 / total), 0, 100) : 0);
    widgets->ProgressBar->setValue(percent);
    widgets->PercentLabel->setText(QStringLiteral("%1%").arg(percent));
    widgets->TransferredLabel->setText(QStringLiteral("%1 / %2").arg(sizeText(uploaded), sizeText(total)));
    widgets->StatusLabel->setText(statusText(task->status()));
    widgets->StatusLabel->setStyleSheet(QStringLiteral("color: %1; border: 0;").arg(statusColor(task->status())));
}

std::shared_ptr<UploadSession> UploadSessionListWidget::findSession(UploadSession* session) const {
    if (!uploadManager_ || !session) {
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

std::shared_ptr<UploadTask> UploadSessionListWidget::findTask(UploadTask* task) const {
    if (!uploadManager_ || !task) {
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
