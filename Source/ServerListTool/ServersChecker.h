#ifndef IU_SERVERLISTTOOL_SERVERSCHECKER_H
#define IU_SERVERLISTTOOL_SERVERSCHECKER_H

#include <atomic>
#include <memory>
#include <vector>
#include <chrono>

#include "Core/Upload/UploadSession.h"
#include "Core/FileDownloader.h"
#include "Core/Network/INetworkClient.h"
#include "Helpers.h"

class CFileDownloader;

namespace ServersListTool {

class ServersCheckerModel;
class CheckShortUrlTask;

struct UploadTaskUserData {
    size_t rowIndex = 0;
    std::chrono::steady_clock::time_point startTime;
};

class ServersChecker {
public:
    ServersChecker(ServersCheckerModel* model, UploadManager* uploadManager, std::shared_ptr<INetworkClientFactory> networkClientFactory);
    bool start(const std::string& testFileName, const std::string& testUrl = "");
    void stop();
    bool isRunning() const;
    void setUseAccounts(bool value);
    void setOnlyAccs(bool value);
    void setCheckImageServers(bool value);
    void setCheckFileServers(bool value);
    void setCheckUrlShorteners(bool value);
    void setOnFinishedCallback(std::function<void()> callback);

protected:
    void onTaskStatusChanged(UploadTask* task);
    void onTaskFinished(UploadTask* task, bool ok);
    void onSessionFinished(UploadSession* session);
    void onFileFinished(bool ok, int statusCode, const CFileDownloader::DownloadFileListItem& it);
    void checkShortUrl(UploadTask* task);
    void markServer(size_t id);
    void processFinished();

    std::shared_ptr<UploadSession> uploadSession_;
    std::unique_ptr<CFileDownloader> fileDownloader_;
    // isRunning_ - is uploading process running
    // but there is also CFileDownloader.
    std::atomic_bool needStop_, isRunning_; 
    ServersCheckerModel* model_;
    UploadManager* uploadManager_;
    bool useAccounts_, onlyAccs_, checkImageServers_, checkFileServers_, checkURLShorteners_;
    std::vector<std::unique_ptr<UploadTaskUserData>> uploadTaskUserDatas_;
    std::function<void()> onFinishedCallback_;
    Helpers::MyFileInfo m_sourceFileInfo;
    std::string srcFileHash_;
    std::shared_ptr<INetworkClientFactory> networkClientFactory_;
    std::atomic<int> linksToCheck_;
    std::vector<std::shared_ptr<CheckShortUrlTask>> scheduledTasks_;
    std::mutex scheduledTasksMutex_;
};
}
#endif
