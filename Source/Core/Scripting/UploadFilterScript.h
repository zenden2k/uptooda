#pragma once

#include <string>
#include "Script.h"

class UploadTask;

class UploadFilterScript : public Script {
    public:
        UploadFilterScript(const std::string& fileName, std::shared_ptr<ThreadSync> serverSync, std::shared_ptr<INetworkClientFactory> networkClientFactory);
        bool preUpload(UploadTask* task);
        bool postUpload(UploadTask* task);
};
