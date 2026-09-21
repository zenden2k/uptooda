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

#include "UploadEngineManager.h"

#include "../UploadEngineList.h"
#include "ServerProfile.h"
#include "ScriptUploadEngine.h"
#include "DefaultUploadEngine.h"
#include "Core/Logging.h"
#include "ServerSync.h"
#include "Core/Settings/BasicSettings.h"
#include "Core/Upload/UploadErrorHandler.h"
#include "Core/ServiceLocator.h"
#ifdef IU_ENABLE_MEGANZ
#include "MegaNzUploadEngine.h"
#endif

UploadEngineManager::UploadEngineManager(CUploadEngineList* uploadEngineList, std::shared_ptr<IUploadErrorHandler> uploadErrorHandler,
    std::shared_ptr<INetworkClientFactory> factory) :
        uploadEngineList_(uploadEngineList),
        uploadErrorHandler_(std::move(uploadErrorHandler)),
        networkClientFactory_(std::move(factory))
{
}

UploadEngineManager::~UploadEngineManager()
{
    unloadUploadEngines();
}

std::shared_ptr<CAbstractUploadEngine> UploadEngineManager::getUploadEngine(const ServerProfile &serverProfile)
{
    if (serverProfile.serverName().empty()) {
        LOG(ERROR) << "UploadEngineManager::getUploadEngine" << " empty server name";
        return nullptr;
    }
    const CUploadEngineData *ue = uploadEngineList_->byName(serverProfile.serverName());
    if (!ue) {
        LOG(ERROR) << "No such server " << serverProfile.serverName();
        return nullptr;
    }
    std::shared_ptr<CAbstractUploadEngine> result = nullptr;
    std::string serverName = serverProfile.serverName();
    const std::thread::id threadId = std::this_thread::get_id();

    BasicSettings* Settings = ServiceLocator::instance()->basicSettings();
    ServerSettingsStruct* serverSettings = Settings->getServerSettings(serverProfile, true);
    std::string authDataLogin = serverSettings ? serverSettings->authData.Login : std::string();
    const auto key = std::make_pair(serverName, serverProfile.profileName());

    if (ue->UsingPlugin) {
        // Try to load Squirrel (.nut) script
        result = getPlugin(serverProfile, ue->PluginName);
        if (!result) {
            LOG(ERROR) << "Cannot load plugin '" << ue->PluginName << "'";
            return nullptr;
        }
    } else {
        std::lock_guard<std::mutex> guard(pluginsMutex_);
        std::shared_ptr<CAbstractUploadEngine> plugin = nullptr;
        auto it = m_plugins.find(threadId);
        if (it != m_plugins.end()) {
            auto it2 = it->second.find(key);
            if (it2 != it->second.end()) {
                plugin = it2->second;
            }
        }

        if (plugin && plugin->serverSettings()->authData.Login == authDataLogin) {
            return plugin;
        }

        plugin.reset();
        std::shared_ptr<ServerSync> serverSync = getServerSync(serverProfile);
        CAbstractUploadEngine::ErrorMessageCallback errorCallback([uploadErrorHandler = uploadErrorHandler_.get()](auto && PH1) {
            uploadErrorHandler->ErrorMessage(std::forward<decltype(PH1)>(PH1));
        });
        if (!ue->Engine.empty()) {
#ifdef IU_ENABLE_MEGANZ
            if (ue->Engine == "MegaNz") {
                result = std::make_shared<CMegaNzUploadEngine>(serverSync, serverSettings, errorCallback);
            }
#endif
            if (!result) {
                LOG(ERROR) << "There is no built-in upload engine named '" << ue->Engine << "'.";
                return nullptr;
            }
        } else {
            result = std::make_shared<CDefaultUploadEngine>(serverSync, errorCallback);
        }
        result->setServerSettings(serverSettings);
        result->setUploadData(ue);

        m_plugins[threadId][key] = result;
    }

    result->setServerSettings(serverSettings);
    result->setUploadData(ue);
    result->setOnErrorMessageCallback([uploadErrorHandler = uploadErrorHandler_.get()](auto && PH1) {
        uploadErrorHandler->ErrorMessage(std::forward<decltype(PH1)>(PH1));
    });
    return result;
}

std::shared_ptr<CScriptUploadEngine> UploadEngineManager::getScriptUploadEngine(const ServerProfile& serverProfile)
{
    return std::dynamic_pointer_cast<CScriptUploadEngine>(getUploadEngine(serverProfile));
}

 std::shared_ptr<CScriptUploadEngine> UploadEngineManager::getPlugin(const ServerProfile& serverProfile, const std::string& pluginName, bool UseExisting) {
    std::lock_guard<std::mutex> lock(pluginsMutex_);
    std::string serverName = serverProfile.serverName();

    BasicSettings* basicSettings = ServiceLocator::instance()->basicSettings();
    ServerSettingsStruct* params = basicSettings->getServerSettings(serverProfile, true);

    const std::thread::id threadId = std::this_thread::get_id();
    std::shared_ptr<CScriptUploadEngine> plugin;
    auto key = std::make_pair(serverName, serverProfile.profileName());
    auto it = m_plugins.find(threadId);
    if (it != m_plugins.end()) {
        auto it2 = it->second.find(key);
        if (it2 != it->second.end()) {
            plugin = std::dynamic_pointer_cast<CScriptUploadEngine>(it2->second);
        }
    }

    BasicSettings* settings = ServiceLocator::instance()->basicSettings();
    if (plugin && (time(nullptr)- plugin->getCreationTime() < (settings->DeveloperMode ? 3000 : 1000 * 60 * 5)))
        UseExisting = true;

    if (plugin) {
        ServerSettingsStruct* serverSettings = plugin->serverSettings();
        if (UseExisting && plugin->name() == pluginName && serverSettings->authData.Login == params->authData.Login) {
            plugin->setOnErrorMessageCallback([capture0 = uploadErrorHandler_.get()](auto && PH1) { capture0->ErrorMessage(std::forward<decltype(PH1)>(PH1)); });
            plugin->switchToThisVM();
            return plugin;
        }
    }

    if (plugin) {
        m_plugins[threadId].erase(key);
    }
    auto serverSync = getServerSync(serverProfile);
    std::string fileName = scriptsDirectory_ + pluginName + ".nut";
    auto newPlugin = std::make_shared<CScriptUploadEngine>(fileName, serverSync, params, networkClientFactory_,
        [uploadErrorHandler = uploadErrorHandler_.get()](auto && PH1) { uploadErrorHandler->ErrorMessage(std::forward<decltype(PH1)>(PH1)); });

    if (newPlugin->isLoaded()) {
        m_plugins[threadId][key] = newPlugin;
        return newPlugin;
    }

    return nullptr;
}

void UploadEngineManager::unloadUploadEngines() {
    std::lock_guard<std::mutex> lock(pluginsMutex_);
    m_plugins.clear();
}

void UploadEngineManager::unloadUploadEngines(const std::string& serverName, const std::string& profileName) {
    std::lock_guard<std::mutex> lock(pluginsMutex_);
    for (auto &pr: m_plugins) {
        pr.second.erase({ serverName, profileName });
    }
}

void UploadEngineManager::setScriptsDirectory(const std::string & directory) {
    scriptsDirectory_ = directory;
}

void UploadEngineManager::clearThreadData()
{
    std::lock_guard<std::mutex> lock(pluginsMutex_);
    const std::thread::id threadId = std::this_thread::get_id();
    auto it = m_plugins.find(threadId);
    if (it != m_plugins.end()) {
        m_plugins.erase(it);
    }
}

void UploadEngineManager::resetAuthorization(const ServerProfile& serverProfile)
{
    auto sync = getServerSync(serverProfile);
    sync->resetAuthorization();
}

void UploadEngineManager::resetFailedAuthorization()
{
    std::lock_guard<std::mutex> lock(serverSyncsMutex_);
    for (const auto& sync : serverSyncs_) {
        sync.second->resetFailedAuthorization();
    }
}

std::shared_ptr<ServerSync> UploadEngineManager::getServerSync(const ServerProfile& serverProfile)
{
    std::lock_guard<std::mutex> lock(serverSyncsMutex_);
    ServerSyncMapKey key = std::make_pair(serverProfile.serverName(), serverProfile.profileName());
    const auto it = serverSyncs_.find(key);
    if (it == serverSyncs_.end()) {
        auto sync = std::make_shared<ServerSync>();
        serverSyncs_[key] = sync;
        return sync;
    }
    return it->second;
}
