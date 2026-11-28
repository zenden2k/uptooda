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

#include "BasicSettings.h"

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include "Core/BasicConstants.h"
#include "Core/Utils/CryptoUtils.h"
#include "EncodedPassword.h"

BasicSettings::BasicSettings() {
    rootName_ = "ImageUploader";
    LastUpdateTime = 0;
    UploadBufferSize = 1024 * 1024;
    FileRetryLimit = MAX_RETRIES_PER_FILE;
    ActionRetryLimit = MAX_RETRIES_PER_ACTION;
    ExecuteScript = false;
    loadFromRegistry_ = false;
    MaxThreads = 3;
    DeveloperMode = false;
    AutoShowLog = true;
    engineList_ = nullptr;
    MaxUploadSpeed = 0;

    ConnectionSettings.UseProxy = ConnectionSettingsStruct::kNoProxy;

    ConnectionSettings.ProxyPort = 0;
    ConnectionSettings.NeedsAuth = false;
    ConnectionSettings.ProxyType = 0;
    boost::uuids::uuid uuid = boost::uuids::random_generator()();
    DeviceId = boost::uuids::to_string(uuid);
}

BasicSettings::~BasicSettings() = default;

void BasicSettings::setEngineList(CUploadEngineListBase* engineList) {
    engineList_ = engineList;
}

namespace {
constexpr std::string_view ENCODED_PARAM_PREFIX = "EncodedParam_";
constexpr std::string_view ENCRYPTED_VALUE_PREFIX = "ENC:";

const std::string& ServerSettingsKey() {
    static const auto key = IuCoreUtils::CryptoUtils::Base64Decode(ENV_A_E_K);
    return key;
}

std::string EncryptServerSetting(const std::string& value) {
    return std::string(ENCRYPTED_VALUE_PREFIX) + IuCoreUtils::CryptoUtils::EncryptAES(value, ServerSettingsKey());
}

std::string DecodeServerSetting(const std::string& value) {
    if (value.starts_with(ENCRYPTED_VALUE_PREFIX)) {
        return IuCoreUtils::CryptoUtils::DecryptAES(value.substr(ENCRYPTED_VALUE_PREFIX.size()), ServerSettingsKey());
    }
    std::string decoded;
    CEncodedPassword::DecodeString(value, decoded);
    return decoded;
}
}

bool BasicSettings::LoadAccounts(SimpleXmlNode root) {
    std::vector<SimpleXmlNode> servers;
    root.GetChilds("Server", servers);

    for (const auto& server : servers) {
        std::string server_name = server.Attribute("Name");
        std::vector<std::string> attribs;
        server.GetAttributes(attribs);
        ServerSettingsStruct tempSettings;

        for (const auto& attribName : attribs) {
            if (attribName.empty()) {
                continue;
            }
            if (attribName.starts_with(ENCODED_PARAM_PREFIX)) {
                tempSettings.params[attribName.substr(ENCODED_PARAM_PREFIX.size())]
                    = DecodeServerSetting(server.Attribute(attribName));
            } else if (attribName.starts_with('_')) {
                // Legacy plaintext values may also start with ENC:.
                std::string value = server.Attribute(attribName);
                if (!value.empty()) {
                    tempSettings.params.try_emplace(attribName.substr(1), std::move(value));
                }
            }
        }
        tempSettings.authData.DoAuth = server.AttributeBool("Auth");

        tempSettings.authData.Login = DecodeServerSetting(server.Attribute("Login"));
        tempSettings.authData.Password = DecodeServerSetting(server.Attribute("Password"));
        if (const auto* engine = engineList_ ? engineList_->byName(server_name) : nullptr) {
            tempSettings.migrateLegacyPassword(engine->ApiCredentialParam);
        }

        tempSettings.defaultFolder.setId(server.Attribute("DefaultFolderId"));
        tempSettings.defaultFolder.viewUrl = server.Attribute("DefaultFolderUrl");
        tempSettings.defaultFolder.setTitle(server.Attribute("DefaultFolderTitle"));

        myFromString(server.Attribute("DefaultFolderParentIds"), tempSettings.defaultFolder.parentIds);
        ServersSettings[server_name][tempSettings.authData.Login] = tempSettings;
    }
    return true;
}

bool BasicSettings::SaveAccounts(SimpleXmlNode root) {
    for (const auto& [serverName, accounts] : ServersSettings) {
        const auto* engine = engineList_ ? engineList_->byName(serverName) : nullptr;
        for (const auto& [accountName, settings] : accounts) {
            std::lock_guard lock(*settings.paramsMutex_);
            if (settings.isEmpty()) {
                continue;
            }
            SimpleXmlNode serverNode = root.CreateChild("Server");
            serverNode.SetAttribute("Name", serverName);

            for (const auto& [name, value] : settings.params) {
                if (name == "FolderID" || name == "FolderUrl" || name == "FolderTitle") {
                    continue;
                }
                serverNode.SetAttribute(std::string(ENCODED_PARAM_PREFIX) + name, EncryptServerSetting(value));
            }
            serverNode.SetAttributeBool("Auth", settings.authData.DoAuth);
            serverNode.SetAttribute("Login", EncryptServerSetting(settings.authData.Login));

            if (!engine || engine->NeedPassword) {
                serverNode.SetAttribute("Password", EncryptServerSetting(settings.authData.Password));
            }

            if (!settings.defaultFolder.getId().empty()) {
                serverNode.SetAttributeString("DefaultFolderId", settings.defaultFolder.getId());
                serverNode.SetAttributeString("DefaultFolderUrl", settings.defaultFolder.viewUrl);
                serverNode.SetAttributeString("DefaultFolderTitle", settings.defaultFolder.getTitle());
                serverNode.SetAttributeString("DefaultFolderParentIds", myToString(settings.defaultFolder.parentIds));
            }
        }
    }
    return true;
}

void BasicSettings::BindToManager() {
    SettingsNode& upload = mgr_["Uploading"];
    upload.n_bind(MaxThreads);
    upload.n_bind(MaxUploadSpeed);
}

bool BasicSettings::PostLoadSettings(SimpleXml& xml) {
    return true;
}

bool BasicSettings::PostSaveSettings(SimpleXml& xml) {
    return true;
}

void BasicSettings::notifyChange() {
    onChange(this); // emitting signal
}

bool BasicSettings::LoadSettings(const std::string& szDir, const std::string& fileName, bool LoadFromRegistry) {
    loadFromRegistry_ = LoadFromRegistry;
    fileName_ = !szDir.empty() ? szDir + ((!fileName.empty()) ? fileName : "Settings.xml")
                               : SettingsFolder + (!fileName.empty() ? fileName : "Settings.xml");
    serverAccountsLoadFailed_ = false;
    if (!IuCoreUtils::FileExists(fileName_)) {
        return true;
    }
    SimpleXml xml;
    xml.LoadFromFile(fileName_);
    mgr_.loadFromXmlNode(xml.getRoot(rootName_).GetChild("Settings"));
    try {
        LoadAccounts(xml.getRoot(rootName_).GetChild("Settings").GetChild("ServersParams"));
    }
    catch (const std::exception& e) {
        serverAccountsLoadFailed_ = true;
        LOG(ERROR) << "Could not decode server settings in " << fileName_ << ": " << e.what();
        return false;
    }
    PostLoadSettings(xml);
    notifyChange();
    return true;
}

bool BasicSettings::SaveSettings() {
    if (serverAccountsLoadFailed_) {
        LOG(ERROR) << "Could not save settings because server settings could not be decoded in " << fileName_;
        return false;
    }
    SimpleXml xml;
    mgr_.saveToXmlNode(xml.getRoot(rootName_).GetChild("Settings"));
    PostSaveSettings(xml);
    try {
        SaveAccounts(xml.getRoot(rootName_).GetChild("Settings").GetChild("ServersParams"));
    }
    catch (const std::exception& e) {
        LOG(ERROR) << "Could not encrypt server settings in " << fileName_ << ": " << e.what();
        return false;
    }

    bool result = true;
    if (!xml.SaveToFile(fileName_)) {
        LOG(ERROR) << "Could not save settings!" << std::endl << "File: " << fileName_;
        result = false;
    }
    notifyChange();
    return result;
}

ServerSettingsStruct* BasicSettings::getServerSettings(const ServerProfile& profile, bool create) {
    std::lock_guard<std::mutex> lock(serverSettingsMutex_);
    if (create) {
        return &ServersSettings[profile.serverName()][profile.profileName()];
    }
    auto it = ServersSettings.find(profile.serverName());
    if (it != ServersSettings.end()) {
        auto it2 = it->second.find(profile.profileName());
        if (it2 != it->second.end()) {
            return &it2->second;
        }
    }
    return nullptr;
}

void BasicSettings::deleteProfile(const std::string& serverName, const std::string& profileName) {
    auto it = ServersSettings.find(serverName);
    if (it == ServersSettings.end()) {
        return;
    }
    if (it->second.erase(profileName)) {
        onProfileListChanged(this, { serverName });
    }
}

void BasicSettings::clearServerSettings() {
    std::vector<std::string> deletedServerProfiles;
    {
        std::lock_guard<std::mutex> lock(serverSettingsMutex_);

        for (auto& [serverName, serverMap] : ServersSettings) {
            // const CUploadEngineData* ued = engineList_->byName(serverName);

            // Maybe we should skip profiles related to these scripts ?
            // bool isBuiltInScript = ued  && !ued->PluginName.empty() &&
            //    std::find(builtInScripts.begin(), builtInScripts.end(), ued->PluginName) != builtInScripts.end();

            for (auto it = serverMap.cbegin(); it != serverMap.cend();) {
                bool mustDelete = false;
                if (!it->first.empty()) {
                    mustDelete = true;
                }
                if (mustDelete) {
                    it = serverMap.erase(it);
                    deletedServerProfiles.push_back(serverName);
                } else {
                    ++it;
                }
            }
        }
    }
    if (!deletedServerProfiles.empty()) {
        onProfileListChanged(this, deletedServerProfiles);
    }
}
