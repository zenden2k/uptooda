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

#ifndef IU_CORE_UPLOAD_DEFAULTUPLOADENGINE_H
#define IU_CORE_UPLOAD_DEFAULTUPLOADENGINE_H

#include <map>
#include <random>
#include <string>

#include "CommonTypes.h"
#include "UploadEngine.h"
#include "Core/Network/NetworkClient.h"

class FileUploadTask;
class UrlShorteningTask;
class SearchByImageUrlTask;
class ServerSync;

class CDefaultUploadEngine: public CAbstractUploadEngine, public NetworkClient::Logger
{
    public:
        CDefaultUploadEngine(std::shared_ptr<ServerSync> serverSync, ErrorMessageCallback errorCallback);
        int processTask(std::shared_ptr<UploadTask> task, UploadParams& params) override;
    protected:
        int doUpload(std::shared_ptr<UploadTask> task, UploadParams& params);
        bool DoAction(const UploadAction &Action);
        bool DoUploadAction(const UploadAction &Action, bool bUpload);
        bool DoGetAction(const UploadAction &Action);
        bool ParseAnswer(const UploadAction &Action, const std::string& Body);
        std::string ReplaceVars(const std::string& Text);
        int RetryLimit() override;
        void AddQueryPostParams(const UploadAction& Action);
        bool ReadServerResponse(const UploadAction& Action);
        void AddCustomHeaders(const UploadAction& Action);
        void SetStatus(StatusType status, const std::string& param = "") override;
        bool needStop() override;
        void UploadError(bool error, const std::string& errorStr, const UploadAction* m_CurrentAction, bool writeToBuffer = true);
        bool doUploadFile(std::shared_ptr<FileUploadTask> task, UploadParams& params);
        bool doUploadUrl(std::shared_ptr<UrlShorteningTask> task, UploadParams& params);
        bool doSearchImageByUrl(std::shared_ptr<SearchByImageUrlTask> task, UploadParams& params);
        void prepareUpload(UploadParams& params);
        bool executeActions();

        static bool reg_single_match(const std::string& pattern, const std::string& text, std::string& res);

        std::string m_ErrorReason;
        std::string m_FileName;
        std::string m_displayFileName;
        bool fatalError_;
        LoginInfo li;
        ErrorInfo m_LastError;
        std::string m_ErrorBuffer;
        int m_CurrentActionIndex;
        std::map<std::string, std::string> m_Vars;
        std::map<size_t, bool> m_PerformedActions;
        std::random_device randomDevice_;
        std::mt19937 mt_;
    private:
        DISALLOW_COPY_AND_ASSIGN(CDefaultUploadEngine);
        void logNetworkError(bool error, const std::string & msg) override;
};

#endif
