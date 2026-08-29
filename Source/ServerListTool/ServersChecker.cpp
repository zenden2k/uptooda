#include "ServersChecker.h"

#include "Core/Upload/UploadSession.h"
#include "Core/Settings/BasicSettings.h"
#include "Core/AppRuntimeInfo.h"
#include "Core/Upload/FileUploadTask.h"
#include "Core/Upload/UrlShorteningTask.h"
#include "Core/Upload/UploadManager.h"
#include "ServersCheckerModel.h"
#include "Func/WinUtils.h"
#include "Core/Utils/CryptoUtils.h"
#include "ServerListTool/Helpers.h"
#include "Core/ServiceLocator.h"
#include "CheckShortUrlTask.h"

namespace ServersListTool {

ServersChecker::ServersChecker(ServersCheckerModel* model, UploadManager* uploadManager, std::shared_ptr<INetworkClientFactory> networkClientFactory) :
    model_(model),
    uploadManager_(uploadManager),
    useAccounts_(true),
    checkImageServers_(true),
    checkFileServers_(true),
    checkURLShorteners_(true),
    networkClientFactory_(std::move(networkClientFactory)),
    linksToCheck_(0)
{
    needStop_ = false;
    isRunning_ = false;

    fileDownloader_ = std::make_unique<CFileDownloader>(networkClientFactory_, AppRuntimeInfo::instance()->tempDirectory());
    fileDownloader_->setOnFileFinishedCallback([this](bool ok, int statusCode, auto && downloadItem) {
        onFileFinished(
            ok,
            statusCode,
            std::forward<decltype(downloadItem)>(downloadItem)
        );
    });
}

bool ServersChecker::start(const std::string& testFileName, const std::string& testUrl) {
    linksToCheck_ = 0;
    auto* settings = ServiceLocator::instance()->settings<BasicSettings>();
    GetFileInfo(U2W(testFileName), &m_sourceFileInfo);
    srcFileHash_ = IuCoreUtils::CryptoUtils::CalcMD5HashFromFile(testFileName);
    uploadSession_ = std::make_shared<UploadSession>(false);

    using namespace std::placeholders;
    uploadSession_->addSessionFinishedCallback([this](auto && session) {
        onSessionFinished(std::forward<decltype(session)>(session));
    });
    int taskCount = 0;
    for (size_t i = 0; i < model_->getCount(); i++) {
        ServerData* item = model_->getDataByIndex(i);
        if (item->skip) {
            continue;
        }

        //uploader.ShouldStop = &m_NeedStop;
        const CUploadEngineData *ue = item->ued;
        if (!(item->serverType == CUploadEngineData::TypeImageServer && checkImageServers_) &&
            !(item->serverType == CUploadEngineData::TypeFileServer && checkFileServers_) &&
            !(item->serverType == CUploadEngineData::TypeUrlShorteningServer && checkURLShorteners_)) {
            continue;
        }

        ServerSettingsStruct  ss;
        auto serverIt = settings->ServersSettings.find(ue->Name);
        if (serverIt != settings->ServersSettings.end()) {
            std::map <std::string, ServerSettingsStruct>::const_iterator it = serverIt->second.begin();

            if (!useAccounts_) {
                if (it != serverIt->second.end()) {
                    ss = it->second;
                }
            } else {
                if (it != serverIt->second.end()) {
                    if (it->first.empty()) {
                        ++it;
                        if (it != serverIt->second.end()) {
                            ss = it->second;
                        }
                    } else {
                        ss = it->second;
                    }
                }
            }
        }

        if ((ue->NeedAuthorization == CUploadEngineData::naObligatory || (onlyAccs_ && ue->NeedAuthorization)) && ss.authData.Login.empty()) {
            item->setStatusText("No account is set");
            model_->notifyRowChanged(i);
            continue;
        }
        if (onlyAccs_ && !ue->NeedAuthorization) {
            item->setStatusText("skipped");
            model_->notifyRowChanged(i);
            continue;
        }
        ServerProfile serverProfile(ue->Name);
        serverProfile.setShortenLinks(false);
        serverProfile.setProfileName(ss.authData.Login);

        std::shared_ptr<UploadTask> task;
        if (item->serverType == CUploadEngineData::TypeImageServer || item->serverType == CUploadEngineData::TypeFileServer) {
            task = std::make_shared<FileUploadTask>(testFileName, IuCoreUtils::ExtractFileName(testFileName));

        } else if (item->serverType == CUploadEngineData::TypeUrlShorteningServer) {
            task = std::make_shared<UrlShorteningTask>(testUrl);
        }

        if (task) {
            task->setServerProfile(serverProfile);
            task->setOnStatusChangedCallback([this](UploadTask* task) {
                onTaskStatusChanged(task);
            });
            task->addTaskFinishedCallback([this](UploadTask* task, bool ok) {
                onTaskFinished(task, ok);
            });
            auto userData = std::make_unique<UploadTaskUserData>();
            userData->rowIndex = i;
            task->setUserData(userData.get());
            uploadTaskUserDatas_.push_back(std::move(userData));
            uploadSession_->addTask(task);
            taskCount++;
        }
    }
    if (taskCount) {
        isRunning_ = true;
        uploadManager_->addSession(uploadSession_);
    } else {
        isRunning_ = false;
        processFinished();
    }
    return true;
}

void ServersChecker::stop() {
    needStop_ = true;
    if (fileDownloader_ && fileDownloader_->isRunning()) {
        fileDownloader_->stop();
    }
    if (uploadSession_) {
        uploadSession_->stop();
    }
    std::lock_guard<std::mutex> lk(scheduledTasksMutex_);
    for (auto& task : scheduledTasks_) {
        task->cancel();
    }
}

bool ServersChecker::isRunning() const {
    return isRunning_ || (fileDownloader_ && fileDownloader_->isRunning());
}

void ServersChecker::setUseAccounts(bool value) {
    useAccounts_ = value;
}

void ServersChecker::setOnlyAccs(bool value) {
    onlyAccs_ = value;
}

void ServersChecker::setCheckImageServers(bool value) {
    checkImageServers_ = value;
}

void ServersChecker::setCheckFileServers(bool value) {
    checkFileServers_ = value;
}

void ServersChecker::setCheckUrlShorteners(bool value) {
    checkURLShorteners_ = value;
}

void ServersChecker::setOnFinishedCallback(std::function<void()> callback) {
    onFinishedCallback_ = std::move(callback);
}

void ServersChecker::onFileFinished(bool ok, int /*statusCode*/, const CFileDownloader::DownloadFileListItem& it)
{
    size_t serverId = reinterpret_cast<size_t>(it.id) / 10;
    size_t fileId = reinterpret_cast<size_t>(it.id) % 10;

    ServerData& serverData = *model_->getDataByIndex(serverId);
    ++serverData.filesChecked;
    --serverData.fileToCheck;
    CString fileName = Utf8ToWstring(it.fileName).c_str();

    if (!ok) {
        serverData.stars[fileId] = 1;
        serverData.setLinkInfo(static_cast<int>(fileId), "Cannot download file");
    }
    if (IuCoreUtils::CryptoUtils::CalcMD5HashFromFile(W2U(fileName)) == srcFileHash_) {
        if (fileId == 0)
            serverData.stars[fileId] = 5;
        else serverData.stars[fileId] = 4;
        serverData.setLinkInfo(static_cast<int>(fileId), "Identical file");
    } else {
        Helpers::MyFileInfo mfi;

        CString report = GetFileInfo(fileName, &mfi);

        if (fileId < 2) // is thumb or image
        {
            if (mfi.mimeType.Find(_T("image/")) >= 0) {
                if (fileId == 0 && (mfi.width != m_sourceFileInfo.width || mfi.height != m_sourceFileInfo.height))
                    serverData.stars[fileId] = 2;
                else
                    serverData.stars[fileId] = fileId == 0 ? 4 : 5;
            } else serverData.stars[fileId] = 1;
        } else serverData.stars[fileId] = 5;

        serverData.setLinkInfo(static_cast<int>(fileId), W2U(report));
    }
    if (serverData.fileToCheck == 0) {
        serverData.finished = true;
    }

    markServer(serverId);

    --linksToCheck_;
    processFinished();
}

void ServersChecker::checkShortUrl(UploadTask* task) {
    auto* urlTask = dynamic_cast<UrlShorteningTask*>(task);
    if (!urlTask) {
        return;
    }
    auto* userData = static_cast<UploadTaskUserData*>(task->userData());
    ServerData* data = model_->getDataByIndex(userData->rowIndex);

    auto checkTask = std::make_shared<CheckShortUrlTask>(networkClientFactory_, task->uploadResult()->directUrl, urlTask->getUrl());
    checkTask->onTaskFinished.connect([this, rowIndex = userData->rowIndex, data](auto* task, bool ok) {
        if (ok) {
            data->setStrMark("Good link");
        }
        ++data->filesChecked;
        data->stars[0] = ok ? 5 : 1;
        data->finished = true;
        markServer(rowIndex);
        std::lock_guard<std::mutex> lk(scheduledTasksMutex_);
        auto it = std::remove_if(scheduledTasks_.begin(), scheduledTasks_.end(), [task](auto&& a) { return a.get() == task; });
        scheduledTasks_.erase(it, scheduledTasks_.end());
    });
    {
        std::lock_guard<std::mutex> lk(scheduledTasksMutex_);
        scheduledTasks_.push_back(checkTask);
    }

    ServiceLocator::instance()->taskDispatcher()->postTask(checkTask);
}

void ServersChecker::onTaskFinished(UploadTask* task, bool ok) {
    const CUploadEngineData* ue = task->serverProfile().uploadEngineData();
    auto* userData = static_cast<UploadTaskUserData*>(task->userData());
    size_t i = userData->rowIndex;
    ServerData& data = *model_->getDataByIndex(i);
    if (task->status() == UploadTask::StatusStopped) {
        data.setStatusText(std::string());
        return;
    }
    if (ok) {
        std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        UploadResult* result = task->uploadResult();
        data.timeElapsed = std::chrono::duration_cast<std::chrono::milliseconds>(end - userData->startTime).count();

        if (task->type() == UploadTask::TypeFile) {
            std::string imgUrl = result->getDirectUrl();
            std::string thumbUrl = result->getThumbUrl();
            std::string viewUrl = result->getDownloadUrl();
            int nFilesToCheck = 0;
            if (!imgUrl.empty()) {
                ++linksToCheck_;
                fileDownloader_->addFile(result->getDirectUrl(), reinterpret_cast<void*>(static_cast<intptr_t>(i * 10)));
                nFilesToCheck++;
                data.setDirectUrl(imgUrl);

            } else {
                if (!ue->ImageUrlTemplate.empty()) {
                    /*if (!ue->UsingPlugin)
                    m_CheckedServersMap[i].filesChecked++;*/
                    data.setDirectUrlInfo("<empty>");
                }
            }

            if (!thumbUrl.empty()) {
                nFilesToCheck++;
                ++linksToCheck_;
                fileDownloader_->addFile(result->getThumbUrl(), reinterpret_cast<void*>(static_cast<intptr_t>(i * 10 + 1)));

                data.setThumbUrl(thumbUrl);
            } else {
                if (!ue->ThumbUrlTemplate.empty()) {
                    /*if (!ue->UsingPlugin)
                    m_CheckedServersMap[i].filesChecked++;*/
                    data.setThumbUrlInfo("<empty>");
                }
            }

            if (!viewUrl.empty()) {
                nFilesToCheck++;
                ++linksToCheck_;
                fileDownloader_->addFile(result->getDownloadUrl(), reinterpret_cast<void*>(static_cast<intptr_t>(i * 10 + 2)));
                data.setViewUrl(viewUrl);
            } else {

                if (!ue->DownloadUrlTemplate.empty()) {
                    /*if (!ue->UsingPlugin)
                    m_CheckedServersMap[i].filesChecked++;*/
                    data.setViewUrlInfo("<empty>");
                }

            }
            data.fileToCheck = nFilesToCheck;
            fileDownloader_->start();
            if (nFilesToCheck)
                data.setStatusText("Checking links");
            else {
                data.finished = true;
            }
        } else if (task->type() == UploadTask::TypeUrl) {
            std::string shortURL = result->getDirectUrl();
            if (!shortURL.empty()) {
                data.setDirectUrl(shortURL);
                data.setStatusText("Checking short link");
                checkShortUrl(task);
            } else {
                data.finished = true;
                data.setDirectUrlInfo("<empty>");
            }
        }


    } else {
        data.finished = true;
    }
    markServer(i);
}

void ServersChecker::onSessionFinished(UploadSession* session) {
    isRunning_ = false;
    LOG(INFO) << "Uploader has finished";
    processFinished();
}

void ServersChecker::onTaskStatusChanged(UploadTask* task) {
    auto* userData = static_cast<UploadTaskUserData*>(task->userData());
    size_t i = userData->rowIndex;
    if (task->status() == UploadTask::StatusRunning) {
        userData->startTime = std::chrono::steady_clock::now();
        ServerData* data = model_->getDataByIndex(i);
        data->setStatusText(task->type() == UploadTask::TypeUrl ? "Shortening link..." : "Uploading file...");
        model_->notifyRowChanged(i);
    } /*else if (task->status() == UploadTask::StatusFinished) {

    } else if (task->status() == UploadTask::StatusFailure) {

    }*/
}

void ServersChecker::markServer(size_t id)
{
    ServerData& serverData = *model_->getDataByIndex(id);
    if (serverData.finished) {
        int mark = 0;
        std::vector<int> v;
        std::copy_if(std::begin(serverData.stars), std::end(serverData.stars), std::back_inserter(v), [](int i) { return i > 0; });

        auto it = std::min_element(v.begin(), v.end());
        int count = serverData.filesChecked;
        if (count && it != v.end()) {
            mark = *it;
        }

        CString timeLabel;
        timeLabel.Format(_T("%02d:%02d"), static_cast<int>(serverData.timeElapsed / 60000), static_cast<int>(serverData.timeElapsed / 1000 % 60));
        serverData.setTimeStr(W2U(timeLabel));

        if (mark == 5) {
            serverData.setStrMark("EXCELLENT");
            serverData.color = RGB(0, 255, 50);
        } else if (mark >= 4) {
            serverData.setStrMark("GOOD");
            serverData.color = RGB(145, 213, 0);
        } else if (mark > 1) {
            serverData.setStrMark("ACCEPTABLE");
            serverData.color = RGB(245, 233, 66);
        } else {
            serverData.setStrMark("FAILED");
            serverData.color = RGB(198, 0, 0);
        }
    }

    //ServiceLocator::instance()->taskRunner()->runInGuiThread([&] {
        model_->notifyRowChanged(id);
    //});
}

void ServersChecker::processFinished() {
    if (!isRunning_ && (!linksToCheck_ || !fileDownloader_ || !fileDownloader_->isRunning())) {
        if (onFinishedCallback_) {
            onFinishedCallback_();
        }
    }
}

}
