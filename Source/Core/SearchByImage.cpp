#include "SearchByImage.h"

#include "Core/Upload/SearchByImageFileTask.h"
#include "Core/Upload/TempImageUploadTask.h"
#include "Core/Upload/UploadSession.h"
#include "Core/Upload/UploadManager.h"
#include "Core/i18n/Translator.h"

std::shared_ptr<UploadSession> SearchByImage::search(const std::string& fileName, const ServerProfile& imageSearchServer, const ServerProfile& temporaryServer, UploadManager* uploadManager, std::function<void(const std::string&)> statusCallback)
{
    ServerProfile server = imageSearchServer;
    server.setUseDefaultSettings(false);
    server.setShortenLinks(false);
    auto& uploadParams = server.getImageUploadParamsRef();
    uploadParams.CreateThumbs = false;
    uploadParams.ProcessImages = false;

    const CUploadEngineData* ued = server.uploadEngineData();
    if (!ued->hasType(CUploadEngineData::TypeSearchByImageServer)) {
        return {};
    }
    auto session = std::make_shared<UploadSession>(false);
    std::string displayName = IuCoreUtils::ExtractFileName(fileName);
    if (ued->UploadToTempServer) { 
        auto uploadTask = std::make_shared<TempImageUploadTask>(fileName, displayName);
        uploadTask->setServerProfile(temporaryServer);
        uploadTask->setSearchServerProfile(server);
        session->addTask(uploadTask);
    } else {
        auto uploadTask = std::make_shared<SearchByImageFileTask>(fileName, displayName);
        uploadTask->setServerProfile(server);
        session->addTask(uploadTask);
    }

    return session;
}
