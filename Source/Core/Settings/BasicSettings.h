#ifndef IU_CORE_SETTINGS_BASICSETTINGS_H
#define IU_CORE_SETTINGS_BASICSETTINGS_H

#pragma once
#include <boost/signals2.hpp>

#include "Core/SettingsManager.h"
#include "Core/Upload/UploadEngine.h"
#include "EncodedPassword.h"

typedef std::map <std::string, std::map <std::string, ServerSettingsStruct>> ServerSettingsMap;

struct ConnectionSettingsStruct {
    enum UseProxyEnum { kNoProxy = 0, kUserProxy, kSystemProxy };
    int UseProxy;
    std::string ServerAddress;
    int ProxyPort;
    bool NeedsAuth;
    std::string ProxyUser;
    CEncodedPassword ProxyPassword;
    int ProxyType;
};

class BasicSettings {
public:
    BasicSettings();
    virtual ~BasicSettings();
    void setEngineList(CUploadEngineListBase* engineList);
    bool LoadSettings(const std::string& szDir = "", const std::string& fileName = "", bool LoadFromRegistry = true);
    bool SaveSettings();
    void notifyChange();
    boost::signals2::signal<void(BasicSettings*)> onChange;
    // The second argument is a vector of affected servers
    boost::signals2::signal<void(BasicSettings*, const std::vector<std::string>&)> onProfileListChanged;
    unsigned int LastUpdateTime;
    int FileRetryLimit;
    int ActionRetryLimit;
    std::mutex serverSettingsMutex_;
    ServerSettingsMap ServersSettings;
    std::string ScriptFileName;
    bool ExecuteScript;
    bool DeveloperMode;

    int MaxThreads;
    bool AutoShowLog;

    unsigned int MaxUploadSpeed; // measured in KB/s
    std::string SettingsFolder;

    ConnectionSettingsStruct ConnectionSettings;
    std::string DeviceId;

    ServerSettingsStruct* getServerSettings(const ServerProfile& profile, bool create = false);

    void deleteProfile(const std::string& serverName, const std::string& profileName);

    // Delete account data (profiles)
    void clearServerSettings();

protected:
    SettingsManager mgr_;
    std::string fileName_;
    std::string rootName_;
  
    CUploadEngineListBase* engineList_;
    bool loadFromRegistry_;
    bool LoadAccounts(SimpleXmlNode root);
    bool SaveAccounts(SimpleXmlNode root);
    void BindToManager();
    virtual bool PostLoadSettings(SimpleXml &xml);
    virtual bool PostSaveSettings(SimpleXml &xml);
};
#endif
