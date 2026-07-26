#ifndef IU_CORE_SCRIPTING_SCRIPTSMANAGER_H
#define IU_CORE_SCRIPTING_SCRIPTSMANAGER_H

#pragma once
#include <thread>
#include <memory>
#include <mutex>
#include <map>

#include "Script.h"
#include "Core/Utils/CoreTypes.h"
#include "Core/Upload/ServerSync.h"

class ScriptsManager {
public:
    enum class ScriptType {TypeUploadFilterScript};
    explicit ScriptsManager(std::shared_ptr<INetworkClientFactory> networkClientFactory);
    ~ScriptsManager();
    std::shared_ptr<Script> getScript(const std::string &fileName, ScriptType type);
    void unloadScripts();
    void clearThreadData();
    std::shared_ptr<ServerSync> getServerSync(const std::string& fileName);
protected:
    std::map<std::thread::id, std::map<const std::string,std::shared_ptr<Script>>> scripts_;
    std::mutex scriptsMutex_;
    typedef std::string ServerSyncMapKey;
    std::map<ServerSyncMapKey, std::shared_ptr<ServerSync>> serverSyncs_;
    std::mutex serverSyncsMutex_;
    std::shared_ptr<INetworkClientFactory> networkClientFactory_;
private:
    DISALLOW_COPY_AND_ASSIGN(ScriptsManager);
};


#endif