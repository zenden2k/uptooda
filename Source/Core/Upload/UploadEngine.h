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
#ifndef IU_CORE_UPLOADENGINE_H
#define IU_CORE_UPLOADENGINE_H

#pragma once

#include <vector>
#include <string>
#include <string_view>
#include <map>
#include <random>
#include <unordered_set>
#include <set>

#include "Core/Network/NetworkClient.h"
#include "CommonTypes.h"
#include "Core/Scripting/API/UploadTaskWrappers.h"
#include "FolderItem.h"

class ServerSync;

struct LoginInfo
{
    std::string Login, Password, Cookies;
    bool DoAuth;
    LoginInfo() {
        DoAuth = false;
    }
};

struct ActionVariable
{
    std::string Name;
    int nIndex;
    ActionVariable(std::string name, int index): Name(std::move(name)), nIndex(index) {

    }
    ActionVariable() : nIndex(0) {

    }
};

struct ActionFunc {
    static constexpr auto FUNC_REGEXP = "regexp";
    static constexpr auto FUNC_JSON = "json";
    std::string Func;
    std::string AssignVars;
    std::vector<std::string> Arguments;
    std::vector<ActionVariable> Variables;
    bool Required;

    ActionFunc() : Required(true) {

    }

    ActionFunc(std::string functionName): Func(std::move(functionName)), Required(true){

    }

    void setArg(size_t index, const std::string& value) {
        if (index > 100) {
            LOG(ERROR) << "Function call has too many arguments: " << index << " (max 100)";
            return;
        }
        if (Arguments.size() < index + 1) {
            Arguments.resize(index + 1);
        }
        Arguments[index] = value;
    }
    std::string getArg(size_t index) const {
        if (index < Arguments.size()) {
            return Arguments[index];
        }
        return {};
    }
};

namespace UserTypes {
    constexpr std::string_view ANONYMOUS = "anon";
    constexpr std::string_view REGISTERED = "reg";
}

struct UploadAction
{
    int Index;
    bool IgnoreErrors;
    bool OnlyOnce;
    std::string Url;
    std::string Description;
    std::string Referer;
    std::string PostParams;
    std::string CustomHeaders;
    std::string Type;
   // std::string RegExp;

    //std::vector<ActionRegExp> Regexes;
    std::vector<ActionFunc> FunctionCalls;
    int RetryLimit;
    //int NumOfTries;

    UploadAction() {
        Index = 0;
        IgnoreErrors = false;
        OnlyOnce = false;
        RetryLimit = 1;
        //NumOfTries = 0;
    }
};

struct FileFormat {
    std::vector<std::string> MimeTypes;
    std::vector<std::string> FileNameWildcards;
    int64_t MaxFileSize = 0;
    int64_t MinFileSize = 0; // forbidden formats
};

struct FileFormatGroup {
    std::vector<FileFormat> Formats;
    int64_t MaxFileSize = 0; // allowed formats
    int64_t MinFileSize = 0; // forbidden formats
    std::unordered_set<std::string> UserTypes;
    std::set<unsigned int> UserTypeIds;
    std::set<std::string> Extensions;
    int MinUserRank = 0;
    bool acceptsUserType(std::string_view userType) const;
};

struct StorageTime {
    inline static constexpr int TIME_INFINITE = -1;
    int Time = 0; // in days
    bool AfterLastDownload = false;
    std::vector<std::string> UserTypes;
    std::set<unsigned int> UserTypeIds;
    int MinUserRank = 0;
};
    /**
Global object <b>ServerParams</b> is an instance of ServerSettingsStruct.
*/
class ServerSettingsStruct
{
public:
    /*! @cond PRIVATE */
    ServerSettingsStruct() : paramsMutex_(new std::mutex()){ authData.DoAuth = 0; }
    std::map<std::string, std::string> params;
    std::shared_ptr<std::mutex> paramsMutex_;
    LoginInfo authData;
    CFolderItem newFolder;
    CFolderItem defaultFolder;

    bool isEmpty() const {
        return authData.Login.empty() && authData.Password.empty() && !authData.DoAuth && params.empty() && defaultFolder.getId().empty();
    }
    /*! @endcond */

    /**
     * Params:
     *
     * Login
     * Password
     *
     * Custom parameters which should be visible in server settings dialog, should have the same name as in \ref GetServerParamList.
     */
    std::string getParam(const std::string& name)
    {
        std::lock_guard<std::mutex> lock(*paramsMutex_);
        std::string result;
        std::string pname = name;
        if(pname == "Password" && authData.DoAuth)
            result = authData.Password;
        else if(pname == "Login" && authData.DoAuth)
            result = authData.Login;
        else {
            auto it = params.find(name);
            if (it != params.end()) {
                result = it->second;
            }
        }
        return result;
    }

    void setParam(const std::string& name, const std::string& value)
    {
        std::lock_guard<std::mutex> lock(*paramsMutex_);
        params[name] = value;
    }
};

typedef std::map <std::string, std::map <std::string, ServerSettingsStruct>> ServerSettingsMap;

class CUploadEngineData
{
    public:
        enum ServerType { TypeInvalid = 0, TypeImageServer = 1, TypeFileServer = 2 , TypeUrlShorteningServer = 4, TypeTextServer = 8, TypeSearchByImageServer = 16, TypeVideoServer = 32};
        enum NeedAuthorizationEnum { naNotAvailable = 0, naAvailable, naObligatory };
        inline static constexpr int MAX_FILE_SIZE_UNLIMITED = -1;
        std::string Name;
        std::string DisplayName;
        std::string PluginName;
        bool SupportsFolders;
        bool UsingPlugin;
        bool Debug;
        //bool ImageHost;
        bool SupportThumbnails;
        bool BeforehandAuthorization;
        int NeedAuthorization;
        bool NeedPassword;
        int64_t MaxFileSize;
        std::string RegistrationUrl;
        std::string WebsiteUrl;
        std::string CodedLogin;
        std::string CodedPassword;
        std::string ThumbUrlTemplate, ImageUrlTemplate, DownloadUrlTemplate, DeleteUrlTemplate, EditUrlTemplate;
        std::vector<UploadAction> Actions;
        std::vector<FileFormatGroup> SupportedFormatGroups;
        std::vector<FileFormatGroup> ForbiddenFormatGroups;
        std::vector<StorageTime> StorageTimeInfo;
        std::string LoginLabel, PasswordLabel;
        std::string UserAgent;
        std::string Engine;
        int RetryLimit;
        //int NumOfTries;
        int MaxThreads;
        bool UploadToTempServer;
        int TypeMask;
        std::vector<std::string> userTypes = {std::string(UserTypes::ANONYMOUS)};
        bool hasType(ServerType type) const;
        bool supportsFileFormat(const std::string& fileName, const std::string& mimeType, int64_t fileSize, std::string_view userType) const;
        std::set<std::string> getSupportedExtensions() const;
        CUploadEngineData();
        int addUserType(const std::string_view& name);
        static ServerType ServerTypeFromString(const std::string& serverType);
        std::set<unsigned int> getUserTypesIds() const;
};
/**
UploadParams class
*/
class UploadParams
{
public:
    /*! @cond PRIVATE */
    int apiVersion;
    int thumbWidth;
    int thumbHeight;
    std::string serverName;
    std::string data;
    CFolderItem folder;
    std::string DirectUrl;

    std::string ThumbUrl;
    std::string ViewUrl;
    std::string EditUrl;
    std::string DeleteUrl;

    std::string ServerFileName;
    std::shared_ptr<UploadTask> task_;
    bool createThumbnail = false;
    bool useServerSideThumbnail = false;
    bool addTextOnThumb = false;

    UploadParams() {
        apiVersion = 0;
        thumbWidth = 0;
        thumbHeight = 0;
    }

    void setFolder(const CFolderItem& fld)
    {
        folder = fld;
    }
    /*! @endcond */

    /**
     *  Possible parameters:
     *    THUMBWIDTH
     *    THUMBHEIGHT
     *    THUMBCREATE
     *    THUMBADDTEXT
     *    THUMBUSESERVER
     */
    std::string getParam(const std::string& name)
    {
        if (name == "THUMBWIDTH") {
            return std::to_string(thumbWidth);
        } else if (name == "THUMBHEIGHT") {
            return std::to_string(thumbHeight);
        } else if (name == "THUMBCREATE") {
            return std::to_string(createThumbnail);
        } else if (name == "THUMBADDTEXT") {
            return std::to_string(addTextOnThumb);
        } else if (name == "THUMBUSESERVER") {
            return std::to_string(useServerSideThumbnail);
        }

        return {};
    }

    CFolderItem getFolder() const {
        return folder;
    }

    std::string getFolderID() { return folder.getId(); }
    void setDirectUrl(const std::string& url) { DirectUrl = url;}
    void setThumbUrl(const std::string& url) { ThumbUrl = url;}
    void setViewUrl(const std::string& url) { ViewUrl = url;}
    /**
    @since version 1.3.2
    */
    void setEditUrl(const std::string& url) { EditUrl = url;}
    /**
    @since version 1.3.2
    */
    void setDeleteUrl(const std::string& url) { DeleteUrl = url;}

    /**
    @since version 1.3.2
    */
    std::string getDirectUrl() const
    {
        return DirectUrl;
    }
    /**
    @since version 1.3.2
    */
    std::string getThumbUrl() const
    {
        return ThumbUrl;
    }
    /**
    @since version 1.3.2
    */
    std::string getViewUrl() const
    {
        return ViewUrl;
    }
    /**
    @since version 1.3.2
    */
    std::string getEditUrl() const
    {
        return EditUrl;
    }
    /**
    @since version 1.3.2
    */
    std::string getDeleteUrl() const
    {
        return DeleteUrl;
    }

    std::string getServerFileName() const { return ServerFileName; }
    ScriptAPI::UploadTaskUnion getTask();
    /* ScriptAPI::FileUploadTaskWrapper getFileTask();
    ScriptAPI::UrlShorteningTaskWrapper getUrlShorteningTask();*/
};

/**
 * Task result code
 */
enum class ResultCode {
    FatalError = -3,
    TryAgain = -2,
    FatalServerError = -1,
    Failure = 0,
    Success = 1
};

class CUploadEngineListBase
{
public:
    CUploadEngineListBase();
    virtual ~CUploadEngineListBase() = default;
    CUploadEngineData* byIndex(size_t index);
    CUploadEngineData* byName(const std::string& name);
    CUploadEngineData* firstEngineOfType(CUploadEngineData::ServerType type);
    void removeServer(const std::string& name);
    int count() const;
    int getRandomImageServer();
    int getRandomFileServer();
    int getUploadEngineIndex(const std::string& Name) const;
    std::vector<std::unique_ptr<CUploadEngineData>>::const_iterator begin() const;
    std::vector<std::unique_ptr<CUploadEngineData>>::const_iterator end() const;
    std::string getDefaultServerNameForType(CUploadEngineData::ServerType serverType) const;
    static std::vector<std::string_view> builtInScripts();
    std::string getServerDisplayName(const CUploadEngineData* data) const;

    inline static constexpr std::string_view CORE_SCRIPT_FTP = "ftp";
    inline static constexpr std::string_view CORE_SCRIPT_SFTP = "sftp";
    inline static constexpr std::string_view CORE_SCRIPT_WEBDAV = "webdav";
    inline static constexpr std::string_view CORE_SCRIPT_DIRECTORY = "directory";

    inline static constexpr int ALL_SERVERS = 0xffffffff;
    inline static std::vector<std::string_view> BUILTIN_SCRIPTS = { CORE_SCRIPT_FTP, CORE_SCRIPT_SFTP, CORE_SCRIPT_WEBDAV, CORE_SCRIPT_DIRECTORY };

protected:
    std::vector<std::unique_ptr<CUploadEngineData>> m_list;
    std::map<CUploadEngineData::ServerType, std::string> m_defaultServersForType;
    std::mt19937 mt_;
private:
    DISALLOW_COPY_AND_ASSIGN(CUploadEngineListBase);
};

class UploadTask;
class CUploader;

class CAbstractUploadEngine
{
    public:
        typedef std::function<void(const ErrorInfo&)> ErrorMessageCallback;

        CAbstractUploadEngine(ServerSync* serverSync, ErrorMessageCallback errorCallback);
        virtual ~CAbstractUploadEngine();
        virtual int processTask(std::shared_ptr<UploadTask> task, UploadParams& params) = 0;
        void setServerSettings(ServerSettingsStruct* settings);
        ServerSettingsStruct * serverSettings() const;
        virtual int RetryLimit()=0;
        virtual void setNetworkClient(INetworkClient* nm);
        void setUploadData(CUploadEngineData* data);
        void setServerSync(ServerSync* sync);
        void setCurrentUploader(CUploader *uploader);
        CUploader * currentUploader() const;
        virtual void stop();
        ServerSync* serverSync() const;
        CUploadEngineData* getUploadData() const;
        // Events
        void setOnNeedStopCallback(std::function<bool()> cb);
        void setOnProgressCallback(std::function<void(InfoProgress)> cb);
        void setOnStatusChangedCallback(std::function<void(StatusType, int, const std::string&)> cb);
        void setOnDebugMessageCallback(std::function<void(const std::string&, bool)> cb);
        void setOnErrorMessageCallback(ErrorMessageCallback cb);
        DISALLOW_COPY_AND_ASSIGN(CAbstractUploadEngine);
    protected:
        bool m_bShouldStop;
        INetworkClient * m_NetworkClient;
        CUploadEngineData * m_UploadData;
        CUploader * currUploader_;
        ServerSettingsStruct* m_ServersSettings;
        std::shared_ptr<UploadTask> currentTask_;
        ServerSync* serverSync_;
        ErrorMessageCallback onErrorMessage_;

        std::function<bool()> onNeedStop_;
        std::function<void(InfoProgress)> onProgress_;
        std::function<void(StatusType, int, const std::string&)> onStatusChanged_;
        std::function<void(const std::string&, bool)> onDebugMessage_;
        bool DebugMessage(const std::string& message, bool isServerResponseBody = false);
        bool ErrorMessage(const ErrorInfo&);
        virtual bool needStop();
        virtual void SetStatus(StatusType status, const std::string& param = "");
};

#endif // IU_CORE_UPLOADENGINE_H
