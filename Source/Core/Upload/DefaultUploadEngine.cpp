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

#include "DefaultUploadEngine.h"

#include <unordered_set>
#include <boost/format.hpp>
#include <json/json.h>

#include "Core/3rdpart/pcreplusplus.h"
#include "UrlShorteningTask.h"
#include "FileUploadTask.h"
#include "SearchByImageUrlTask.h"
#include "SearchByImageFileTask.h"
#include "Core/Utils/StringUtils.h"
#include "ServerSync.h"
#include "Core/Utils/TextUtils.h"
#include "Core/Utils/DesktopUtils.h"
#include "Core/i18n/Translator.h"

namespace {
    const std::unordered_set<std::string_view> ALLOWED_ACTION_TYPES = { "get", "post", "put", "patch", "delete", "head", "options", "openurl", "login", "upload" };
}

CDefaultUploadEngine::CDefaultUploadEngine(std::shared_ptr<ServerSync> serverSync, ErrorMessageCallback errorCallback)
    : CAbstractUploadEngine(std::move(serverSync), std::move(errorCallback)), mt_(randomDevice_())
{
    m_CurrentActionIndex = -1;
    fatalError_ = false;
}

int CDefaultUploadEngine::processTask(std::shared_ptr<UploadTask> task, UploadParams& params) {
    if (task->type() == UploadTask::TypeAuth) {
        UploadError(ErrorInfo::mtError, "Upload task of type '" + task->toString() + "' is not supported", 0, false);
        return 0;
    }

    return doUpload(task, params);
}

int CDefaultUploadEngine::doUpload(std::shared_ptr<UploadTask> task, UploadParams& params) {
    fatalError_ = false;
    int res = 0;
    currentTask_ = task;
    try {

        if (task->type() == UploadTask::TypeFile) {
            res = doUploadFile(std::dynamic_pointer_cast<FileUploadTask>(task), params);
        } else if (task->type() == UploadTask::TypeUrl) {
            res = doUploadUrl(std::dynamic_pointer_cast<UrlShorteningTask>(task), params);
        } else if (task->type() == UploadTask::TypeSearchByImageFile) {
            res = doUploadFile(std::dynamic_pointer_cast<SearchByImageFileTask>(task), params);
        } else if (task->type() == UploadTask::TypeSearchByImageUrl) {
            res = doSearchImageByUrl(std::dynamic_pointer_cast<SearchByImageUrlTask>(task), params);
        } else {
            UploadError(true, "Upload task of type '" + task->toString() + "' is not supported", nullptr, false);
        }
    } catch (const INetworkClient::AbortedException& ex) {
        UploadError(false, _("Operation was cancelled by user."), nullptr, false);
        res = 0;
    } catch(std::exception& ex) {
        UploadError(false, ex.what(), nullptr, false);
        res = 0;
    }
    if (!res && fatalError_)
    {
        return -1;
    }
    return res;
}

bool CDefaultUploadEngine::doUploadFile(std::shared_ptr<FileUploadTask> task, UploadParams& params) {
    const std::string fileName = task->getFileName();
    const std::string displayName = task->getDisplayName();
    if ( fileName.empty() ) {
        UploadError( true, "Filename should not be empty!", nullptr );
        return false;
    }

    if ( !m_UploadData ) {
        UploadError( true, "m_UploadData should not be NULL!", nullptr );
        return false;
    }
    m_FileName = fileName;
    m_displayFileName = displayName;

    prepareUpload(params);

    m_Vars["_FILENAME"] = IuCoreUtils::ExtractFileName(displayName);
    m_Vars["_FILENAMEWITHOUTEXT"] = IuCoreUtils::ExtractFileNameNoExt(displayName);
    m_Vars["_FILEEXT"] = IuCoreUtils::ExtractFileExt(displayName);
    m_Vars["_FILESIZE"] = std::to_string(IuCoreUtils::GetFileSize(fileName));

    if ( !executeActions()) {
        return false;
    }

    params.ThumbUrl  = ReplaceVars(m_UploadData->ThumbUrlTemplate);
    params.DirectUrl = ReplaceVars(m_UploadData->ImageUrlTemplate);
    params.ViewUrl   = ReplaceVars(m_UploadData->DownloadUrlTemplate);
    params.EditUrl = ReplaceVars(m_UploadData->EditUrlTemplate);
    params.DeleteUrl = ReplaceVars(m_UploadData->DeleteUrlTemplate);
    return true;
}

bool  CDefaultUploadEngine::doUploadUrl(std::shared_ptr<UrlShorteningTask> task, UploadParams& params) {
    prepareUpload(params);
    m_Vars["_ORIGINALURL"] = task->getUrl();
    bool actionsExecuteResult = executeActions();
    if ( !actionsExecuteResult ) {
        return false;
    }
    std::string m_ThumbUrl    = ReplaceVars( m_UploadData->ThumbUrlTemplate );
    std::string m_ImageUrl    = ReplaceVars( m_UploadData->ImageUrlTemplate );
    std::string m_DownloadUrl = ReplaceVars( m_UploadData->DownloadUrlTemplate );

    params.ThumbUrl  = m_ThumbUrl;
    params.DirectUrl = m_ImageUrl;
    params.ViewUrl   = m_DownloadUrl;
    if ( m_ImageUrl.empty() ) {
        UploadError( ErrorInfo::mtError, "Empty result", nullptr, false );
        return false;
    }
    return true;
}

bool CDefaultUploadEngine::doSearchImageByUrl(std::shared_ptr<SearchByImageUrlTask> task, UploadParams& params) {
    prepareUpload(params);
    m_Vars["_IMAGEURL"] = task->url();
    bool actionsExecuteResult = executeActions();
    if (!actionsExecuteResult) {
        return false;
    }

    return true;
}

void CDefaultUploadEngine::prepareUpload(UploadParams& params) {
    m_Vars.clear();
    if ( m_UploadData->NeedAuthorization ) {
        li = m_ServersSettings->authData;
        if ( li.DoAuth ) {
            m_Vars["_LOGIN"] = li.Login;
            m_Vars["_PASSWORD"] = li.Password;
        }
    }
    std::uniform_int_distribution<int> dist(0, 256 * 256);
    const int n = dist(mt_);
    m_Vars["_RAND16BITS"] = str(boost::format("%05d") % n);
    m_Vars["_THUMBWIDTH"] = std::to_string(params.thumbWidth);
    m_Vars["_THUMBHEIGHT"] = std::to_string(params.thumbHeight);
    m_Vars["_THUMBCREATE"] = std::to_string(params.createThumbnail);
    m_Vars["_THUMBADDTEXT"] = std::to_string(params.addTextOnThumb);
    m_Vars["_THUMBUSESERVER"] = std::to_string(params.useServerSideThumbnail);
    m_Vars["_THREADID"] = IuCoreUtils::ThreadIdToString(std::this_thread::get_id());
    m_NetworkClient->enableResponseCodeChecking(false);
    m_NetworkClient->setLogger(this);
}

bool CDefaultUploadEngine::executeActions() {
    m_NetworkClient->setTreatErrorsAsWarnings(true);
    for (const auto & action : m_UploadData->Actions) {
        int NumOfTries = 0;
        bool ActionRes = false;
        do {
            if ( needStop() ) {
                return false;
            }
            ActionRes = DoAction( action );
            NumOfTries ++;
            if (needStop())
                return false;

            if (!ActionRes ) {
                // Prepare error string which will be displayed in Log Window
                std::string ErrorStr = m_ErrorReason;
                ErrorType errorType;

                if (!m_ErrorReason.empty()) {
                    ErrorStr += m_ErrorReason;
                }

                if (NumOfTries == action.RetryLimit) {
                    errorType = etActionRetriesLimitReached;
                }
                else {
                    errorType = etActionRepeating;
                    ErrorStr += "retrying last action... ";
                    ErrorStr += "(" + std::to_string(NumOfTries + 1)
                        + " of " + std::to_string(action.RetryLimit) + ")";
                }
                UploadError( false, ErrorStr, nullptr, false );
            }
        }
        while (NumOfTries < action.RetryLimit && !ActionRes);
        if ( !ActionRes ) {
            if (action.Type == "login")
            {
                fatalError_ = true;
            }
            return false;
        }
    }
    return true;
}

void CDefaultUploadEngine::logNetworkError(bool error, const std::string& msg) {
    UploadError(error, msg, nullptr);
}

bool CDefaultUploadEngine::DoUploadAction(const UploadAction& Action, bool bUpload)
{
    try {

        AddQueryPostParams(Action);

        m_NetworkClient->setUrl(Action.Url);

        if ( bUpload ) {
            if (Action.Type == "put") {
                m_NetworkClient->setMethod( "PUT" );
                m_NetworkClient->doUpload(Action.Body.empty() ? m_FileName : "", Action.Body);
            } else {
                bool res = m_NetworkClient->doUploadMultipartData();
            }
        } else {
            m_NetworkClient->setMethod(IuStringUtils::ToUpper(Action.Type));
            m_NetworkClient->doPost(ReplaceVars(Action.Body));
        }

        return ReadServerResponse(Action);
    } catch (const INetworkClient::AbortedException& ex) {
        throw;
    } catch (const std::exception& ex) {
        LOG(ERROR) << ex.what() << std::endl;
        return false;
    }
}

bool CDefaultUploadEngine::DoGetAction(const UploadAction& Action)
{
    bool Result = false;

    try {
        m_NetworkClient->doGet( Action.Url );
        if (needStop()) {
            return false;
        }
        Result = ReadServerResponse(Action);
    } catch (const INetworkClient::AbortedException& ex) {
        throw;
    } catch (const std::exception& ex) {
        LOG(ERROR) << ex.what() << std::endl;
        return false;
    }
    return Result;
}

bool CDefaultUploadEngine::reg_single_match(const std::string& pattern, const std::string& text, std::string& res)
{
    pcrepp::Pcre reg(pattern, "imc"); // Case insensitive match
    if (reg.search(text)) {
        if ( reg.matches() > 0 ) {
            res = reg.get_match(1);
        }
        return true;
    }

    return false;
}

bool CDefaultUploadEngine::ParseAnswer(const UploadAction& Action, const std::string& Body)
{
    std::string DebugVars;
    if (!Action.FunctionCalls.empty() && m_UploadData->Debug) {
        DebugMessage(Body, true);
    }

    auto assignVars = [&](const ActionFunc& actionRegExp, auto callback) {
        if (actionRegExp.Variables.empty()) {
            DebugVars += "Variables list is empty!\r\n";
        }

        for (const auto & v : actionRegExp.Variables) {
            std::string temp;
            temp = callback(v.nIndex);
            if (!v.Name.empty()) {
                if (v.Name[0] == '_')
                {
                    serverSync_->setConstVar(v.Name, temp);
                }
                else
                {
                    m_Vars[v.Name] = temp;
                }
                DebugVars += v.Name + " = " + temp + "\r\n";
            }

        }
    };
    for (const auto& actionRegExp : Action.FunctionCalls)
    {
        if (!actionRegExp.getArg(1).empty()) {
            std::string codePage;
            if (reg_single_match("text/html;\\s+charset=([\\w-]+)", Body, codePage)) {
                IuCoreUtils::ConvertToUtf8(Body, codePage);
            }
            const std::string* data = &Body;
            std::string dataSrc;
            if (!actionRegExp.getArg(0).empty())
            {
                dataSrc = ReplaceVars(actionRegExp.getArg(0));
                data = &dataSrc;
            }
            // TODO: rewrite
            if (actionRegExp.Func == ActionFunc::FUNC_REGEXP) {
                try {
                    pcrepp::Pcre reg(actionRegExp.getArg(1), "imc");

                    DebugVars += "Regex: " + actionRegExp.getArg(1) + "\r\n\r\n";
                    if (reg.search(*data)) {
                        assignVars(actionRegExp, [&](int index) {
                            return reg.get_match(1 + index);
                        });
                    } else {
                        if (m_UploadData->Debug) {
                            DebugVars += "NO MATCHES FOUND!\r\n";
                        }

                        if (actionRegExp.Required) {
                            if (m_UploadData->Debug) {
                                DebugMessage(DebugVars);
                            }

                            UploadError(false, "Cannot obtain the necessary information from server response.", &Action);
                            return false; // ERROR! Current action failed!
                        }
                    }
                }
                catch (const std::exception& e) {
                    UploadError(true, std::string("Regular expression error:") + e.what(), &Action, false);
                }
                DebugVars += "\r\n";
            }
            else if (actionRegExp.Func == ActionFunc::FUNC_JSON) {
                Json::Value root;

                Json::Reader reader;
                reader.parse(*data, root, false);
                Json::Path jsonPath(ReplaceVars(actionRegExp.getArg(1)), Json::PathArgument());
                std::string result;
                const auto& resolved = jsonPath.resolve(root);
                if (resolved.isString()) {
                    result = resolved.asCString();
                } else {
                    std::stringstream sstr;
                    sstr << resolved;
                    result = sstr.str();
                }

                if (!resolved.isNull()) {
                    assignVars(actionRegExp, [&result](int index) -> std::string {
                        if (index == 0) {
                            return result;
                        }
                        return {};
                    });
                } else {
                    if (m_UploadData->Debug) {
                        DebugVars += "NO MATCHES FOUND!\r\n";
                    }

                    if (actionRegExp.Required) {
                        if (m_UploadData->Debug) {
                            DebugMessage(DebugVars);
                        }

                        UploadError(false, "Cannot obtain the necessary information from server response.", &Action);
                        return false; // ERROR! Current action failed!
                    }
                }

            }
        }

    }

    if (!DebugVars.empty() && m_UploadData->Debug) {
        DebugMessage(DebugVars);
    }

    return true;
}

bool CDefaultUploadEngine::DoAction(const UploadAction& Action)
{
    bool Result = true;

    m_CurrentActionIndex = Action.Index;
    if (Action.OnlyOnce)
    {
        if (m_PerformedActions[Action.Index]) {
            return true;
        }
    }

    defer d([this] {
        m_NetworkClient->setReferer({});
        m_NetworkClient->cleanupAfter();
    });

    if (!Action.Description.empty())
        SetStatus(stUserDescription, Action.Description);
    else
    {
        if (Action.Type == "upload") {
            SetStatus(stUploading);
        } else if (Action.Type == "login" && (m_UploadData->NeedAuthorization && li.DoAuth)) {
            SetStatus(stAuthorization);
        } else {
            SetStatus(stPerformingAction);
        }
    }

    UploadAction Current = Action;
    std::string temp = Current.Url;
    Current.Url = ReplaceVars(temp);
    if (m_UploadData->Debug) {
        if (Action.Type != "login" || (m_UploadData->NeedAuthorization && li.DoAuth ) ) {
            DebugMessage("\r\nType:" + Action.Type + "\r\nURL: " + Current.Url);
        }
    }

    if (!m_UploadData->UserAgent.empty())
    {
        m_NetworkClient->setUserAgent(m_UploadData->UserAgent);
    }
    AddCustomHeaders(Current);

    if (Action.Type == "upload" || Action.Type == "put") {
        Result = DoUploadAction(Current, true);
    } else if (Action.Type == "post") {
        Result = DoUploadAction(Current, false);
    } else if (Action.Type == "login") {
        if (m_UploadData->NeedAuthorization && li.DoAuth) {
            serverSync_->beginAuth();
            if (!serverSync_->isAuthPerformed()) {
                Result = DoUploadAction(Current, false);
                if (!Action.OnlyOnce)
                {
                    serverSync_->setAuthPerformed(Result);
                }
            }
            serverSync_->endAuth();
        }
    }
    else
    if (Action.Type == "get")
        Result = DoGetAction(Current);
    else if (Action.Type == "openurl") {
        Result = DesktopUtils::ShellOpenUrl(Current.Url);
    } else if (ALLOWED_ACTION_TYPES.find(Action.Type) != ALLOWED_ACTION_TYPES.end()) {
        Result = DoUploadAction(Current, false);
    } else {
        UploadError(true, "Unknown action type: " + Action.Type, &Action);
        Result = false;
    }

    if (Action.OnlyOnce)
    {
        if (Result)
            m_PerformedActions[Action.Index] = true;
    }

    if (Action.IgnoreErrors)
        return true;
    else
        return Result;
}

bool CDefaultUploadEngine::ReadServerResponse(const UploadAction& Action)
{
    bool Result = false;
    bool Exit = false;

    int StatusCode = m_NetworkClient->responseCode();

    if (!(StatusCode >= 200 && StatusCode <= 299) && !(StatusCode >= 300 && StatusCode <= 304))
    {
        std::string error;
        if (m_NetworkClient->getCurlResult() != CURLE_OK)
        {
            error = "Curl error: " + m_NetworkClient->getCurlResultString();
        }
        else
        {
            error += "Server response code: " + std::to_string(StatusCode) + "\r\n";
            error += m_NetworkClient->errorString();
        }
        if (!StatusCode)
            StatusCode = m_NetworkClient->getCurlResult();
        UploadError(false, error, &Action);
        return false;
    }

    std::string Refresh = m_NetworkClient->responseHeaderByName("Refresh");
    if (Refresh.empty()) {
        Refresh = m_NetworkClient->responseHeaderByName("refresh");
    }

    if (!Refresh.empty()) // Redirecting to URL
    {
        std::string redirectUrl;
        if (reg_single_match("url=(\\S+)", Refresh, redirectUrl))
        {
            UploadAction Redirect = Action;
            Redirect.Url = std::move(redirectUrl);
            Redirect.Referer = Action.Url;
            Redirect.Type = "get";
            Result = DoGetAction(Redirect);
            Exit = true;
        }
    }

    if (!Exit)
    {
        std::string answer = m_NetworkClient->responseBody();
        Result =  ParseAnswer(Action, answer);

        if (!Result && answer.empty())
        {
            UploadError(false, "Empty response!", &Action);
        }
    }
    return Result;
}

void CDefaultUploadEngine::AddQueryPostParams(const UploadAction& Action)
{
    std::string Txt = Action.PostParams;

    std::ostringstream message;
    if (m_UploadData->Debug) {
        message << "Post Request to URL: " << Action.Url << "\r\n";
    }

    pcrepp::Pcre reg2("\\\\;(*SKIP)(*FAIL)|;", "imcs");
    const std::string& str = Txt;
    auto strings = reg2.split(str);
    pcrepp::Pcre reg3("\\\\=(*SKIP)(*FAIL)|=", "imcs");

    for (const auto& item : strings) {
        auto tokens = reg3.split(item);
        if (tokens.size() < 2) {
            continue;
        }
        const std::string& varName = tokens[0];
        const std::string& varValue = tokens[1];

        if (varName.empty())
            continue;

        std::string NewValue = varValue;
        NewValue = IuStringUtils::Replace(NewValue, "\\;", ";");

        std::string NewName = varName;

        NewName = ReplaceVars(NewName);

        if (NewValue == "%filename%") {
            if (m_UploadData->Debug) {
                message << NewName << " = ** FILE CONTENTS ** \r\n";
            }
            m_NetworkClient->addPostFieldFile(NewName, m_FileName, IuCoreUtils::ExtractFileName(m_displayFileName),
                IuCoreUtils::GetFileMimeType(m_FileName));
        } else {
            NewValue = ReplaceVars(NewValue);
            if (m_UploadData->Debug) {
                message << NewName << " = " << NewValue << "\r\n";
            }
            m_NetworkClient->addPostField(NewName, NewValue);
        }
    }

    if (m_UploadData->Debug) {
        DebugMessage(message.str());
    }
}

void CDefaultUploadEngine::AddCustomHeaders(const UploadAction& Action)
{
    m_NetworkClient->setReferer(Action.Referer.empty() ? Action.Url : ReplaceVars(Action.Referer));

    std::string Txt = Action.CustomHeaders;
    size_t len = Txt.length();
    if (len)
    {
        if (Txt[len - 1] != ';')
        {
            Txt += ";";
        }

        pcrepp::Pcre reg("(.*?):(.*?[^\\x5c]{0,1});", "imc");

        std::string str = Txt;

        int pos = 0;
        while (pos < str.length())
        {

            if (reg.search(str, pos))
            {
                std::string VarName = reg[1];
                std::string VarValue = reg[2];
                pos = reg.get_match_end() + 1;

                if (VarName.empty())
                    continue;

                std::string NewValue = VarValue;
                NewValue = IuStringUtils::Replace(NewValue, "\\;", ";");

                std::string NewName = VarName;

                NewName = ReplaceVars(NewName);

                NewValue = ReplaceVars(NewValue);
                m_NetworkClient->addQueryHeader(NewName, NewValue);
            }
            else
                break;
        }
    }
}

std::string CDefaultUploadEngine::ReplaceVars(const std::string& Text)
{
    if (Text.empty())
    {
        return Text;
    }
    std::string Result =  Text;

    pcrepp::Pcre reg(R"(\$\(([A-z0-9_|]*?)\))", "imc");

    int pos = 0;
    while (pos <= Text.length())
    {
        if ( reg.search(Text, pos))
        {
            pos = reg.get_match_end() + 1;
            std::string vv = reg[1];
            std::string varName = vv;
            std::vector<std::string> tokens;
            IuStringUtils::Split(vv, "|", tokens, -1);
            if ( !tokens.empty() ) {
                varName = tokens[0];
            }
            std::string value;

            if (!vv.empty() ) {
                auto it = m_Vars.find(varName);  // first search variable in local map
                if (it != m_Vars.end()) {
                    value = it->second;
                } else if (vv[0] == '_') {
                    value = serverSync_->getConstVar(varName); // then search variable in shared map
                }
            }
            for ( size_t i = 1; i < tokens.size(); i++ ) {
                const std::string& modifier = tokens[i];
                if ( modifier == "urlencode" ) {
                    value = m_NetworkClient->urlEncode(value);
                } else if (modifier == "htmldecode")
                {
                    value = IuTextUtils::DecodeHtmlEntities(value);
                } else if (modifier == "jsonescape") {
                    value = Json::valueToQuotedString(value.c_str());
                }
            }

            Result = IuStringUtils::Replace(Result, "$(" + vv + ")", value);
        }
        else
            break;
    }
    return Result;
}

bool CDefaultUploadEngine::needStop()
{
    bool shouldStop = false;
    if (onNeedStop_)
        shouldStop = onNeedStop_();  // delegate call
    return shouldStop;
}

void CDefaultUploadEngine::SetStatus(StatusType status, const std::string& param)
{
    if (onStatusChanged_)
        onStatusChanged_(status, m_CurrentActionIndex, param);
}

int CDefaultUploadEngine::RetryLimit()
{
    return m_UploadData->RetryLimit;
}

void CDefaultUploadEngine::UploadError(bool error, const std::string& errorStr, const UploadAction* m_CurrentAction,
                                       bool writeToBuffer )
{
    m_LastError.ServerName = m_UploadData->Name;
    if (currentTask_) {
        m_LastError.FileName = currentTask_->toString();
        UploadTask* task = currentTask_->parentTask() ? currentTask_->parentTask() : currentTask_.get();
        auto fileTask = dynamic_cast<FileUploadTask*>(task);
        if (fileTask) {
            m_LastError.TopLevelFileName = fileTask->getFileName();
        }
    }
    if (m_CurrentAction)
    {
        m_LastError.ActionIndex = m_CurrentAction->Index + 1;
        m_LastError.Url = m_CurrentAction->Url;
        //m_LastError.RetryIndex = m_CurrentAction->NumOfTries + 1;
    }

    if (!m_LastError.error.empty())
        m_LastError.error += "\n";

    m_LastError.error += errorStr;

    if (!writeToBuffer)
    {
        m_LastError.messageType = error ? (ErrorInfo::mtError) : (ErrorInfo::mtWarning);
        m_LastError.errorType = etUserError;
        m_LastError.sender = "CDefaultUploadEngine";
        ErrorMessage(m_LastError);
        m_LastError.Clear();
    }
}
