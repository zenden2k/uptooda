/*

Image Uploader -  free application for uploading images/files to the Internet

Copyright 2007-2018 Sergey Svistunov (zenden2k@yandex.ru)

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
/*
#include <boost/uuid/uuid.hpp>           
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp> */

#include "EncodedPassword.h"

#ifndef _WIN32
#include "Core/Utils/CryptoUtils.h"
#endif

BasicSettings::BasicSettings()
{
    rootName_ = "ImageUploader";
    LastUpdateTime = 0;
    UploadBufferSize = 1024 * 1024;
    FileRetryLimit = 3;
    ActionRetryLimit = 2;
    ExecuteScript = false;
    loadFromRegistry_ = false;
    MaxThreads = 3;
    DeveloperMode = false;
    AutoShowLog = true;
    engineList_ = nullptr;

    ConnectionSettings.UseProxy = ConnectionSettingsStruct::kNoProxy;

    ConnectionSettings.ProxyPort = 0;
    ConnectionSettings.NeedsAuth = false;
    ConnectionSettings.ProxyType = 0;
#ifdef _WIN32
	GUID gidReference;
	HRESULT hCreateGuid = CoCreateGuid(&gidReference);
	OLECHAR* guidString = nullptr;
	StringFromCLSID(gidReference, &guidString);
	if (guidString) {
		DeviceId = W2U(guidString);
		::CoTaskMemFree(guidString);
	}
#else
	DeviceId = IuCoreUtils::CryptoUtils::CalcMD5HashFromString(std::to_string(rand()));
#endif
}

BasicSettings::~BasicSettings() {

}

void BasicSettings::setEngineList(CUploadEngineListBase* engineList) {
    engineList_ = engineList;
}

bool BasicSettings::LoadAccounts(SimpleXmlNode root)
{
    std::vector<SimpleXmlNode> servers;
    root.GetChilds("Server", servers);

    for (const auto& server: servers) {
        std::string server_name = server.Attribute("Name");
        std::vector<std::string> attribs;
        server.GetAttributes(attribs);
        ServerSettingsStruct tempSettings;

        for (size_t j = 0; j < attribs.size(); j++) {
            std::string attribName = attribs[j];

            if (attribName.empty())
                continue;
            if (attribName.substr(0, 1) == "_") {
                std::string value = server.Attribute(attribName);
                attribName = attribName.substr(1, attribName.size() - 1);
                if (!value.empty())
                    tempSettings.params[attribName] = value;
            }
        }
        tempSettings.authData.DoAuth = server.AttributeBool("Auth");

        std::string encodedLogin = server.Attribute("Login");
        CEncodedPassword login;
        login.fromEncodedData(encodedLogin);
        tempSettings.authData.Login = login;

        std::string encodedPass = server.Attribute("Password");
        CEncodedPassword pass;
        pass.fromEncodedData(encodedPass);
        tempSettings.authData.Password = pass;

        tempSettings.defaultFolder.setId(server.Attribute("DefaultFolderId"));
        tempSettings.defaultFolder.viewUrl = server.Attribute("DefaultFolderUrl");
        tempSettings.defaultFolder.setTitle(server.Attribute("DefaultFolderTitle"));

        ServersSettings[server_name][tempSettings.authData.Login] = tempSettings;
    }
    return true;
}

bool BasicSettings::SaveAccounts(SimpleXmlNode root)
{
    ServerSettingsMap::iterator it1;
    for (it1 = ServersSettings.begin(); it1 != ServersSettings.end(); ++it1) {
        std::map <std::string, ServerSettingsStruct>::iterator it;
        for (it = it1->second.begin(); it != it1->second.end(); ++it) {
            ServerSettingsStruct & sss = it->second;
            if (sss.isEmpty()) {
                continue;
            }
            SimpleXmlNode serverNode = root.CreateChild("Server");

            serverNode.SetAttribute("Name", it1->first);

            std::map <std::string, std::string>::iterator param;
            for (param = it->second.params.begin(); param != sss.params.end(); ++param) {
                if (param->first == "FolderID" || param->first == "FolderUrl" || param->first == "FolderTitle") {
                    continue;
                }
                serverNode.SetAttribute("_" + param->first, param->second);
            }
            serverNode.SetAttributeBool("Auth", sss.authData.DoAuth);

            CEncodedPassword login(sss.authData.Login);
            serverNode.SetAttribute("Login", login.toEncodedData());

            CUploadEngineData* ued = engineList_->byName(it->first);
            if (!ued || ued->NeedPassword) {
                CEncodedPassword pass(it->second.authData.Password);
                serverNode.SetAttribute("Password", pass.toEncodedData());
            }

            if (!it->second.defaultFolder.getId().empty()) {
                serverNode.SetAttributeString("DefaultFolderId", sss.defaultFolder.getId());
                serverNode.SetAttributeString("DefaultFolderUrl", sss.defaultFolder.viewUrl);
                serverNode.SetAttributeString("DefaultFolderTitle", sss.defaultFolder.getTitle());
            }

        }
    }
    return true;
}

void BasicSettings::BindToManager()
{
    SettingsNode& upload = mgr_["Uploading"];
    upload.n_bind(MaxThreads);
}

bool BasicSettings::PostLoadSettings(SimpleXml &xml)
{
    return true;
}

bool BasicSettings::PostSaveSettings(SimpleXml& xml)
{
    return true;
}

void BasicSettings::notifyChange() {
    onChange(this); // emitting signal
}

bool BasicSettings::LoadSettings(const std::string& szDir, const std::string& fileName, bool LoadFromRegistry) {
    loadFromRegistry_ = LoadFromRegistry;
    fileName_ = !szDir.empty() ? szDir + ((!fileName.empty()) ? fileName : "Settings.xml")
        : SettingsFolder + (!fileName.empty() ? fileName : "Settings.xml");
    if (!IuCoreUtils::FileExists(fileName_)) {
        return true;
    }
    SimpleXml xml;
    xml.LoadFromFile(fileName_);
    mgr_.loadFromXmlNode(xml.getRoot(rootName_).GetChild("Settings"));
    LoadAccounts(xml.getRoot(rootName_).GetChild("Settings").GetChild("ServersParams"));
    PostLoadSettings(xml);
    notifyChange();
    return true;
}

bool BasicSettings::SaveSettings()
{
    SimpleXml xml;
    mgr_.saveToXmlNode(xml.getRoot(rootName_).GetChild("Settings"));
    PostSaveSettings(xml);
    SaveAccounts(xml.getRoot(rootName_).GetChild("Settings").GetChild("ServersParams"));
    //std::cerr << "Saving setting to "<< IuCoreUtils::WstringToUtf8((LPCTSTR)fileName_);
    bool result = true;
    if (!xml.SaveToFile(fileName_)) {
        LOG(ERROR) << "Could not save settings!" << std::endl << "File: "<< fileName_;
        result = false;
    }
    notifyChange();
    return result;
}

ServerSettingsStruct* BasicSettings::getServerSettings(const ServerProfile& profile, bool create)
{
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