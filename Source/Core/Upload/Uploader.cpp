/*

    Uptooda - free application for uploading images/files to the Internet

    Copyright 2007-2025 Sergey Svistunov (zenden2k@gmail.com)

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

*/

#include "Uploader.h"

#include <cmath>

#include "Core/Upload/FileUploadTask.h"
#include "Core/BasicConstants.h"
#include "Core/ServiceLocator.h"

CUploader::CUploader(const std::shared_ptr<INetworkClientFactory>& networkClientFactory)
{
    m_bShouldStop = false;
    m_CurrentStatus = stNone;
    m_CurrentEngine = nullptr;
    m_PrInfo.IsUploading = false;
    m_PrInfo.Total = 0;
    m_PrInfo.Uploaded = 0;
    isFatalServerError_ = false;
    m_NetworkClient = networkClientFactory->create();
}

CUploader::~CUploader()
{
}

void CUploader::Cleanup()
{
    m_CurrentEngine->setOnDebugMessageCallback(nullptr);
    m_CurrentEngine->setOnNeedStopCallback(nullptr);
    m_CurrentEngine->setOnStatusChangedCallback(nullptr);
    m_CurrentEngine->setOnErrorMessageCallback(nullptr);
}

int CUploader::pluginProgressFunc(INetworkClient* nc, int64_t dltotal, int64_t dlnow, int64_t ultotal, int64_t ulnow) {
    CUploader* uploader = this;
    auto networkClient = dynamic_cast<NetworkClient*>(nc);

    if (!networkClient)
        return 0;

    if (uploader->needStop())
        return -1;

    if (ultotal < 0 || ulnow < 0)
        return 0;

    if (ultotal != 0 && ulnow == 0 && uploader->m_CurrentStatus == stWaitingAnswer) {
        uploader->SetStatus(stUploading);
    }
    if (ultotal == ulnow) {
        uploader->m_PrInfo.IsUploading = false;
        uploader->m_PrInfo.Total = ultotal;
        uploader->m_PrInfo.Uploaded = ulnow;

        if (ultotal != 0 && uploader->m_CurrentStatus == stUploading) {
            uploader->SetStatus(stWaitingAnswer);
        }
    } else {
        uploader->m_PrInfo.IsUploading = true;
        uploader->m_PrInfo.Total = ultotal;
        uploader->m_PrInfo.Uploaded = ulnow;
        uploader->m_PrInfo.IsUploading = networkClient->currentActionType() == NetworkClient::ActionType::atUpload;
    }

    uploader->currentTask_->uploadProgress(uploader->m_PrInfo);

    if (uploader->onProgress_) {
        uploader->onProgress_(uploader, uploader->m_PrInfo);
    }
    return 0;
}

bool CUploader::UploadFile(const std::string& FileName, const std::string& displayFileName, int maxRetries)
{
    return Upload(std::make_shared<FileUploadTask>(FileName, displayFileName), maxRetries);
}

bool CUploader::Upload(const std::shared_ptr<UploadTask>& task, int maxRetries)
{
    isFatalServerError_ = false;
    if (!m_CurrentEngine) {
        Error(true, "Cannot proceed: m_CurrentEngine is NULL!");
        return false;
    }
    std::string FileName;
    currentTask_ = task;

    if (task->type() == UploadTask::TypeFile) {
        FileName = dynamic_cast<FileUploadTask*>(task.get())->getFileName();
        if ( FileName.empty() ) {
            Error(true, "Empty filename!");
            return false;
        }

        if (!IuCoreUtils::FileExists (FileName) ) {
            Error(true, "File \""+FileName+"\" doesn't exist!");
            return false;
        }
    }
    UploadTask* topLevelTask = task->parentTask() ? task->parentTask() : task.get();
    auto* topLevelFileTask = dynamic_cast<FileUploadTask*>(topLevelTask);
    std::string topLevelFileName;
    if (topLevelFileTask) {
        topLevelFileName = topLevelFileTask->getFileName();
    }
    m_PrInfo.IsUploading = false;
    m_PrInfo.Total = 0;
    m_PrInfo.Uploaded = 0;
    m_FileName = FileName;
    m_bShouldStop = false;
    if (onConfigureNetworkClient_) {
        onConfigureNetworkClient_(this, m_NetworkClient.get());
    }
    m_NetworkClient->setLogger(nullptr);

    m_CurrentEngine->setNetworkClient(m_NetworkClient.get());
    using namespace std::placeholders;
    m_CurrentEngine->setOnDebugMessageCallback([this](auto && PH1, auto && PH2) {
        DebugMessage(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2));
    });
    m_CurrentEngine->setOnNeedStopCallback([this] { return needStop(); });
    m_CurrentEngine->setOnStatusChangedCallback([this](auto && PH1, auto && PH2, auto && PH3) {
        SetStatus(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2), std::forward<decltype(PH3)>(PH3));
    });
    m_CurrentEngine->setOnErrorMessageCallback([this](auto && PH1) {
        ErrorMessage(std::forward<decltype(PH1)>(PH1));
    });
    m_CurrentEngine->setCurrentUploader(this);

    task->setCurrentUploadEngine(m_CurrentEngine);

    UploadParams uparams;

    ImageUploadParams imageUploadParams;
    auto& serverProfile = task->serverProfile();
    imageUploadParams = serverProfile.getImageUploadParams();

    ThumbCreatingParams& tcParams = imageUploadParams.getThumbRef();

    if (tcParams.ResizeMode == ThumbCreatingParams::trByBoth || tcParams.ResizeMode == ThumbCreatingParams::trByWidth) {
        uparams.thumbWidth = tcParams.Width;
    }
    if (tcParams.ResizeMode == ThumbCreatingParams::trByBoth || tcParams.ResizeMode == ThumbCreatingParams::trByHeight) {
        uparams.thumbHeight = tcParams.Height;
    }
    uparams.createThumbnail = imageUploadParams.CreateThumbs;
    uparams.addTextOnThumb = tcParams.AddImageSize;
    uparams.useServerSideThumbnail = imageUploadParams.UseServerThumbs;

    /*if (task->type() == UploadTask::TypeFile) {
        FileUploadTask* fileTask = dynamic_cast<FileUploadTask*>(task.get());
    }*/
    m_NetworkClient->setProgressCallback([this](auto && PH1, auto && PH2, auto && PH3, auto && PH4, auto && PH5) {
        return pluginProgressFunc(
            std::forward<decltype(PH1)>(PH1),
            std::forward<decltype(PH2)>(PH2),
            std::forward<decltype(PH3)>(PH3),
            std::forward<decltype(PH4)>(PH4),
            std::forward<decltype(PH5)>(PH5));
    });
    ResultCode EngineRes = ResultCode::Failure;
    int retryLimit = IuCoreUtils::Coalesce(task->retryLimit(), maxRetries, m_CurrentEngine->RetryLimit(), MAX_RETRIES_PER_FILE);

    int i = 0;
    do
    {
        if (needStop())
        {
            Cleanup();
            return false;
        }
        EngineRes = static_cast<ResultCode>(m_CurrentEngine->processTask(task, uparams));
        task->setCurrentUploadEngine(nullptr);

        switch (EngineRes) {
        case ResultCode::FatalServerError:
            isFatalServerError_ = true;
            Cleanup();
            return false;
        case ResultCode::TryAgain:
            if (i == retryLimit) {
                retryLimit++;
            }
            break;
        case ResultCode::FatalError:
            i = std::max(i, retryLimit - 1);
            break;
        default:
            break;
        }

        i++;
        if (needStop())
        {
            Cleanup();
            return false;
        }
        if (EngineRes < ResultCode::Success && i != retryLimit)
        {
            Error(false, "", etRepeating, i, task.get(), topLevelFileName);
        }
    } while (EngineRes < ResultCode::Success && i < retryLimit);

    if (EngineRes == ResultCode::Failure || EngineRes == ResultCode::FatalError) {
        Error(true, "", etRetriesLimitReached, -1, task.get(), topLevelFileName);
        Cleanup();
        return false;
    }
    if (task->type() == UploadTask::TypeFile || task->type() == UploadTask::TypeUrl) {
        UploadResult* result = task->uploadResult();
        result->directUrl = uparams.DirectUrl;
        result->downloadUrl = uparams.ViewUrl;
        if (result->thumbUrl.empty()) {
            result->thumbUrl = uparams.ThumbUrl;
        }
        result->deleteUrl = uparams.DeleteUrl;
        result->editUrl = uparams.EditUrl;

        return !(result->directUrl.empty() && result->downloadUrl.empty() && result->editUrl.empty());
    } else {
        return EngineRes == ResultCode::Success;
    }
}

void CUploader::setUploadEngine(const std::shared_ptr<CAbstractUploadEngine>& uploadEngine)
{
    m_CurrentEngine = uploadEngine;
}

void CUploader::SetStatus(StatusType status, int param1, const std::string& param)
{
    m_CurrentStatus = status;
    if (onStatusChanged_) {
        onStatusChanged_(this, status, param1, param);
    }
}

StatusType CUploader::GetStatus() const
{
    return m_CurrentStatus;
}

bool CUploader::isFatalError() const
{
    return isFatalServerError_;
}

std::shared_ptr<CAbstractUploadEngine> CUploader::getUploadEngine() const
{
    return m_CurrentEngine;
}

void CUploader::stop()
{
    m_bShouldStop = true;
}

bool CUploader::needStop()
{
    if (m_bShouldStop)
        return m_bShouldStop;
    if (currentTask_->stopSignal())
    {
        m_bShouldStop = true;
        return m_bShouldStop;
    }
    if (onNeedStop_) {
    m_bShouldStop = onNeedStop_();  // delegate call
    }
    return m_bShouldStop;
}

std::shared_ptr<UploadTask> CUploader::currentTask() const
{
    return currentTask_;
}

void CUploader::setOnNeedStopCallback(const std::function<bool()>& cb) {
    onNeedStop_ = cb;
}

void CUploader::setOnProgress(const std::function<void(CUploader*, InfoProgress)>& cb) {
    onProgress_ = cb;
}

void CUploader::setOnStatusChanged(const std::function<void(CUploader*, StatusType, int, std::string)>& cb) {
    onStatusChanged_ = cb;
}

void CUploader::setOnDebugMessage(const std::function<void(CUploader*, const std::string&, bool)>& cb) {
    onDebugMessage_ = cb;
}

void CUploader::setOnErrorMessage(const std::function<void(CUploader*, ErrorInfo)>& cb) {
    onErrorMessage_ = cb;
}

void CUploader::setOnConfigureNetworkClient(const std::function<void(CUploader*, INetworkClient*)>& cb) {
    onConfigureNetworkClient_ = cb;
}

void CUploader::DebugMessage(const std::string& message, bool isServerResponseBody)
{
    if (onDebugMessage_) {
        onDebugMessage_(this, message, isServerResponseBody);
    }
}

void CUploader::ErrorMessage(const ErrorInfo& error)
{
    if (onErrorMessage_) {
        onErrorMessage_(this, error);
    }
}

void CUploader::Error(bool error, const std::string& message, ErrorType type, int retryIndex, UploadTask* uploadTask, const std::string& topLevelFileName)
{
    ErrorInfo err;
    err.ActionIndex  = -1;
    err.messageType = error ? ErrorInfo::mtError : ErrorInfo::mtWarning;
    err.error = message;
    err.FileName = m_FileName;
    err.errorType = type;
    err.sender = "CUploader";
    err.RetryIndex = retryIndex;
    err.TopLevelFileName = topLevelFileName;
    if (uploadTask) {
        err.ServerName = uploadTask->serverName();
        err.uploadEngineData = uploadTask->serverProfile().uploadEngineData();
    }
    ErrorMessage(err);
}
