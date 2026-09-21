#ifndef IU_CORE_UPLOAD_FILEQUEUEUPLOADERPRIVATE_H
#define IU_CORE_UPLOAD_FILEQUEUEUPLOADERPRIVATE_H

#pragma once
#include <mutex>
#include <condition_variable>

#include "UploadTask.h"
#include "FileQueueUploader.h"

#include "Core/Scripting/ScriptsManager.h"
#include "Core/Upload/UploadErrorHandler.h"

class CUploader;
class CAbstractUploadEngine;
class CFileQueueUploader;
class BasicSettings;

struct ServerThreadsInfo {
    //int maxThreads;
    int runningThreads;
    int waitingFileCount;
    const CUploadEngineData* ued;
    //bool fatalError;
    ServerThreadsInfo()
    {
        runningThreads = 0;
        waitingFileCount = 0;
        ued = nullptr;
    }
};

class TaskAcceptorBase : public UploadTaskAcceptor
{
public:
    explicit TaskAcceptorBase(bool useMutex = true);
    bool canAcceptUploadTask(UploadTask* task) override;
    std::map<std::string, ServerThreadsInfo> serverThreads_;
    std::recursive_mutex serverThreadsMutex_;
    bool useMutex_;

};
class FileQueueUploaderPrivate : public  TaskAcceptorBase {
public:
    FileQueueUploaderPrivate(CFileQueueUploader* queueUploader, UploadEngineManager* uploadEngineManager, ScriptsManager* scriptsManager,
        std::shared_ptr<IUploadErrorHandler> uploadErrorHandler, std::shared_ptr<INetworkClientFactory> networkClientFactory, BasicSettings* settings, int maxThreads
        );
    virtual ~FileQueueUploaderPrivate();
    
    virtual void run();
    std::shared_ptr<UploadTask> getNextJob();
    void AddSingleTask(std::shared_ptr<UploadTask> task);
    void AddSession(std::shared_ptr<UploadSession> uploadSession);
    void AddTaskToQueue(std::shared_ptr<UploadTask> task);
    void removeSession(std::shared_ptr<UploadSession> uploadSession);
    void insertTaskAfter(UploadTask* after, std::shared_ptr<UploadTask> task);
    bool removeTaskFromQueue(UploadTask* task); 
    void addUploadFilter(UploadFilter* filter);
    void removeUploadFilter(UploadFilter* filter);
    void retrySession(std::shared_ptr<UploadSession> uploadSession);
    void setMaxThreadCount(int threadCount);
    void stopSession(UploadSession* uploadSession);
    int sessionCount();
    std::shared_ptr<UploadSession> session(int index);
    CFileQueueUploader *queueUploader_;
    volatile bool stopSignal_;
    bool isRunning_;
    std::vector<std::shared_ptr<UploadSession>> sessions_;
    std::recursive_mutex sessionsMutex_;
    std::vector<UploadFilter*> filters_;
    std::atomic_int runningThreadsCount_;
    int threadCount_;
    std::vector<std::thread> threads_;
    //int runningThreads_;
    friend class CFileQueueUploader;
protected:
    void start();
    bool onNeedStopHandler();
    void OnConfigureNetworkClient(CUploader*, INetworkClient* nm);
    void onErrorMessage(CUploader*, ErrorInfo);
    void onDebugMessage(CUploader*, const std::string& msg, bool isResponseBody);
    void onTaskAdded(UploadSession*, UploadTask*);
    void addSessionToQueue(std::shared_ptr<UploadSession> uploadSession);
    void startThreads(int count);
    std::recursive_mutex mutex_;
    std::recursive_mutex callMutex_;
    std::deque<std::shared_ptr<UploadTask>> queue_;
    std::mutex queueMutex_;
    std::condition_variable queueCondition_;
    void taskAdded(UploadTask* task);
    void decrementThreadCount(const std::string& serverName);
    
    int startFromSession_;
    UploadEngineManager* uploadEngineManager_;
    ScriptsManager* scriptsManager_; 
    std::shared_ptr<IUploadErrorHandler> uploadErrorHandler_;
    std::shared_ptr<INetworkClientFactory> networkClientFactory_;
    BasicSettings* settings_;
    std::function<void(CFileQueueUploader*)> onQueueFinishedCallback_;
    std::function<void(UploadSession*)> onSessionAddedCallback_;
    std::function<void(UploadTask*)> onTaskAddedCallback_;
    std::function<void(CFileQueueUploader*, INetworkClient*)> onConfigureNetworkClientCallback_;
};

#endif
