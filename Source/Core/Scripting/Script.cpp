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

#include "Script.h"

#include "API/ScriptAPI.h"
#include "Core/Upload/ScriptUploadEngine.h"
#include "Core/Logging.h"
#include "Core/ThreadSync.h"

Script::Script(const std::string& fileName, std::shared_ptr<ThreadSync> serverSync, std::shared_ptr<INetworkClientFactory> networkClientFactory, bool doLoad)
{
    m_CreationTime = time(nullptr);
    m_bIsPluginLoaded = false;
    sync_ = std::move(serverSync);
    owningThread_ = std::this_thread::get_id();
    networkClientFactory_ = std::move(networkClientFactory);
    fileName_ = fileName;
    if (doLoad) {
        load(fileName);
    }
}

Script::~Script()
{
    ScriptAPI::ClearVmData(vm_);
}

void Script::CompilerErrorHandler(HSQUIRRELVM vm, const SQChar * desc, const SQChar * source, SQInteger line, SQInteger column) {
    sq_getprintfunc(vm)(vm, ("Script compilation failed\r\nFile:  " + std::string(source) + "\r\nLine: " + std::to_string(line)
        + "   Column: " + std::to_string(column) + "\r\n\r\n" + desc).c_str() );
}

void Script::InitScriptEngine()
{
    ScriptAPI::SetPrintCallback(vm_, [this](auto && PH1) { PrintCallback(std::forward<decltype(PH1)>(PH1)); });
    sqstd_seterrorhandlers(vm_.GetVM());
    ScriptAPI::SetScriptName(vm_, fileName_);
    sq_setcompilererrorhandler(vm_.GetVM(), CompilerErrorHandler);
}

void Script::DestroyScriptEngine()
{
    ScriptAPI::CleanUp();
}

void Script::FlushSquirrelOutput()
{
    ScriptAPI::FlushSquirrelOutput(vm_.GetVM());
}

bool Script::preLoad()
{
    networkClient_ = networkClientFactory_->create();
    networkClient_->setCurlShare(sync_->getCurlShare());
    Sqrat::RootTable& rootTable = vm_.GetRootTable();
    rootTable.SetInstance("Sync", sync_.get());
    rootTable.SetInstance("nm", networkClient_.get());
    return true;
}

bool Script::postLoad()
{
    return true;
}

bool Script::isLoaded() const
{
    return m_bIsPluginLoaded;
}

time_t Script::getCreationTime() const
{
    return m_CreationTime;
}

void Script::switchToThisVM()
{
    ScriptAPI::SetCurrentThreadVM(vm_.GetVM());
}

Sqrat::SqratVM& Script::getVM()
{
    return vm_;
}

bool Script::load(const std::string& fileName)
{
    using namespace Sqrat;
    if (!IuCoreUtils::FileExists(fileName))
    {
        LOG(ERROR) << "Script file doesn't exist: " << fileName;
        return false;
    }

    using namespace ScriptAPI;
    try
    {
        InitScriptEngine();
        ScriptAPI::RegisterAPI(vm_);

        std::string scriptText;
        if (!IuCoreUtils::ReadUtf8TextFile(fileName, scriptText)) {
            LOG(ERROR) << "Failed to read script from file " << fileName;
            return false;
        }

        preLoad();

        switchToThisVM();
        m_SquirrelScript = std::make_unique<Sqrat::Script>(vm_.GetVM());
        m_SquirrelScript->CompileString(scriptText, IuCoreUtils::ExtractFileName(fileName));

        m_SquirrelScript->Run();
        RegisterShortTranslateFunctions(vm_);
        postLoad();
        m_bIsPluginLoaded = true;
    }
    catch (std::exception& e)
    {
        LOG(ERROR)<< "CScriptUploadEngine::Load failed" << std::endl
            << "File: " << IuCoreUtils::ExtractFileName(fileName) << std::endl
            << std::string("Error: ") <<  e.what();
        FlushSquirrelOutput();
        return false;
    }
    FlushSquirrelOutput();
    return true;
}

void Script::PrintCallback(const std::string& output)
{
    const std::thread::id threadId = std::this_thread::get_id();
    LOG(WARNING) << IuCoreUtils::ExtractFileName(fileName_) << " [ThreadId=" << IuCoreUtils::ThreadIdToString(threadId) << "]\r\n" << output;
}

void Script::checkCallingThread() const
{
    const std::thread::id threadId = std::this_thread::get_id();
    if (threadId != owningThread_)
    {
        throw std::runtime_error("Script methods should be called only in the owning thread.");
    }
}

void Script::setCurrentTopLevelFileName(const std::string& fileName) {
    topLevelFileName_ = fileName;
    ScriptAPI::SetCurrentTopLevelFileName(vm_, fileName);
}
