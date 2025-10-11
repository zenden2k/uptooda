#ifndef IU_CORE_UPLOADENGINE_MANAGER_H
#define IU_CORE_UPLOADENGINE_MANAGER_H

#pragma once
#include <thread>
#include <mutex>
#include <memory>
#include <unordered_map>

#include "Core/Utils/CoreUtils.h"
#include "UploadEngine.h"

// Forward class declarations
class IUploadErrorHandler;
class CScriptUploadEngine;
class CUploadEngineList;
class ServerProfile;

/** UploadEngineManager class manages upload engines (instances of classes derivated from CAbstractUploadEngine).
    and their lifetime
**/
class UploadEngineManager
{
public:
    UploadEngineManager(CUploadEngineList* uploadEngineList, std::shared_ptr<IUploadErrorHandler> uploadErrorHandler, std::shared_ptr<INetworkClientFactory> factory);
    ~UploadEngineManager();

    /**
    Load and create upload engine. Object is owned by UploadEngineManager.
    CScriptUploadEngine functions can be called only in the thread it has been created.
    **/
    CAbstractUploadEngine* getUploadEngine(ServerProfile &serverProfile);

    /*
    Same as getUploadEngine(), but result is casted to CScriptUploadEngine
    */
    CScriptUploadEngine* getScriptUploadEngine(ServerProfile &serverProfile);
    
    /** 
    Force unload all cached upload engines
    */
    void unloadUploadEngines();

    void unloadUploadEngines(const std::string& serverName, const std::string& profileName);

    void setScriptsDirectory(const std::string & directory);

    /** 
    This function destroys all upload engines owned by current thread and all associated Scripting API data.
    */
    void clearThreadData();

    /**
    Reset authorization on this server (failed and succeeded).
    It is called when user starts new upload
    */
    void resetAuthorization(const ServerProfile& serverProfile);

    /**
    Reset failed authorization on ALL servers
    */
    void resetFailedAuthorization();
protected:
    std::shared_ptr<CScriptUploadEngine> getPlugin(ServerProfile& serverProfile, const std::string& pluginName, bool UseExisting = false);
    ServerSync* getServerSync(const ServerProfile& serverProfile);
    std::map<std::thread::id, std::unordered_map<std::pair<std::string, std::string>, std::shared_ptr<CAbstractUploadEngine>>> m_plugins;
    std::mutex pluginsMutex_;
    std::string scriptsDirectory_;
    CUploadEngineList* uploadEngineList_;
    typedef std::pair<std::string, std::string> ServerSyncMapKey;
    std::map<ServerSyncMapKey, ServerSync*> serverSyncs_;
    std::mutex serverSyncsMutex_;
    std::shared_ptr<IUploadErrorHandler> uploadErrorHandler_;
    std::shared_ptr<INetworkClientFactory> networkClientFactory_;
};

#endif
