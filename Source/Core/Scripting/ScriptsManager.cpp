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

#include "ScriptsManager.h"

#include "UploadFilterScript.h"
#include "Core/Settings/BasicSettings.h"
#include "Core/ServiceLocator.h"

ScriptsManager::ScriptsManager(std::shared_ptr<INetworkClientFactory> networkClientFactory) :
    networkClientFactory_(std::move(networkClientFactory))
{
}

ScriptsManager::~ScriptsManager()
{
    unloadScripts();
}

std::shared_ptr<Script> ScriptsManager::getScript(const std::string& fileName, ScriptType type) {
    std::lock_guard<std::mutex> lock(scriptsMutex_);
    bool useExisting = false;
    const std::thread::id threadId = std::this_thread::get_id();
    std::shared_ptr<Script> plugin = nullptr;
    auto it = scripts_.find(threadId);
    if (it != scripts_.end()) {
        auto it2 = it->second.find(fileName);
        if (it2 != it->second.end()) {
            plugin = it2->second;
        }
    }
    const auto settings = ServiceLocator::instance()->basicSettings();
    if (plugin && (time(nullptr) - plugin->getCreationTime() < (settings->DeveloperMode ? 0 : 1000 * 60 * 5))) {
        useExisting = true;
    }

    if (plugin && useExisting) {
        plugin->switchToThisVM();
        return plugin;
    }

    if (plugin) {
        plugin.reset();
        scripts_.erase(threadId);
    }
    std::shared_ptr<ServerSync> serverSync = getServerSync(fileName);
    std::shared_ptr<Script> newPlugin;
    if (type == ScriptType::TypeUploadFilterScript) {
        newPlugin = std::make_shared<UploadFilterScript>(fileName, serverSync, networkClientFactory_);
    } else {
        newPlugin = std::make_shared<Script>(fileName, serverSync, networkClientFactory_);
    }

    if (newPlugin->isLoaded()) {
        scripts_[threadId][fileName] = newPlugin;
        return newPlugin;
    }

    return nullptr;
}

void ScriptsManager::unloadScripts()
{
    std::lock_guard<std::mutex> lock(scriptsMutex_);
    scripts_.clear();
}

void ScriptsManager::clearThreadData()
{
    std::lock_guard<std::mutex> lock(scriptsMutex_);
    const std::thread::id threadId = std::this_thread::get_id();
    auto it = scripts_.find(threadId);
    if (it != scripts_.end()) {
        scripts_.erase(it);
    }
}

std::shared_ptr<ServerSync> ScriptsManager::getServerSync(const std::string& fileName)
{
    std::lock_guard<std::mutex> lock(serverSyncsMutex_);
    auto it = serverSyncs_.find(fileName);
    if (it == serverSyncs_.end())
    {
        auto sync = std::make_shared<ServerSync>();
        serverSyncs_[fileName] = sync;
        return sync;
    }
    return it->second;
}
