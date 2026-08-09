#ifndef IU_CORE_UPLOADMANAGER_H
#define IU_CORE_UPLOADMANAGER_H

#include <boost/signals2.hpp>
#include "FileQueueUploader.h"

class CMyEngineList;
class ScriptsManager;
class BasicSettings;
class CUploadEngineList;
class UrlShorteningFilter;
class BasicSettings;

class UploadManager : public CFileQueueUploader {
public:
    UploadManager(UploadEngineManager* uploadEngineManager, ScriptsManager* scriptsManager, std::shared_ptr<IUploadErrorHandler> uploadErrorHandler,
        std::shared_ptr<INetworkClientFactory> networkClientFactory, BasicSettings* settings, int threadCount);
    ~UploadManager() override;
    bool shortenLinksInSession(std::shared_ptr<UploadSession> session, UrlShorteningFilter* filter);
protected:
    UploadEngineManager* uploadEngineManager_;
    boost::signals2::connection settingsChangedConnection_;

    void sessionAdded(UploadSession* session) override;
    void onSessionFinished(UploadSession* session);
    void onTaskFinished(UploadTask* task, bool ok);
    void taskAdded(UploadTask* task) override;
    void settingsChanged(BasicSettings* settings);
};
#endif
