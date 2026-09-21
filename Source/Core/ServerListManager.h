#ifndef IU_CORE_SERVERLISTMANAGER_H
#define IU_CORE_SERVERLISTMANAGER_H

#include <string>

#include "Core/Utils/CoreTypes.h"
#include "Core/Upload/UploadEngine.h"

class CUploadEngineList;

class ServerListManager
{
public:
    enum class ServerType { stFTP = 0, stSFTP = 1, stWebDAV = 2 };

    ServerListManager(std::string serversDirectory, CUploadEngineList* uel, ServerSettingsMap& serversSettings);
    ~ServerListManager()=default;

    /**
    * @throws std::runtime_error
    */
    std::string addFtpServer(ServerType serverType, bool temporary, const std::string &name, const std::string &serverName, const std::string &login, const std::string &password,
        const std::string& remoteDirectory, const std::string& downloadUrl, const std::string& privateKeyFile, int securedConnection, const std::string& activeConnectionPort);

    /**
    * @throws std::runtime_error
    */
    std::string addDirectoryAsServer(const std::string &name, const std::string &directory, const std::string &downloadUrl, bool convertUncPath);
protected:
    DISALLOW_COPY_AND_ASSIGN(ServerListManager);
    CUploadEngineList * uploadEngineList_;
    std::string serversDirectory_;
    ServerSettingsMap& serversSettings_;
};

#endif
