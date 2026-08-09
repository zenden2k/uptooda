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

#ifndef IU_SCRIPT_UPLOAD_ENGINE_H
#define IU_SCRIPT_UPLOAD_ENGINE_H

#include <vector>
#include <string>

#include "Core/Scripting/Squirrelnc.h"

#include "CommonTypes.h"
#include "UploadEngine.h"
#include "Core/Utils/CoreTypes.h"
#include "Core/Upload/FolderList.h"
#include "Core/Scripting/Script.h"
#include "Core/Network/NetworkClient.h"
#include "AdvancedUploadEngine.h"
#include "Parameters/AbstractParameter.h"

class CScriptUploadEngine : public CAdvancedUploadEngine,
                            public Script,
                            public NetworkClient::Logger
{
    public:
        CScriptUploadEngine(const std::string& pluginName, std::shared_ptr<ServerSync> serverSync, ServerSettingsStruct* settings,
            std::shared_ptr<INetworkClientFactory> factory, ErrorMessageCallback errorCallback);

        void setNetworkClient(INetworkClient* nm) override;
        //bool load(std::string fileName, ServerSettingsStruct& params);
        int getFolderList(CFolderList &FolderList) override;
        int createFolder(const CFolderItem &parent, CFolderItem &folder) override;
        int modifyFolder(CFolderItem &folder) override;
        int getAccessTypeList(std::vector<std::string> &list) override;
        int getServerParamList(ParameterList& list) override;
        int doLogin() override;
        int doLogout() override;
        bool supportsLogout() override;
        bool supportsSettings() override;

        /**
        Beforehand authorization - obtain access token only once then use it for all requests (before upload)
        Return true if there is "DoLogin" function in squirrel script
        **/
        bool supportsBeforehandAuthorization() override;
        bool isAuthenticated() override;
        std::string name() const;

        // FIXME: not working
        void stop() override;
    protected:
        int doUpload(std::shared_ptr<UploadTask> task, UploadParams& params);
        void Log(ErrorInfo::MessageType mt, const std::string& error);
        void PrintCallback(const std::string& output) override;
        int processAuthTask(std::shared_ptr<UploadTask> task);
        int processTestTask(std::shared_ptr<UploadTask> task);
        int doProcessTask(std::shared_ptr<UploadTask> task, UploadParams& params) override;
        int refreshToken();
        bool preLoad() override;
        bool postLoad() override;
        void logNetworkError(bool error, const std::string & msg) override;
        bool functionExists(const std::string& name);
        int checkAuth();
        bool needStop() override;
        bool newAuthMode_;
        bool hasRefreshTokenFunc_ = false;
        DISALLOW_COPY_AND_ASSIGN(CScriptUploadEngine);
};
#endif
