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

#include "ScriptAPI.h"

#include "RegularExpression.h"
#include "Process.h"
#include "GumboBingings/GumboDocument.h"
#ifdef _WIN32
//#if defined(IU_WTL) && !defined(IU_NOWEBBROWSER)
#include "WebBrowser.h"
#include "WebBrowserPrivateBase.h"
#endif

#include "Core/Network/NetworkClient.h"
#include "Core/Utils/SimpleXml.h"
#include "Core/Upload/UploadEngine.h"
#include "Core/Upload/FolderList.h"
#include "Core/Scripting/Squirrelnc.h"
#include "Core/Upload/ServerSync.h"
#include "Core/Scripting/Squirrelnc.h"
#include "WebServer.h"

#include <set>
#include <unordered_map>


#ifndef _WIN32
#include <unistd.h>
#endif

namespace ScriptAPI {;

std::unordered_map<HSQUIRRELVM, std::string> squirrelOutput;
std::mutex squirrelOutputMutex;
std::unordered_map<HSQUIRRELVM, PrintCallback> printCallbacks;
std::mutex printCallbacksMutex;
std::unordered_map<HSQUIRRELVM, std::string> scriptNames;
std::mutex scriptNamesMutex;
std::unordered_map<HSQUIRRELVM, std::string> currentTopLevelFileName;
std::mutex currentTopLevelFileNameMutex;

#ifndef _MSC_VER
int _vscprintf(const char * format, va_list pargs) {
    int retval;
    va_list argcopy;
    va_copy(argcopy, pargs);
    retval = vsnprintf(NULL, 0, format, argcopy);
    va_end(argcopy);
    return retval;
}
#endif
void printFunc(HSQUIRRELVM v, const SQChar* s, ...)
{
    std::lock_guard<std::mutex> lock(squirrelOutputMutex);
    va_list vl;
    va_start(vl, s);
    int len = /*1024; /*/ _vscprintf(s, vl) + 1;
    auto buffer = std::make_unique<char[]>(len + 1);
    vsnprintf(buffer.get(), len, s, vl);
    va_end(vl);
    squirrelOutput[v] += buffer.get();
}

void RegisterConstants(Sqrat::SqratVM& vm) {
    using namespace Sqrat;

    ConstTable(vm.GetVM()).Enum("ResultCode", Enumeration(vm.GetVM()).
        Const("FatalError", static_cast<int>(ResultCode::FatalError)).
        Const("TryAgain", static_cast<int>(ResultCode::TryAgain)).
        Const("FatalServerError", static_cast<int>(ResultCode::FatalServerError)).
        Const("Failure", static_cast<int>(ResultCode::Failure)).
        Const("Success", static_cast<int>(ResultCode::Success))
    );
}

void RegisterNetworkClientClass(Sqrat::SqratVM& vm) {
    using namespace Sqrat;
    vm.GetRootTable().Bind("NetworkClient", Class<INetworkClient>(vm.GetVM(), "NetworkClient").
        Func("doGet", &INetworkClient::doGet).
        Func("responseBody", &INetworkClient::responseBody).
        Func("responseCode", &INetworkClient::responseCode).
        Func("setUrl", &INetworkClient::setUrl).
        Func("doPost", &INetworkClient::doPost).
        Func("addQueryHeader", &INetworkClient::addQueryHeader).
        Func("addQueryParam", &INetworkClient::addQueryParam).
        Func("addQueryParamFile", &INetworkClient::addQueryParamFile).
        Func("addPostField", &INetworkClient::addPostField).
        Func("addPostFieldFile", &INetworkClient::addPostFieldFile).
        Func("responseHeaderCount", &INetworkClient::responseHeaderCount).
        Func("urlEncode", &INetworkClient::urlEncode).
        Func("urlDecode", &INetworkClient::urlDecode).
        Func("errorString", &INetworkClient::errorString).
        Func("doUpload", &INetworkClient::doUpload).
        Func("setMethod", &INetworkClient::setMethod).
        Func("setCurlOption", &INetworkClient::setCurlOption).
        Func("setCurlOptionInt", &INetworkClient::setCurlOptionInt).
        Func("doUploadMultipartData", &INetworkClient::doUploadMultipartData).
        Func("enableResponseCodeChecking", &INetworkClient::enableResponseCodeChecking).
        Func("setChunkSize", &INetworkClient::setChunkSize).
        Func("setChunkOffset", &INetworkClient::setChunkOffset).
        Func("setUserAgent", &INetworkClient::setUserAgent).
        Func("responseHeaderText", &INetworkClient::responseHeaderText).
        Func("responseHeaderByName", &INetworkClient::responseHeaderByName).
        Func("getCurlInfoString", &INetworkClient::getCurlInfoString).
        Func("getCurlInfoInt", &INetworkClient::getCurlInfoInt).
        Func("getCurlInfoDouble", &INetworkClient::getCurlInfoDouble).
        Func("getCurlResult", &INetworkClient::getCurlResult).
        Func("getCurlResultString", &INetworkClient::getCurlResultString).
        Func("setReferer", &INetworkClient::setReferer).
        Func("setUploadAction", &INetworkClient::setUploadAction));
}

namespace SimpleXmlExtend
{
SimpleXmlNode& Each(SimpleXmlNode* pthis, Sqrat::Function callback) {
    pthis->each([&callback](int i, SimpleXmlNode& child) {
        Sqrat::SharedPtr<bool> res = callback.Evaluate<bool>(i, child);

        if (!!res && *res) {
            return true;
        }
        i++;
        return false;
    });

    return *pthis;
}

Sqrat::Array GetChildren(SimpleXmlNode* pthis, const std::string& name) {
    std::vector<SimpleXmlNode> childs;
    pthis->GetChilds(name, childs);
    Sqrat::Array res(GetCurrentThreadVM(), childs.size());
    res.Resize(childs.size());
    int i = 0;
    for (auto& child : childs) {
        res.SetValue(i++, child);
    }
    return res;
}

};

void RegisterSimpleXmlClass(Sqrat::SqratVM& vm) {
    using namespace Sqrat;
     vm.GetRootTable().Bind("SimpleXml", Class<SimpleXml>(vm.GetVM(), "SimpleXml").
         Func("LoadFromFile", &SimpleXml::LoadFromFile).
         Func("LoadFromString", &SimpleXml::LoadFromString).
         Func("SaveToFile", &SimpleXml::SaveToFile).
         Func("ToString", &SimpleXml::ToString).
         Func("GetRoot", &SimpleXml::getRoot)
     );

     vm.GetRootTable().Bind("SimpleXmlNode", Class<SimpleXmlNode>(vm.GetVM(), "SimpleXmlNode").
         Func("Attribute", &SimpleXmlNode::Attribute).
         Func("AttributeInt", &SimpleXmlNode::AttributeInt).
         Func("AttributeBool", &SimpleXmlNode::AttributeBool).
         Func("Name", &SimpleXmlNode::Name).
         Func("Text", &SimpleXmlNode::Text).
         Func("CreateChild", &SimpleXmlNode::CreateChild).
         Func("GetChild", &SimpleXmlNode::GetChild).
         Func("SetAttribute", &SimpleXmlNode::SetAttributeString).
         Func("SetAttributeInt", &SimpleXmlNode::SetAttributeInt).
         Func("SetAttributeBool", &SimpleXmlNode::SetAttributeBool).
         Func("SetText", &SimpleXmlNode::SetText).
         Func("IsNull", &SimpleXmlNode::IsNull).
         Func("DeleteChilds", &SimpleXmlNode::DeleteChilds).
         Func("GetChildCount", &SimpleXmlNode::GetChildCount).
         Func("GetChildByIndex", &SimpleXmlNode::GetChildByIndex).
         Func("GetAttributeCount", &SimpleXmlNode::GetAttributeCount).
         GlobalFunc("each", SimpleXmlExtend::Each).
         GlobalFunc("Each", SimpleXmlExtend::Each).
         GlobalFunc("GetChildren", SimpleXmlExtend::GetChildren)
    );
}

void RegisterUploadClasses(Sqrat::SqratVM& vm) {
    using namespace Sqrat;
    RootTable& root = vm.GetRootTable();
    root.Bind("CFolderItem", Class<CFolderItem>(vm.GetVM(), "CFolderItem").
        Func("getId", &CFolderItem::getId).
        Func("getParentId", &CFolderItem::getParentId).
        Func("getSummary", &CFolderItem::getSummary).
        Func("getTitle", &CFolderItem::getTitle).
        Func("setId", &CFolderItem::setId).
        Func("setParentId", &CFolderItem::setParentId).
        Func("setSummary", &CFolderItem::setSummary).
        Func("setTitle", &CFolderItem::setTitle).
        Func("getAccessType", &CFolderItem::getAccessType).
        Func("setAccessType", &CFolderItem::setAccessType).
        Func("setItemCount", &CFolderItem::setItemCount).
        Func("setViewUrl", &CFolderItem::setViewUrl).
        Func("getItemCount", &CFolderItem::getItemCount)
    );

    root.Bind("CIUUploadParams", Class<UploadParams>(vm.GetVM(), "CIUUploadParams").
        Func("getFolderID", &UploadParams::getFolderID).
        Func("getFolder", &UploadParams::getFolder).
        Func("setDirectUrl", &UploadParams::setDirectUrl).
        Func("getDirectUrl", &UploadParams::getDirectUrl).
        Func("setThumbUrl", &UploadParams::setThumbUrl).
        Func("getThumbUrl", &UploadParams::getThumbUrl).
        Func("setViewUrl", &UploadParams::setViewUrl).
        Func("getViewUrl", &UploadParams::getViewUrl).
        Func("setDeleteUrl", &UploadParams::setDeleteUrl).
        Func("getDeleteUrl", &UploadParams::getDeleteUrl).
        Func("setEditUrl", &UploadParams::setEditUrl).
        Func("getEditUrl", &UploadParams::getEditUrl).
        Func("getServerFileName", &UploadParams::getServerFileName).
        Func("getTask", &UploadParams::getTask).
        //Func("getFileTask", &UploadParams::getFileTask).
        //Func("getUrlShorteningTask", &UploadParams::getUrlShorteningTask).
        Func("getParam", &UploadParams::getParam)
    );

    root.Bind("CFolderList", Class<CFolderList>(vm.GetVM(), "CFolderList").
        Func("AddFolder", &CFolderList::AddFolder).
        Func("AddFolderItem", &CFolderList::AddFolderItem).
        Func("GetCount", &CFolderList::GetCount).
        Func("setParentFolder", &CFolderList::setParentFolder).
        Func("parentFolder", &CFolderList::parentFolder)
    );


    root.Bind("ServerSettingsStruct", Class<ServerSettingsStruct>(vm.GetVM(), "ServerSettingsStruct").
        Func("setParam", &ServerSettingsStruct::setParam).
        Func("getParam", &ServerSettingsStruct::getParam)
    );

    Class<ThreadSync> threadSyncClass(vm.GetVM(), "ThreadSync");
    threadSyncClass.Func("setValue", &ThreadSync::setValue).
        Func("getValue", &ThreadSync::getValue);

    root.Bind("ServerSync", DerivedClass<ServerSync, ThreadSync>(vm.GetVM(), "ServerSync").
        Func("beginAuth", &ServerSync::beginAuth).
        Func("endAuth", &ServerSync::endAuth).
        Func("isAuthPerformed", &ServerSync::isAuthPerformed).
        Func("setAuthPerformed", &ServerSync::setAuthPerformed).
        Func("isAuthPerformedSuccess", &ServerSync::isAuthPerformedSuccess)
    );
}

#ifdef _MSC_VER
#ifndef IU_TESTS // to avoid vld memory leaks messages; We use just one thread in test cases
__declspec(thread)
#endif
HSQUIRRELVM threadVm;
#else
thread_local  HSQUIRRELVM threadVm;
#endif

std::unordered_map<HSQUIRRELVM, std::set<Stoppable*>> vmServices;
std::mutex vmServicesMutex;

void RegisterClasses(Sqrat::SqratVM& vm) {
   // Sqrat::DefaultVM::Set(vm.GetVM());

    RegisterNetworkClientClass(vm);
    RegisterRegularExpressionClass(vm);
    RegisterUploadClasses(vm);
    RegisterUploadTaskWrappers(vm);
    RegisterProcessClass(vm);
    RegisterSimpleXmlClass(vm);
    RegisterGumboClasses(vm);
    RegisterWebServerClass(vm);

//#if defined(IU_WTL) && !defined(IU_NOWEBBROWSER)
#ifdef _WIN32
    RegisterWebBrowserClass(vm);
    //RegisterHtmlDocumentClass(vm);
    //RegisterHtmlElementClass(vm);
#endif
}
void RegisterAPI(Sqrat::SqratVM& vm)
{
    threadVm = vm.GetVM();
    //sq_setcompilererrorhandler(vm_.GetVM(), CompilerErrorHandler);
    sq_setprintfunc(vm.GetVM(), printFunc, printFunc);
    RegisterConstants(vm);
    RegisterFunctions(vm);
    RegisterClasses(vm);
}
void CleanUp()
{
    CleanUpFunctions();
}

HSQUIRRELVM GetCurrentThreadVM()
{
    return threadVm;
}
void SetCurrentThreadVM(HSQUIRRELVM vm) {
    threadVm = vm;
}

void StopAssociatedServices(HSQUIRRELVM vm)
{
    {
        std::lock_guard<std::mutex> guard(vmServicesMutex);
        for (auto* it : vmServices[vm]) {
            it->stop();
        }
    }
}

void AddServiceToVM(HSQUIRRELVM vm, Stoppable* service)
{
    std::lock_guard<std::mutex> guard(vmServicesMutex);
    vmServices[vm].insert(service);
}

void RemoveServiceFromVM(HSQUIRRELVM vm, Stoppable* service)
{
    try
    {
        std::lock_guard<std::mutex> guard(vmServicesMutex);
        vmServices[vm].erase(service);
    } catch (std::exception& ex)
    {
        LOG(ERROR) << ex.what();
    }
}

void SetPrintCallback(Sqrat::SqratVM& vm, const PrintCallback& callback)
{
    std::lock_guard<std::mutex> guard(printCallbacksMutex);
    printCallbacks[vm.GetVM()] = callback;
}

void SetScriptName(Sqrat::SqratVM& vm, const std::string& fileName)
{
    std::lock_guard<std::mutex> lock(scriptNamesMutex);
    scriptNames[vm.GetVM()] = fileName;
}

void SetCurrentTopLevelFileName(Sqrat::SqratVM& vm, const std::string& fileName)
{
    std::lock_guard<std::mutex> lock(currentTopLevelFileNameMutex);
    currentTopLevelFileName[vm.GetVM()] = fileName;
}

std::string GetCurrentTopLevelFileName() {
    std::lock_guard<std::mutex> lock(currentTopLevelFileNameMutex);
    auto it = currentTopLevelFileName.find(GetCurrentThreadVM());
    if (it != currentTopLevelFileName.end()) {
        return it->second;
    }
    return {};
}
void ClearVmData(Sqrat::SqratVM& vm)
{
    std::lock_guard<std::mutex> lock(scriptNamesMutex);
    auto it = scriptNames.find(vm.GetVM());
    if (it != scriptNames.end())
    {
        scriptNames.erase(it);
    }

    auto it2 = currentTopLevelFileName.find(vm.GetVM());
    if (it2 != currentTopLevelFileName.end()) {
        currentTopLevelFileName.erase(it2);
    }
}

std::string GetScriptName(HSQUIRRELVM vm)
{
    std::lock_guard<std::mutex> lock(scriptNamesMutex);
    return scriptNames[vm];
}

void FlushSquirrelOutput(HSQUIRRELVM vm) {
    std::lock_guard<std::mutex> guard(squirrelOutputMutex);
    std::string& output = squirrelOutput[vm];
    std::lock_guard<std::mutex> guard2(printCallbacksMutex);

    if (!output.empty())
    {
        auto it = printCallbacks.find(vm);
        if (it != printCallbacks.end())
        {
            PrintCallback& callback = it->second;
            if (callback)
            {
                callback(output);
            }

        }
        output.clear();
    }
}

}

HSQUIRRELVM GetCurrentThreadHVM()
{
    return ScriptAPI::GetCurrentThreadVM();
}

