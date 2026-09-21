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

#include "ScriptUploadEngine.h"

#include <thread>

#include "Core/Scripting/Squirrelnc.h"
#include "Core/Scripting/API/ScriptAPI.h"
#include "Core/Upload/FileUploadTask.h"
#include "Core/Upload/UrlShorteningTask.h"
#include "Core/Upload/SearchByImageFileTask.h"
#include "Core/Upload/ServerSync.h"
#include "Core/ThreadSync.h"
#include "AuthTask.h"
#include "FolderTask.h"
#include "Parameters/TextParameter.h"
#include "Parameters/ParameterFactory.h"
#include "Core/i18n/Translator.h"

namespace {

std::pair<int, Sqrat::Table> GetOperationResult(Sqrat::SharedPtr<Sqrat::Object> obj) {
    int res = 0;

    Sqrat::Table t;
    if (!obj) {
        LOG(WARNING) << "Invalid result type";
        return { 0, t };
    }

    if (obj->GetType() == OT_INTEGER) {
        res = obj->Cast<int>();
    } else if (obj->GetType() == OT_TABLE) {
        t = obj->Cast<Sqrat::Table>();
        res = ScriptAPI::GetValue(t.GetValue<int>("status"));
    } else {
        LOG(WARNING) << "Invalid result type";
    }
    return {res, t};
}

void SqTableToParameterList(Sqrat::SharedPtr<Sqrat::Table> tbl, ParameterList& list) {
    list.clear();
    Sqrat::Object::iterator it;
    while (tbl->Next(it)) {
        Sqrat::Object obj(it.getValue(), tbl->GetVM());
        if (obj.GetType() == OT_STRING) {
            // The old way to declare a parameter
            auto newParam = std::make_unique<TextParameter>(it.getName());
            std::string value = obj.Cast<std::string>();
            newParam->setTitle(value);
            list.push_back(std::move(newParam));
        } else if (obj.GetType() == OT_TABLE) {
            // The new way to declare a parameter (in a nested table)
            Sqrat::Table table(it.getValue(), tbl->GetVM());
            try {
                if (!table.HasKey("title")) {
                    throw std::runtime_error("Parameter's declaration should have 'title' key.");
                }
                std::string title = ScriptAPI::GetValue(table.GetValue<std::string>("title"));
                std::string typeStr;

                if (table.HasKey("type")) {
                    typeStr = ScriptAPI::GetValue(table.GetValue<std::string>("type"));
                }
                auto newParam = SqTableToParameter(it.getName(), typeStr, table);
                if (newParam) {
                    newParam->setTitle(title);
                    list.push_back(std::move(newParam));
                }
            } catch (const std::exception& ex) {
                LOG(ERROR) << ex.what();
            }
        }
    }
}

}

CScriptUploadEngine::CScriptUploadEngine(const std::string& fileName, std::shared_ptr<ServerSync> serverSync, ServerSettingsStruct* settings,
                                         std::shared_ptr<INetworkClientFactory> factory, ErrorMessageCallback errorCallback) :
    CAdvancedUploadEngine(serverSync, settings, std::move(errorCallback)),
    Script(fileName, serverSync, std::move(factory), false)
{
    setServerSettings(settings);
    newAuthMode_ = false;
    name_ = IuCoreUtils::ExtractFileNameNoExt(fileName);
    load(fileName);
}

void CScriptUploadEngine::PrintCallback(const std::string& output)
{
    Log(ErrorInfo::mtInformation, output);
}

int CScriptUploadEngine::doProcessTask(std::shared_ptr<UploadTask> task, UploadParams& params) {
    if (task->type() == UploadTask::TypeAuth) {
        return processAuthTask(task);
    } if (task->type() == UploadTask::TypeTest) {
        return processTestTask(task);
    } if (task->type() == UploadTask::TypeFolder) {
        return processFolderTask(task);
    } else {
        return doUpload(task, params);
    }
}

int CScriptUploadEngine::processAuthTask(std::shared_ptr<UploadTask> task) {
    SetStatus(stAuthorization);
    auto authTask = std::dynamic_pointer_cast<AuthTask>(task);
    if (!authTask) {
        return 0;
    }
    switch (authTask->authActionType()) {
    case AuthActionType::Login:
        return doLogin();
    case AuthActionType::Logout:
        return doLogout();
    }
    return 0;
}

int CScriptUploadEngine::processTestTask(std::shared_ptr<UploadTask> task) {
    using namespace Sqrat;
    try {
        checkCallingThread();

        Function func(vm_.GetRootTable(), "TestConnection");
        if (func.IsNull()) {
            return 0;
        }

        auto tbl = func.Evaluate<Sqrat::Table>();

        int res = static_cast<int>(ScriptAPI::GetValue(tbl->GetValue<SQInteger>("status")));
        auto msg = ScriptAPI::GetValue(tbl->GetValue<Sqrat::string>("message"));
        task->uploadResult()->message = msg;

        return res;
    }
    catch (const Sqrat::Exception& e) {
        Log(ErrorInfo::mtError, "CScriptUploadEngine::processTestTask\r\n" + e.Message());
    } catch (const std::exception& e) {
        Log(ErrorInfo::mtError, "CScriptUploadEngine::processTestTask\r\n" + std::string(e.what()));
    }
    FlushSquirrelOutput();
    return 0;
}

int CScriptUploadEngine::doUpload(std::shared_ptr<UploadTask> task, UploadParams& params)
{
    if (checkAuth() < 1) {
        return -1;
    }
    using namespace Sqrat;
    std::string FileName;

    currentTask_ = task;
    if (task->type() == UploadTask::TypeFile ) {
        auto* fileTask = dynamic_cast<FileUploadTask*>(task.get());
        if (fileTask) {
            FileName = fileTask->getFileName();
        }
    }

    UploadTask* topLevelTask = task->parentTask() ? task->parentTask() : task.get();
    auto* topLevelFileTask = dynamic_cast<FileUploadTask*>(topLevelTask);
    if (topLevelFileTask) {
        setCurrentTopLevelFileName(topLevelFileTask->getFileName());
    }

    createNewFolderIfNeeded(task);

    params.setFolder(task->serverProfile().folder());
    params.task_ = task;
    int ival = 0;

    if (!params.getFolderID().empty() && task->onFolderUsed_) {
        task->onFolderUsed_(task.get());
    }
    try {
        checkCallingThread();
        SetStatus(stUploading);
        if (task->type() == UploadTask::TypeFile) {
            Function func(vm_.GetRootTable(), "UploadFile");
            if ( func.IsNull() ) {
                 Log(ErrorInfo::mtError, "CScriptUploadEngine::doUpload\r\n" + std::string("Function UploadFile not found in script"));
                 currentTask_ = nullptr;
                 return -1;
            }

            auto [status, table] = GetOperationResult(func.Evaluate<Object>(FileName.c_str(), &params));
            ival = status /*ScriptAPI::GetValue(func.Evaluate<int>(FileName.c_str(), &params)) */ ;
        } else if (task->type() == UploadTask::TypeUrl ) {
            std::shared_ptr<UrlShorteningTask> urlShorteningTask = std::dynamic_pointer_cast<UrlShorteningTask>(task);
            Function func(vm_.GetRootTable(), "ShortenUrl");
            if ( func.IsNull() ) {
                 Log(ErrorInfo::mtError, "CScriptUploadEngine::doUpload\r\n" + std::string("Function ShortenUrl not found in script"));
                 currentTask_ = nullptr;
                 return -1;
            }
            std::string url = urlShorteningTask->getUrl();
            auto [status, table] = GetOperationResult(func.Evaluate<Object>(url.c_str(), &params));
            /*SharedPtr<int> ivalPtr*/ival = status;
            if ( ival > 0 ) {
                ival = !params.DirectUrl.empty();
            }
        } else if (task->type() == UploadTask::TypeSearchByImageFile) {
            auto searchTask = std::dynamic_pointer_cast<SearchByImageFileTask>(task);
            Function func(vm_.GetRootTable(), "SearchByImage");
            if (func.IsNull()) {
                Log(ErrorInfo::mtError, "CScriptUploadEngine::doUpload\r\n" + std::string("Function SearchByImage not found in the script"));
                currentTask_ = nullptr;
                return -1;
            }
            std::string fileName = searchTask->getFileName();
            auto [status, table] = GetOperationResult(func.Evaluate<Object>(fileName.c_str(), &params));
            /*SharedPtr<int> ivalPtr*/ ival = status;
            /* if (ival > 0) {
                ival = !params.DirectUrl.empty();
            }*/
        }
    }
    catch (NetworkClient::AbortedException & ) {
        throw;
    }
    catch (ServerSyncException& e) {
        Log(ErrorInfo::mtError, "CScriptUploadEngine::doUpload\r\n" + std::string(e.what()));
        ival = -1; // fatal error
    }
	/*catch (const Sqrat::Exception& e) {
    	if (!strcmp(e.what(), "unauthorized_exception")) {
            ival = -2;
    	} else {
            Log(ErrorInfo::mtError, "CScriptUploadEngine::doUpload\r\n" + std::string(e.what()));
    	}
    }*/
    catch (std::exception & e) {
        Log(ErrorInfo::mtError, "CScriptUploadEngine::doUpload\r\n" + std::string(e.what()));
    }

    FlushSquirrelOutput();
    currentTask_ = nullptr;
    return ival;
}

bool CScriptUploadEngine::needStop()
{
    bool shouldStop = false;
    if (onNeedStop_) {
        shouldStop = onNeedStop_();  // delegate call
    }
    return shouldStop;
}

bool CScriptUploadEngine::preLoad()
{
    try {
        ServerSettingsStruct* par = m_ServersSettings;
        Sqrat::RootTable& rootTable = vm_.GetRootTable();
        rootTable.SetInstance("ServerParams", par);
        rootTable.SetInstance("Sync", serverSync_.get());
    } catch (std::exception& e) {
        Log(ErrorInfo::mtError, "CScriptUploadEngine::preLoad failed\r\n" + std::string("Error: ") + e.what());
        return false;
    }
    FlushSquirrelOutput();
    return true;
}

bool CScriptUploadEngine::postLoad()
{
    ScriptAPI::RegisterShortTranslateFunctions(vm_);
	if (functionExists("Authenticate")) {
        newAuthMode_ = true;
	}
    hasRefreshTokenFunc_ = functionExists("RefreshToken");
    return true;
}

void CScriptUploadEngine::logNetworkError(bool error, const std::string& msg) {
    std::thread::id threadId = std::this_thread::get_id();
    Log(error ? (ErrorInfo::mtError) : (ErrorInfo::mtWarning), name_ + ".nut [" + "ThreadId=" + IuCoreUtils::ThreadIdToString(threadId) + "]\r\n" +msg);
}

int CScriptUploadEngine::getAccessTypeList(std::vector<std::string>& list)
{
    using namespace Sqrat;
    try
    {
        checkCallingThread();

        Function func(vm_.GetRootTable(), "GetFolderAccessTypeList");
        if (func.IsNull()) {

            return -1;
        }
        SharedPtr<Sqrat::Array> arr = func.Evaluate<Sqrat::Array>();

        /*if ( Error::Occurred(vm_.GetVM() ) ) {
            Log(ErrorInfo::mtError, "CScriptUploadEngine::getAccessTypeList\r\n" + std::string(Error::Message(vm_.GetVM())));
            return 0;
        }*/

        list.clear();
        auto count = arr->GetSize();
        for (auto i = 0; i < count; i++)
        {
            std::string title = arr->GetSlot(i).Cast<std::string>();
            list.push_back(title);
        }
    }
    catch (std::exception& e)
    {
        Log(ErrorInfo::mtError, "CScriptUploadEngine::getAccessTypeList\r\n" + std::string(e.what()));
    }
    FlushSquirrelOutput();
    return 1;
}

int CScriptUploadEngine::getServerParamList(ParameterList& list)
{
    using namespace Sqrat;

    try
    {
        checkCallingThread();
        Function func(vm_.GetRootTable(), "GetServerParamList");
        if (func.IsNull()) {
            return -1;
        }
        SharedPtr<Table> arr = func.Evaluate<Sqrat::Table>();

        if (!arr)
        {
            Log(ErrorInfo::mtError, "CScriptUploadEngine::getServerParamList\r\n" + std::string("GetServerParamList result is NULL"));

            return -1;
        }

        SqTableToParameterList(arr, list);
    }
    catch (const std::exception& e)
    {
        Log(ErrorInfo::mtError, "CScriptUploadEngine::getServerParamList\r\n" + std::string(e.what()));
    }
    FlushSquirrelOutput();
    return 1;
}

int CScriptUploadEngine::doLogin()
{
    using namespace Sqrat;

    try
    {
        checkCallingThread();

        Function func(vm_.GetRootTable(), newAuthMode_ ? "Authenticate" : "DoLogin");
        if (func.IsNull()) {
            return 0;
        }
        auto [res, table] = GetOperationResult(func.Evaluate<Object>());

        serverSync_->setAuthPerformed(res == 1);
        /*if ( Error::Occurred(vm_.GetVM() ) ) {
            Log(ErrorInfo::mtError, "CScriptUploadEngine::doLogin\r\n" + std::string(Error::Message(vm_.GetVM())));
            return 0;
        }*/

        return res;
    } catch (const INetworkClient::AbortedException& ex) {
        serverSync_->setAuthPerformed(false);
        Log(ErrorInfo::mtWarning, _("Operation was cancelled by user."));
    }
    catch (const std::exception& e)
    {
        serverSync_->setAuthPerformed(false);
        Log(ErrorInfo::mtError, "CScriptUploadEngine::doLogin\r\n" + std::string(e.what()));
    }
    FlushSquirrelOutput();
    return 0;
}

int CScriptUploadEngine::doLogout() {
    using namespace Sqrat;
    try
    {
        checkCallingThread();

        Function func(vm_.GetRootTable(), "DoLogout");
        if (func.IsNull()) {
            return 0;
        }
        auto [res, table] = GetOperationResult(func.Evaluate<Object>());
        return res;
    } catch (const INetworkClient::AbortedException& ex) {
        serverSync_->setAuthPerformed(false);
        Log(ErrorInfo::mtWarning, _("Operation was cancelled by user."));
    } catch (const std::exception& e)
    {
        Log(ErrorInfo::mtError, "CScriptUploadEngine::doLogout\r\n" + std::string(e.what()));
    }
    FlushSquirrelOutput();
    return 0;
}

int CScriptUploadEngine::modifyFolder(CFolderItem& folder)
{
    if (checkAuth() < 1) {
        return 0;
    }
    using namespace Sqrat;
    int res = 1;
    try
    {
        checkCallingThread();
        Function func(vm_.GetRootTable(), "ModifyFolder");
        if (func.IsNull()) {
            Log(ErrorInfo::mtError, "CScriptUploadEngine::modifyFolder\r\n" + std::string("Function ModifyFolder not found in script"));
            return 0;
        }
        auto [status, table] = GetOperationResult(func.Evaluate<Object>(&folder));
        res = status;
        /*if ( Error::Occurred(vm_.GetVM() ) ) {
            Log(ErrorInfo::mtError, "CScriptUploadEngine::doLogin\r\n" + std::string(Error::Message(vm_.GetVM())));
            return 0;
        }*/
    }
    catch (std::exception& e)
    {
        Log(ErrorInfo::mtError, "CScriptUploadEngine::getServerParamList\r\n" + std::string(e.what()));
    }
    FlushSquirrelOutput();
    return res;
}

int CScriptUploadEngine::getFolderList(CFolderList& FolderList)
{
    if (checkAuth() < 1) {
        return 0;
    }
    using namespace Sqrat;
    int ival = 0;
    try
    {
        checkCallingThread();
        Function func(vm_.GetRootTable(), "GetFolderList");
        if (func.IsNull()) {
            return -1;
        }
        //FolderList.setParentFolder(parent);
        auto [status, table] = GetOperationResult(func.Evaluate<Object>(&FolderList));
        ival = status;
        /*if ( Error::Occurred(vm_.GetVM() ) ) {
            Log(ErrorInfo::mtError, "CScriptUploadEngine::getFolderList\r\n" + std::string(Error::Message(vm_.GetVM())));
        }*/
    }
    catch (NetworkClient::AbortedException& ) {
        FlushSquirrelOutput();
        return 0;
    }
    catch (std::exception& e)
    {
        Log(ErrorInfo::mtError, "CScriptUploadEngine::getFolderList\r\n" + std::string(e.what()));
    }
    FlushSquirrelOutput();
    return ival;
}

std::string CScriptUploadEngine::name() const
{
    return name_;
}

int CScriptUploadEngine::createFolder(const CFolderItem& parent, CFolderItem& folder)
{
    if (checkAuth() < 1) {
        return 0;
    }
    using namespace Sqrat;
    int ival = 0;
    try
    {
        checkCallingThread();
        Function func(vm_.GetRootTable(), "CreateFolder");
        if (func.IsNull()) {
            return -1;
        }

        auto [res, table] = GetOperationResult(func.Evaluate<Object>(&parent, &folder));

        ival = res;
        /*if ( Error::Occurred(vm_.GetVM() ) ) {
            Log(ErrorInfo::mtError, "CScriptUploadEngine::createFolder\r\n" + std::string(Error::Message(vm_.GetVM())));
            return false;
        }*/
    }
    catch (std::exception& e)
    {
        Log(ErrorInfo::mtError, "CScriptUploadEngine::createFolder\r\n" + std::string(e.what()));
    }
    FlushSquirrelOutput();
    return ival;
}

void CScriptUploadEngine::setNetworkClient(INetworkClient* nm)
{
    CAdvancedUploadEngine::setNetworkClient(nm);
    if (!m_UploadData->UserAgent.empty()) {
        nm->setUserAgent(m_UploadData->UserAgent);
    }
    nm->setCurlShare(sync_->getCurlShare());
    nm->setLogger(this);
    vm_.GetRootTable().SetInstance("nm", nm);
}

bool CScriptUploadEngine::supportsSettings()
{
    using namespace Sqrat;

    try {
        checkCallingThread();
        Function func(vm_.GetRootTable(), "GetServerParamList");
        if (func.IsNull()) {
            return false;
        }
    } catch (std::exception& e) {
        Log(ErrorInfo::mtError, "CScriptUploadEngine::supportsSettings\r\n" + std::string(e.what()));
        return false;
    }
    FlushSquirrelOutput();
    return true;
}

bool CScriptUploadEngine::supportsBeforehandAuthorization()
{
    return newAuthMode_ || functionExists("DoLogin");
}

bool CScriptUploadEngine::functionExists(const std::string& name){
    using namespace Sqrat;
    try {
        checkCallingThread();
        Function func(vm_.GetRootTable(), name.c_str());
        if (func.IsNull()) {
            return false;
        }
    }
    catch (std::exception& e) {
        Log(ErrorInfo::mtError, "CScriptUploadEngine::supportsBeforehandAuthorization\r\n" + std::string(e.what()));
        return false;
    }
    FlushSquirrelOutput();
    return true;
}

void CScriptUploadEngine::stop()
{
    ScriptAPI::StopAssociatedServices(vm_.GetVM());
    CAdvancedUploadEngine::stop();
}

void CScriptUploadEngine::Log(ErrorInfo::MessageType mt, const std::string& error)
{
    ErrorInfo ei;
    ei.ActionIndex = -1;
    ei.messageType = mt;
    ei.errorType = etUserError;
    ei.error = error;
    ei.sender = "CScriptUploadEngine";
    if (m_UploadData) {
        ei.ServerName = m_UploadData->Name;
        ei.uploadEngineData = m_UploadData;
    }
    if (currentTask_) {
        ei.FileName = currentTask_->toString();
        UploadTask* task = currentTask_->parentTask() ? currentTask_->parentTask() : currentTask_.get();
        auto* fileTask = dynamic_cast<FileUploadTask*>(task);
        if (fileTask) {
            ei.TopLevelFileName = fileTask->getFileName();
        }
    }
    ei.ThreadId = std::this_thread::get_id();
    ei.Script = IuCoreUtils::ExtractFileName(fileName_);

    ErrorMessage(ei);
}

bool CScriptUploadEngine::isAuthenticated() {
    using namespace Sqrat;
    try
    {
        checkCallingThread();

        Function func(vm_.GetRootTable(), "IsAuthenticated");
        if (func.IsNull()) {
            return false;
        }
        int res = ScriptAPI::GetValue(func.Evaluate<int>());
        return res;
    }
    catch (std::exception& e)
    {
        Log(ErrorInfo::mtError, "CScriptUploadEngine::isAuthenticated\r\n" + std::string(e.what()));
    }
    FlushSquirrelOutput();
    return false;
}

bool CScriptUploadEngine::supportsLogout() {
    using namespace Sqrat;
    try {
        checkCallingThread();
        Function func(vm_.GetRootTable(), "DoLogout");
        if (func.IsNull()) {
            return false;
        }
    }
    catch (std::exception& e) {
        Log(ErrorInfo::mtError, "CScriptUploadEngine::supportsLogout\r\n" + std::string(e.what()));
        return false;
    }
    FlushSquirrelOutput();
    return true;
}

int CScriptUploadEngine::checkAuth() {
    using namespace Sqrat;
    int res = 1, res2 = 1;

    if (newAuthMode_) {
        try {
            serverSync_->beginAuth();
        }
        catch (const ServerSyncException&) {
            return 0;
        }
    }

    defer d([&] { // Run at function exit
        if (newAuthMode_) {
            serverSync_->endAuth();
        }
    });

	if (newAuthMode_ && m_UploadData->NeedAuthorization && m_ServersSettings->authData.DoAuth && !serverSync_->isAuthPerformed()) {
        try {
            checkCallingThread();

            if (!isAuthenticated()) {
                try {
                    res = doLogin();
                } catch (const std::exception& e) {
                    Log(ErrorInfo::mtError, "CScriptUploadEngine::checkAuth\r\n" + std::string(e.what()));
                }
            }
        } catch (const std::exception& e) {
            Log(ErrorInfo::mtError, "CScriptUploadEngine::checkAuth\r\n" + std::string(e.what()));
        }
	}

    if (hasRefreshTokenFunc_) {
        res2 = refreshToken();
    }
    return static_cast<int>(res>=1 && res2>=1);
}

int CScriptUploadEngine::refreshToken()
{
    using namespace Sqrat;

    try
    {
        checkCallingThread();

        std::lock_guard<std::mutex> lk(serverSync_->refreshTokenMutex());
        Function func(vm_.GetRootTable(), "RefreshToken");
        if (func.IsNull()) {
            return 0;
        }
        auto [res, table] = GetOperationResult(func.Evaluate<Object>());

        return res;
    }
    catch (std::exception& e)
    {
        Log(ErrorInfo::mtError, "CScriptUploadEngine::doLogin\r\n" + std::string(e.what()));
    }
    FlushSquirrelOutput();
    return 0;
}
