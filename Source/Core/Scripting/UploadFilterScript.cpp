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

#include "UploadFilterScript.h"

#include "Script.h"
#include "API/ScriptAPI.h"
#include "API/UploadTaskWrappers.h"

UploadFilterScript::UploadFilterScript(const std::string& fileName, std::shared_ptr<ThreadSync> serverSync, std::shared_ptr<INetworkClientFactory> networkClientFactory)
    : Script(fileName, serverSync, networkClientFactory)
{
}

bool UploadFilterScript::preUpload(UploadTask* task)
{
    using namespace Sqrat;
    try
    {
        checkCallingThread();

        Function func(vm_.GetRootTable(), "PreUpload");
        if (func.IsNull()) {
            return true;
        }
        bool res = ScriptAPI::GetValue(func.Evaluate<bool>(ScriptAPI::UploadTaskUnion(task), 0));
        FlushSquirrelOutput();
        return res;
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "UploadFilterScript::preUpload\r\n"  << std::string(e.what());
    }
    FlushSquirrelOutput();
    return false;
}

bool UploadFilterScript::postUpload(UploadTask* task)
{
    using namespace Sqrat;
    try
    {
        checkCallingThread();

        Function func(vm_.GetRootTable(), "PostUpload");
        if (func.IsNull()) {
            return true;
        }
        bool res = ScriptAPI::GetValue(func.Evaluate<bool>(ScriptAPI::UploadTaskUnion(task), 0));
        FlushSquirrelOutput();
        return res;
    }
    catch (std::exception& e)
    {
        LOG(ERROR) << "UploadFilterScript::postUpload\r\n" << std::string(e.what());
    }
    FlushSquirrelOutput();
    return false;
}
