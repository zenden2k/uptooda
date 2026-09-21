#include "SizeExceedFilter.h"

#include "Gui/Dialogs/SizeExceed.h"
#include "Core/Upload/UploadEngine.h"
#include "Core/Upload/FileUploadTask.h"
#include "Core/Upload/UploadSession.h"
#include "Core/ServiceLocator.h"
#include "Core/Scripting/DialogProvider.h"
#include "Core/Settings/WtlGuiSettings.h"

SizeExceedFilter::SizeExceedFilter(CUploadEngineList* engineList, UploadEngineManager* uploadEngineManager) {
    engineList_ = engineList;
    uploadEngineManager_ = uploadEngineManager;
}

bool SizeExceedFilter::PreUpload(UploadTask* task) {
    auto settings = ServiceLocator::instance()->settings<WtlGuiSettings>();

    if (!settings->CheckFileTypesBeforeUpload) {
        return true;
    }

    if (task->type() == UploadTask::TypeFile) {
        FileUploadTask * fileTask = dynamic_cast<FileUploadTask*>(task);
        if (!fileTask || !fileTask->session()) {
            return true;
        }
        const CUploadEngineData* ue = fileTask->serverProfile().uploadEngineData();
        if (ue && ue->MaxFileSize > 0 && fileTask->getFileSize() > ue->MaxFileSize) {
            std::lock_guard<std::mutex> g(uploadSessionDataMapMutex_);
            auto it = uploadSessionDataMap_.find(fileTask->session());
            if (it != uploadSessionDataMap_.end() && !it->second.newImageServer.isNull()) {
                fileTask->setServerProfile(it->second.newImageServer);
            } else {
                std::lock_guard<std::mutex> guard(ServiceLocator::instance()->dialogProvider()->getMutex());
                CSizeExceed sizeExceedDialog(fileTask, engineList_, uploadEngineManager_);
                HWND parent = ServiceLocator::instance()->programWindow()->getNativeHandle();
                INT_PTR res = sizeExceedDialog.DoModal(parent);
                
                if (res == IDC_FORALL) {
                    UploadSessionData& data = uploadSessionDataMap_[fileTask->session()];
                    data.newImageServer = fileTask->serverProfile();
                    if (!data.callbackAdded) {
                        fileTask->session()->addSessionFinishedCallback(std::bind(&SizeExceedFilter::onSessionFinished, this, std::placeholders::_1));
                        data.callbackAdded = true;
                    }
                    //InitialParams = iss; // if user choose button USE FOR ALL

                }
            }
        }
    }
    
    return true;
}

bool SizeExceedFilter::PostUpload(UploadTask* task) {
    return true;
}

void SizeExceedFilter::onSessionFinished(UploadSession* session) {
    uploadSessionDataMap_.erase(session);
}
