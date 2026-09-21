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

#include "AdvancedUploadEngine.h"


#include "FolderTask.h"
#include "Core/Upload/ServerSync.h"

CAdvancedUploadEngine::CAdvancedUploadEngine(std::shared_ptr<ServerSync> serverSync, ServerSettingsStruct* settings, ErrorMessageCallback errorCallback) :
                        CAbstractUploadEngine(std::move(serverSync), std::move(errorCallback)),
                            m_CurrentActionIndex(0),
                            m_nThumbWidth(0)
{
    setServerSettings(settings);
}

CAdvancedUploadEngine::~CAdvancedUploadEngine()
{
}

int CAdvancedUploadEngine::processFolderTask(std::shared_ptr<UploadTask> task) {
    auto folderTask = std::dynamic_pointer_cast<FolderTask>(task);
    assert(folderTask != nullptr);
    if (folderTask->operationType() == FolderOperationType::foGetFolders) {
        return getFolderList(folderTask->folderList());
    } else if (folderTask->operationType() == FolderOperationType::foCreateFolder) {
        CFolderItem parent;
        parent.setId(folderTask->folder().getParentId());
        return createFolder(parent, folderTask->folder());
    } else if (folderTask->operationType() == FolderOperationType::foModifyFolder) {
        return modifyFolder(folderTask->folder());
    } else {
        LOG(ERROR) << "Not implemented";
    }

    return 0;
}

int CAdvancedUploadEngine::processTask(std::shared_ptr<UploadTask> task, UploadParams& params)
{
    int res = doProcessTask(task, params);
    if (res == -2) {
        // Clear authorization flag and try again
        serverSync_->resetAuthorization();
    }
    /*  res = doProcessTask(task, params);
        if (res == -2) {
            return -1;
        }
    }*/
    return res;
}

int CAdvancedUploadEngine::doProcessTask(std::shared_ptr<UploadTask> task, UploadParams& params) {
    return 0;
}

void CAdvancedUploadEngine::setNetworkClient(INetworkClient* nm)
{
    CAbstractUploadEngine::setNetworkClient(nm);
}

bool CAdvancedUploadEngine::supportsSettings()
{
    return false;
}

bool CAdvancedUploadEngine::supportsBeforehandAuthorization()
{
    return false;
}


bool CAdvancedUploadEngine::createNewFolderIfNeeded(std::shared_ptr<UploadTask> task) {
    CFolderItem parent;
    std::string folderID;
    std::lock_guard<std::mutex> guard(serverSync_->folderMutex());
    CFolderItem& newFolder = m_ServersSettings->newFolder;
    ServerProfile& serverProfile = task->serverProfile();
    if (serverProfile.folderId() == CFolderItem::NewFolderMark) {
        serverProfile.setFolderId(newFolder.getId());
        serverProfile.setFolderTitle(newFolder.getTitle());

        if (newFolder.getId() == CFolderItem::NewFolderMark) {
            SetStatus(stCreatingFolder, newFolder.title);
            if (createFolder(parent, newFolder)) {
                folderID = newFolder.id;
                task->serverProfile().setFolderId(folderID);
                m_ServersSettings->setParam("FolderID", folderID);
                m_ServersSettings->setParam("FolderUrl", newFolder.viewUrl);
                return true;
            } else {
                folderID.clear();
            }
        }
    }
    return false;
}

void CAdvancedUploadEngine::log(ErrorInfo::MessageType mt, const std::string& error)
{
    ErrorInfo ei;
    ei.ActionIndex = -1;
    ei.messageType = mt;
    ei.errorType = etUserError;
    ei.error = error;
    ei.sender = "CAdvancedUploadEngine";
    if (m_UploadData) {
        ei.ServerName = m_UploadData->Name;
        ei.uploadEngineData = m_UploadData;
    }
    if (currentTask_) {
        ei.FileName = currentTask_->toString();
    }

    ErrorMessage(ei);
}

int CAdvancedUploadEngine::RetryLimit()
{
    return m_UploadData->RetryLimit;
}
