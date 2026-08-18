#include "QmlUploadSessionModel.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QImageReader>
#include <QMetaObject>
#include <QUrl>

#include <algorithm>

#include "Core/AppRuntimeInfo.h"
#include "Core/CommonDefs.h"
#include "Core/Upload/FileUploadTask.h"
#include "Core/Upload/UploadManager.h"
#include "Core/Upload/UploadSession.h"

namespace {
QString statusText(UploadTask::Status status) {
    switch (status) {
    case UploadTask::StatusInQueue:
        return QCoreApplication::translate("QmlUploadSessionModel", "In queue");
    case UploadTask::StatusRunning:
        return QCoreApplication::translate("QmlUploadSessionModel", "Uploading");
    case UploadTask::StatusStopped:
        return QCoreApplication::translate("QmlUploadSessionModel", "Stopped");
    case UploadTask::StatusFinished:
        return QCoreApplication::translate("QmlUploadSessionModel", "Finished");
    case UploadTask::StatusFailure:
        return QCoreApplication::translate("QmlUploadSessionModel", "Failed");
    case UploadTask::StatusPostponed:
        return QCoreApplication::translate("QmlUploadSessionModel", "Postponed");
    case UploadTask::StatusWaitingChildren:
        return QCoreApplication::translate("QmlUploadSessionModel", "Processing");
    }
    return { };
}

QString statusKey(UploadTask::Status status) {
    switch (status) {
    case UploadTask::StatusInQueue:
        return QStringLiteral("queued");
    case UploadTask::StatusRunning:
        return QStringLiteral("running");
    case UploadTask::StatusStopped:
        return QStringLiteral("stopped");
    case UploadTask::StatusFinished:
        return QStringLiteral("finished");
    case UploadTask::StatusFailure:
        return QStringLiteral("failure");
    case UploadTask::StatusPostponed:
        return QStringLiteral("postponed");
    case UploadTask::StatusWaitingChildren:
        return QStringLiteral("processing");
    }
    return QStringLiteral("unknown");
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
}

QmlUploadSessionModel::QmlUploadSessionModel(UploadManager* uploadManager, QObject* parent) :
    QAbstractListModel(parent), UploadManager_(uploadManager) {
    UploadManager_->setOnSessionAddedCallback([this](UploadSession* session) {
        QMetaObject::invokeMethod(this, [this, session] {
            attachSession(session);
            resetModel();
        });
    });
    UploadManager_->setOnTaskAddedCallback([this](UploadTask*) { queueRefresh(); });
}

QmlUploadSessionModel::~QmlUploadSessionModel() { detach(); }

int QmlUploadSessionModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid() || !UploadManager_) {
        return 0;
    }
    int count = 0;
    for (int i = 0; i < UploadManager_->sessionCount(); ++i) {
        if (!HiddenSessions_.contains(objectId(UploadManager_->session(i).get()))) {
            ++count;
        }
    }
    return count;
}

QVariant QmlUploadSessionModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid() || !UploadManager_) {
        return { };
    }
    std::shared_ptr<UploadSession> session;
    int visibleIndex = 0;
    for (int i = UploadManager_->sessionCount() - 1; i >= 0; --i) {
        auto candidate = UploadManager_->session(i);
        if (HiddenSessions_.contains(objectId(candidate.get()))) {
            continue;
        }
        if (visibleIndex++ == index.row()) {
            session = candidate;
            break;
        }
    }
    if (!session) {
        return { };
    }

    QStringList servers;
    int visibleTasks = 0;
    for (int i = 0; i < session->taskCount(); ++i) {
        auto task = session->getTask(i);
        if (HiddenTasks_.contains(objectId(task.get()))) {
            continue;
        }
        ++visibleTasks;
        const QString server = U2Q(task->serverName());
        if (!server.isEmpty() && !servers.contains(server)) {
            servers.append(server);
        }
    }

    switch (role) {
    case SessionIdRole:
        return objectId(session.get());
    case FileCountRole:
        return visibleTasks;
    case ServerNamesRole:
        return servers.join(QStringLiteral(", "));
    case ServerIconRole:
        return iconForServer(servers.isEmpty() ? std::string() : Q2U(servers.first()));
    case TasksRole:
        return taskList(session);
    default:
        return { };
    }
}

QHash<int, QByteArray> QmlUploadSessionModel::roleNames() const {
    return { { SessionIdRole, "sessionId" },
             { FileCountRole, "fileCount" },
             { ServerNamesRole, "serverNames" },
             { ServerIconRole, "serverIcon" },
             { TasksRole, "tasks" } };
}

bool QmlUploadSessionModel::moveTask(const QString& sessionId, int fromIndex, int toIndex) {
    auto session = findSession(sessionId);
    if (!session) {
        return false;
    }
    QStringList order;
    const QVariantList tasks = taskList(session);
    for (const QVariant& task : tasks) {
        order.append(task.toMap().value(QStringLiteral("taskId")).toString());
    }
    if (fromIndex < 0 || toIndex < 0 || fromIndex >= order.size() || toIndex >= order.size() || fromIndex == toIndex) {
        return false;
    }
    order.move(fromIndex, toIndex);
    TaskOrders_[sessionId] = order;
    queueRefresh();
    return true;
}

void QmlUploadSessionModel::detach() {
    if (!UploadManager_) {
        return;
    }
    UploadManager_->setOnSessionAddedCallback({ });
    UploadManager_->setOnTaskAddedCallback({ });
    for (int i = 0; i < UploadManager_->sessionCount(); ++i) {
        auto session = UploadManager_->session(i);
        for (int j = 0; j < session->taskCount(); ++j) {
            session->getTask(j)->setOnUploadProgressCallback({ });
            session->getTask(j)->setOnStatusChangedCallback({ });
        }
    }
    UploadManager_ = nullptr;
}

std::shared_ptr<UploadSession> QmlUploadSessionModel::findSession(const QString& sessionId) const {
    if (!UploadManager_) {
        return { };
    }
    for (int i = 0; i < UploadManager_->sessionCount(); ++i) {
        auto session = UploadManager_->session(i);
        if (objectId(session.get()) == sessionId) {
            return session;
        }
    }
    return { };
}

std::shared_ptr<UploadTask> QmlUploadSessionModel::findTask(const QString& sessionId, const QString& taskId) const {
    auto session = findSession(sessionId);
    if (!session) {
        return { };
    }
    for (int i = 0; i < session->taskCount(); ++i) {
        auto task = session->getTask(i);
        if (objectId(task.get()) == taskId) {
            return task;
        }
    }
    return { };
}

void QmlUploadSessionModel::hideSession(const QString& sessionId) {
    HiddenSessions_.insert(sessionId);
    resetModel();
}

void QmlUploadSessionModel::hideTask(const QString& taskId) {
    HiddenTasks_.insert(taskId);
    refresh();
}

void QmlUploadSessionModel::refresh() {
    const int count = rowCount();
    if (count > 0) {
        emit dataChanged(index(0, 0), index(count - 1, 0));
    }
}

void QmlUploadSessionModel::resetModel() {
    beginResetModel();
    endResetModel();
}

QString QmlUploadSessionModel::objectId(const void* object) {
    return QString::number(reinterpret_cast<quintptr>(object), 16);
}

QVariantList QmlUploadSessionModel::taskList(const std::shared_ptr<UploadSession>& session) const {
    QVariantList result;
    QList<std::shared_ptr<UploadTask>> tasks;
    for (int i = 0; i < session->taskCount(); ++i) {
        auto task = session->getTask(i);
        if (!HiddenTasks_.contains(objectId(task.get()))) {
            tasks.append(task);
        }
    }
    const QStringList order = TaskOrders_.value(objectId(session.get()));
    if (!order.isEmpty()) {
        std::stable_sort(tasks.begin(), tasks.end(), [&order](const auto& left, const auto& right) {
            int leftIndex = order.indexOf(objectId(left.get()));
            int rightIndex = order.indexOf(objectId(right.get()));
            leftIndex = leftIndex < 0 ? order.size() : leftIndex;
            rightIndex = rightIndex < 0 ? order.size() : rightIndex;
            return leftIndex < rightIndex;
        });
    }
    for (const auto& task : tasks) {
        result.append(taskData(task));
    }
    return result;
}

QVariantMap QmlUploadSessionModel::taskData(const std::shared_ptr<UploadTask>& task) const {
    QVariantMap result;
    result[QStringLiteral("taskId")] = objectId(task.get());
    result[QStringLiteral("fileName")] = U2Q(task->title());
    result[QStringLiteral("server")] = U2Q(task->serverName());
    result[QStringLiteral("status")] = statusText(task->status());
    result[QStringLiteral("statusKey")] = statusKey(task->status());

    const auto* progress = task->progress();
    const qint64 total = std::max<qint64>(0, progress->totalUpload > 0 ? progress->totalUpload : task->getDataLength());
    const qint64 uploaded = std::max<qint64>(0, progress->uploaded);
    const int percent = task->status() == UploadTask::StatusFinished
        ? 100
        : (total > 0 ? std::clamp<int>(static_cast<int>(uploaded * 100 / total), 0, 100) : 0);
    result[QStringLiteral("progress")] = percent;
    result[QStringLiteral("transferred")] = QStringLiteral("%1 / %2").arg(sizeText(uploaded), sizeText(total));
    result[QStringLiteral("serverIcon")] = iconForServer(task->serverName());

    if (auto fileTask = std::dynamic_pointer_cast<FileUploadTask>(task)) {
        const QString filePath = U2Q(fileTask->getFileName());
        result[QStringLiteral("filePath")] = filePath;
        QImageReader reader(filePath);
        const bool isImage = reader.canRead();
        result[QStringLiteral("isImage")] = isImage;
        result[QStringLiteral("thumbnail")]
            = isImage ? QUrl::fromLocalFile(filePath) : QUrl(QStringLiteral("qrc:/res/images.ico"));
    }
    return result;
}

QString QmlUploadSessionModel::iconForServer(const std::string& serverName) const {
    const QString path = U2Q(AppRuntimeInfo::instance()->dataDirectory()) + QStringLiteral("Favicons/")
        + U2Q(serverName).toLower() + QStringLiteral(".ico");
    return QFileInfo::exists(path) ? QUrl::fromLocalFile(path).toString() : QStringLiteral("qrc:/res/server.png");
}

void QmlUploadSessionModel::attachSession(UploadSession* session) {
    if (!session) {
        return;
    }
    for (int i = 0; i < session->taskCount(); ++i) {
        auto task = session->getTask(i);
        task->setOnUploadProgressCallback([this](UploadTask*) { queueRefresh(); });
        task->setOnStatusChangedCallback([this](UploadTask*) { queueRefresh(); });
    }
}

void QmlUploadSessionModel::queueRefresh() {
    QMetaObject::invokeMethod(this, &QmlUploadSessionModel::refresh, Qt::QueuedConnection);
}
