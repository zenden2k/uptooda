#include "UploadListModel.h"

#include "Core/Upload/UploadSession.h"
#include "Core/Upload/FileUploadTask.h"
#include "Core/ServiceLocator.h"
#include "Core/i18n/Translator.h"
#include "Core/Upload/UrlShorteningTask.h"

UploadListModel::UploadListModel(const std::shared_ptr<UploadSession>& session) {
    int n = session->taskCount();
    for (int i = 0; i < n; i++) {
        auto task = session->getTask(i);
        auto fileTask = std::dynamic_pointer_cast<FileUploadTask>(task);
        using namespace std::placeholders;
        task->setOnUploadProgressCallback([this](auto && PH1) { onTaskUploadProgress(std::forward<decltype(PH1)>(PH1)); });
        task->setOnStatusChangedCallback([this](auto && PH1) { onTaskStatusChanged(std::forward<decltype(PH1)>(PH1)); });
        task->addTaskFinishedCallback([this](auto && PH1, auto && PH2) { onTaskFinished(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });
        task->addChildTaskAddedCallback([this](auto && PH1) { onChildTaskAdded(std::forward<decltype(PH1)>(PH1)); });
        auto *sd = new UploadListItem();
        sd->tableRow = i;
        sd->setStatusText(TR("In queue"));
        sd->setFileName(U2W(fileTask->getFileName()));
        sd->setDisplayName(U2W(fileTask->getDisplayName()));
        sd->setServerName(U2W(fileTask->serverProfile().serverName()));
        task->setUserData(sd);
        items_.push_back(sd);
    }
}

UploadListModel::~UploadListModel() {
    for (auto* it : items_) {
        delete it;
    }
}

CString UploadListModel::getItemText(int row, int column) const {
    if (row < 0 || row >= static_cast<int>(items_.size())) {
        return {};
    }
    const UploadListItem* serverData = items_[row];
    if (column == 0) {
        return serverData->displayName() + _T(" [") + serverData->serverName() + _T("]");
    } 
    if (column == 1) {
        return serverData->statusText();
    } 
    if (column == 2) {
        return serverData->thumbStatusText();
    }
    return {};
}

COLORREF UploadListModel::getItemTextColor(size_t row, int column) const {
    if (column == 1 && row < items_.size()) {
        const UploadListItem& serverData = *items_[row];
        return serverData.color();
    }
    return GetSysColor(COLOR_WINDOWTEXT);
}

size_t UploadListModel::getCount() const {
    return items_.size();
}

void UploadListModel::notifyRowChanged(size_t row) {
    if (row < items_.size() && rowChangedCallback_) {
        rowChangedCallback_(row);
    }
}

UploadListItem* UploadListModel::getDataByIndex(size_t row) const {
    if (row >= items_.size()) {
        return nullptr;
    }
    return items_[row];
}

void UploadListModel::setOnRowChangedCallback(std::function<void(size_t)> callback) {
    rowChangedCallback_ = std::move(callback);
}

void UploadListModel::resetData() {
    for (auto& it : items_) {
        it->clearInfo();
    }
}

// This callback is being executed in worker thread
void UploadListModel::onTaskUploadProgress(UploadTask* task) {
    auto* fps = static_cast<UploadListItem*>(task->role() == UploadTask::DefaultRole ? task->userData() : task->parentTask()->userData());
    if (!fps) {
        return;
    }
    auto* fileTask = dynamic_cast<FileUploadTask*>(task);
    if (fileTask) {
        bool isThumb = task->role() == UploadTask::ThumbRole;
        int percent = 0;
        UploadProgress* progress = task->progress();
        if (progress->totalUpload) {
            percent = static_cast<int>(100 * ((float)progress->uploaded) / progress->totalUpload);
        }
        CString uploadSpeed = U2W(progress->speed);
        CString progressText;
        progressText.Format(TR("%s of %s (%d%%) %s"), (LPCTSTR)U2W(IuCoreUtils::FileSizeToString(progress->uploaded)),
            (LPCTSTR)U2W(IuCoreUtils::FileSizeToString(progress->totalUpload)), percent, uploadSpeed.GetString());

        if (isThumb) {
            fps->setThumbStatusText(progressText);
        } else {
            fps->setStatusText(progressText);
        }
        notifyRowChanged(fps->tableRow);
    }
}

void UploadListModel::onTaskStatusChanged(UploadTask* task) {
    UploadProgress* progress = task->progress();
    auto* fps = static_cast<UploadListItem*>(task->role() == UploadTask::DefaultRole ? task->userData() : task->parentTask()->userData());
    if (!fps) {
        return;
    }

    auto* fileTask = dynamic_cast<FileUploadTask*>(task);
    if (fileTask) {
        CString statusText = IuCoreUtils::Utf8ToWstring(progress->statusText).c_str();

        bool isThumb = task->role() == UploadTask::ThumbRole;
        if (isThumb) {
            fps->setThumbStatusText(statusText);
        } else {
            fps->setStatusText(statusText);
        }
        notifyRowChanged(fps->tableRow);
    }

    auto* urlTask = dynamic_cast<UrlShorteningTask*>(task);
    if (urlTask) {
        UploadTask* parentTask = urlTask->parentTask();
        if (urlTask->isFinished() && parentTask && parentTask->isFinished()) {
            CString statusText = U2W(parentTask->progress()->statusText);
            fps->setStatusText(statusText);

            notifyRowChanged(fps->tableRow);
        }
    }
}

// This callback is being executed in worker thread
void UploadListModel::onTaskFinished(UploadTask* task, bool ok) {
    auto* fileTask = dynamic_cast<FileUploadTask*>(task);
    if (!fileTask) {
        return;
    }

    if (fileTask->role() == UploadTask::ThumbRole) {
        auto* fps = static_cast<UploadListItem*>(task->parentTask()->userData());
        if (!fps) {
            return;
        }
        fps->setThumbStatusText(TR("Finished"));
        notifyRowChanged(fps->tableRow);
    } else if (fileTask->role() == UploadTask::DefaultRole ){
        auto* fps = static_cast<UploadListItem*>(task->userData());
        if (fileTask->status() == UploadTask::StatusFinished) {
            fps->setColor(RGB(34, 150, 16));// green
        }
        else if (fileTask->status() == UploadTask::StatusFailure) {
            fps->setColor(RGB(255, 0, 0)); // red
        }
    }
}

void UploadListModel::onChildTaskAdded(UploadTask* child) {
    if (child->role() == UploadTask::UrlShorteningRole) {
        auto* fps = static_cast<UploadListItem*>(child->parentTask()->userData());
        fps->setStatusText(TR("Shortening link..."));
        notifyRowChanged(fps->tableRow);
    }
    using namespace std::placeholders;
    child->addTaskFinishedCallback([this](auto && PH1, auto && PH2) { onTaskFinished(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2)); });
    child->setOnUploadProgressCallback([this](auto && PH1) { onTaskUploadProgress(std::forward<decltype(PH1)>(PH1)); });
    child->setOnStatusChangedCallback([this](auto && PH1) { onTaskStatusChanged(std::forward<decltype(PH1)>(PH1)); });
}
