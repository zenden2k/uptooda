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

#include "UploadEngine.h"

#include "Core/Utils/StringUtils.h"
#include "Core/Upload/ServerSync.h"
#include "Core/Upload/FileUploadTask.h"
#include "Core/Upload/UrlShorteningTask.h"

CUploadEngineData::CUploadEngineData()
{
    SupportsFolders = false;
    UsingPlugin = false;
    Debug = false;
    SupportThumbnails = false;
    BeforehandAuthorization = false;
    NeedAuthorization = naNotAvailable;
    NeedPassword = true;
    MaxFileSize = 0;
    RetryLimit = 0;
    MaxThreads = 0;
    TypeMask = 0;
    UploadToTempServer = false;
}

int CUploadEngineData::addUserType(const std::string_view& name) {
    auto it = std::find(userTypes.begin(), userTypes.end(), name);

    if (it == userTypes.end()) {
        userTypes.push_back(std::string(name));
        it = userTypes.end() - 1;
    }
    return std::distance(userTypes.begin(), it);
}

bool CUploadEngineData::hasType(ServerType type) const
{
    return (TypeMask & type) == type;
}

bool CUploadEngineData::supportsFileFormat(const std::string& fileName, const std::string& mimeType, int64_t fileSize, std::string_view userType) const {
    if (MaxFileSize > 0 && fileSize > MaxFileSize) {
        return false;
    }

    for (const auto& group : this->ForbiddenFormatGroups) {
        if ((group.MaxFileSize > 0 && fileSize > group.MaxFileSize)
            || (group.MinFileSize > 0 && fileSize < group.MinFileSize)) {
            continue;
        }

        if (!group.acceptsUserType(userType)) {
            continue;
        }

        for (const auto& format : group.Formats) {
            if ((format.MaxFileSize > 0 && fileSize > format.MaxFileSize)
                || (format.MinFileSize > 0 && fileSize < format.MinFileSize)) {
                continue;
            }

            bool mimeMatches = false;
            bool fileNameMatches = false;
            if (!format.MimeTypes.empty()) {
                for (const auto& type : format.MimeTypes) {
                    if (IuStringUtils::Match(type, mimeType)) {
                        mimeMatches = true;
                        break;
                    }
                }
                if (!mimeMatches) {
                    continue;
                }
            }
            if (!format.FileNameWildcards.empty()) {
                for (const auto& wildcard : format.FileNameWildcards) {
                    if (IuStringUtils::Match(wildcard, fileName)) {
                        fileNameMatches = true;
                        break;
                    }
                }
                if (!fileNameMatches) {
                    continue;
                }
            }
            if (mimeMatches || fileNameMatches) {
                return false;
            }
        }
    }

    if (this->SupportedFormatGroups.empty()) {
        return true;
    }

    for (const auto& group : this->SupportedFormatGroups) {
        if ((group.MaxFileSize > 0 && fileSize > group.MaxFileSize)
            || (group.MinFileSize > 0 && fileSize < group.MinFileSize)) {
            continue;
        }
        if (!group.acceptsUserType(userType)) {
            continue;
        }

        for (const auto& format : group.Formats) {
            if ((format.MaxFileSize > 0 && fileSize > format.MaxFileSize)
                || (format.MinFileSize > 0 && fileSize < format.MinFileSize)) {
                continue;
            }
            bool mimeMatches = false;
            bool fileNameMatches = false;
            if (!format.MimeTypes.empty()) {
                for (const auto& type : format.MimeTypes) {
                    if (IuStringUtils::Match(type, mimeType)) {
                        mimeMatches = true;
                        break;
                    }
                }
                if (!mimeMatches) {
                    continue;
                }
            }
            if (!format.FileNameWildcards.empty()) {
                for (const auto& wildcard : format.FileNameWildcards) {
                    if (IuStringUtils::Match(wildcard, fileName)) {
                        fileNameMatches = true;
                        break;
                    }
                }
                if (!fileNameMatches) {
                    continue;
                }
            }
            if (mimeMatches || fileNameMatches) {
                return true;
            }
        }
    }
    return false;
}

std::set<std::string> CUploadEngineData::getSupportedExtensions() const {
    std::set<std::string> formatSet;
    for (const auto& formatGroup : SupportedFormatGroups) {
        formatSet.insert(formatGroup.Extensions.begin(), formatGroup.Extensions.end());        
    }
    return formatSet;

}

CUploadEngineData::ServerType CUploadEngineData::ServerTypeFromString(const std::string& serverType) {
    if (serverType == "image"){
        return TypeImageServer;
    } else if (serverType == "file") {
        return TypeFileServer;
    } else if (serverType == "text") {
        return TypeTextServer;
    } else if (serverType == "urlshortening"){
        return TypeUrlShorteningServer;
    } else if (serverType == "searchbyimage") {
        return TypeSearchByImageServer;
    } else if (serverType == "video") {
        return TypeVideoServer;
    }
    return TypeInvalid;
}

std::set<unsigned int> CUploadEngineData::getUserTypesIds() const {
    std::set<unsigned int> res;
    for (unsigned int i = 0; i < userTypes.size(); i++) {
        res.insert(i);
    }
    return res;
}

CUploadEngineListBase::CUploadEngineListBase(): mt_(std::random_device()())
{
}

CUploadEngineData* CUploadEngineListBase::byIndex(size_t index) {
    if ( index < m_list.size() ) {
        return m_list[index].get();
    }
    return nullptr;
}

int CUploadEngineListBase::count() const
{
    return m_list.size();
}

CUploadEngineData* CUploadEngineListBase::byName(const std::string& name)
{
    auto index = getUploadEngineIndex(name);

    if (index == -1) {
        return nullptr;
    }

    return m_list[index].get();
}

CUploadEngineData*  CUploadEngineListBase::firstEngineOfType(CUploadEngineData::ServerType type) {
    for (size_t i = 0; i < m_list.size(); i++)
    {
        if ( m_list[i]->hasType(type)) {
            return m_list[i].get();
        }
    }
    return nullptr;
}

int CUploadEngineListBase::getRandomImageServer()
{
    std::vector<int> m_suitableServers;
    for (size_t i = 0; i < m_list.size(); i++)
    {
        if (m_list[i]->NeedAuthorization != CUploadEngineData::naObligatory && m_list[i]->hasType(CUploadEngineData::TypeImageServer)) {
            m_suitableServers.push_back(i);
        }
    }
    if (m_suitableServers.empty()) {
        return -1;
    }

    std::uniform_int_distribution<int> dist(0, m_suitableServers.size() - 1);
    return m_suitableServers[dist(mt_)];
}

int CUploadEngineListBase::getRandomFileServer()
{
    std::vector<size_t> m_suitableServers;
    for (size_t i = 0; i < m_list.size(); i++)
    {
        if (m_list[i]->NeedAuthorization != CUploadEngineData::naObligatory && m_list[i]->hasType(CUploadEngineData::TypeFileServer)) {
            m_suitableServers.push_back(i);
        }
    }
    if (m_suitableServers.empty()) {
        return -1;
    }
    std::uniform_int_distribution<int> dist(0, m_suitableServers.size() - 1);
    return m_suitableServers[dist(mt_)];
}

int CUploadEngineListBase::getUploadEngineIndex(const std::string& Name) const
{
    auto it = serverNameToIndex_.find(IuStringUtils::ToLower(Name));
    if (it != serverNameToIndex_.end()) {
        return it->second;
    }
    
    return -1;
}

std::vector<std::unique_ptr<CUploadEngineData>>::const_iterator CUploadEngineListBase::begin() const {
    return m_list.begin();
}
std::vector<std::unique_ptr<CUploadEngineData>>::const_iterator CUploadEngineListBase::end() const {
    return m_list.end();
}

std::string CUploadEngineListBase::getDefaultServerNameForType(CUploadEngineData::ServerType serverType) const {
    auto it = m_defaultServersForType.find(serverType);
    if (it != m_defaultServersForType.end()) {
        return it->second;
    }
    return {};
}

std::vector<std::string_view> CUploadEngineListBase::builtInScripts() {
    return BUILTIN_SCRIPTS;
}

void CUploadEngineListBase::removeServer(const std::string& name) {
    m_list.erase(
        std::remove_if(
            m_list.begin(), m_list.end(),
            [&name](const auto& x) {
                return x->Name == name;
            }
        ),
        m_list.end()
   );
}


std::string CUploadEngineListBase::getServerDisplayName(const CUploadEngineData* data)
{
    if (!data) {
        return {};
    }

    std::string serverName = data->DisplayName.empty() ? data->Name : data->DisplayName;

    if (std::find(BUILTIN_SCRIPTS.begin(), BUILTIN_SCRIPTS.end(), data->PluginName) != BUILTIN_SCRIPTS.end()) {
        serverName = IuStringUtils::Replace(serverName, "(" + data->PluginName + ")", "[" + IuStringUtils::ToUpper(data->PluginName) + "]");
    }
    return serverName;
}

/* CAbstractUploadEngine */

CAbstractUploadEngine::~CAbstractUploadEngine()
{
}

bool CAbstractUploadEngine::DebugMessage(const std::string& message, bool isServerResponseBody)
{
    if (onDebugMessage_)
        onDebugMessage_(message, isServerResponseBody);
    return true;
}

bool CAbstractUploadEngine::ErrorMessage(const ErrorInfo& ei)
{
    if (onErrorMessage_) {
        onErrorMessage_(ei);
    } else
    {
        (ei.messageType == ErrorInfo::mtError ? LOG(ERROR) : LOG(WARNING)) << ei.error;
    }
    return true;
}

void CAbstractUploadEngine::setServerSettings(ServerSettingsStruct* settings)
{
    m_ServersSettings = settings;
}

ServerSettingsStruct * CAbstractUploadEngine::serverSettings() const
{
    return m_ServersSettings;
}

bool CAbstractUploadEngine::needStop()
{
    if (m_bShouldStop)
        return m_bShouldStop;
    if (onNeedStop_)
        m_bShouldStop = onNeedStop_();  // delegate call
    return m_bShouldStop;
}

void CAbstractUploadEngine::SetStatus(StatusType status, const std::string& param)
{
    if (onStatusChanged_)
        onStatusChanged_(status, 0,  param);
}

void CAbstractUploadEngine::setNetworkClient(INetworkClient* nm)
{
    m_NetworkClient = nm;
    m_NetworkClient->setCurlShare(serverSync_->getCurlShare());
}

void CAbstractUploadEngine::setUploadData(CUploadEngineData* data)
{
    m_UploadData = data;
}

CAbstractUploadEngine::CAbstractUploadEngine(std::shared_ptr<ServerSync> serverSync, ErrorMessageCallback errorCallback)
{
    m_bShouldStop = 0;
    m_NetworkClient = nullptr;
    m_UploadData = nullptr;
    currUploader_ = nullptr;
    serverSync_ = std::move(serverSync);
    m_ServersSettings = nullptr;
    onErrorMessage_ = errorCallback;
}


CUploadEngineData* CAbstractUploadEngine::getUploadData() const
{
    return m_UploadData;
}

void CAbstractUploadEngine::setOnNeedStopCallback(std::function<bool()> cb) {
    onNeedStop_ = cb;
}

void CAbstractUploadEngine::setOnProgressCallback(std::function<void(InfoProgress)> cb) {
    onProgress_ = cb;
}

void CAbstractUploadEngine::setOnStatusChangedCallback(std::function<void(StatusType, int, const std::string&)> cb) {
    onStatusChanged_ = cb;
}

void CAbstractUploadEngine::setOnDebugMessageCallback(std::function<void(const std::string&, bool)> cb) {
    onDebugMessage_ = cb;
}

void CAbstractUploadEngine::setOnErrorMessageCallback(ErrorMessageCallback cb) {
    onErrorMessage_ = cb;
}

void CAbstractUploadEngine::setServerSync(std::shared_ptr<ServerSync> sync)
{
    serverSync_ = sync;
}

void CAbstractUploadEngine::setCurrentUploader(CUploader* uploader) {
    currUploader_ = uploader;
}

CUploader* CAbstractUploadEngine::currentUploader() const{
    return currUploader_;
}

void CAbstractUploadEngine::stop()
{
    m_bShouldStop = true;
    //serverSync_->stop();
}

std::shared_ptr<ServerSync> CAbstractUploadEngine::serverSync() const
{
    return serverSync_;
}

ScriptAPI::UploadTaskUnion UploadParams::getTask() {
    return task_;
}

/*/ ScriptAPI::FileUploadTaskWrapper UploadParams::getFileTask()
{
    auto fileTask = std::dynamic_pointer_cast<FileUploadTask>(task_);
    return fileTask;
}

ScriptAPI::UrlShorteningTaskWrapper UploadParams::getUrlShorteningTask() {
    auto urlShorteningTask = std::dynamic_pointer_cast<UrlShorteningTask>(task_);
    return urlShorteningTask;
}*/

bool FileFormatGroup::acceptsUserType(std::string_view userType) const {
    if (UserTypes.empty()) {
        return true;
    }
    for (const auto& currentType : UserTypes) {
        if (currentType == "*" || IuStringUtils::Match(currentType, userType)) {
            return true;
        }
    }
    return false;    
}
