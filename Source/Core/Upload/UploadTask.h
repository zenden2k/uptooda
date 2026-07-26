#ifndef CORE_UPLOAD_UPLOADTASK_H
#define CORE_UPLOAD_UPLOADTASK_H

#include <string>
#include <mutex>
#include <deque>
#include <functional>

#include <boost/signals2.hpp>

#include "Core/Utils/CoreTypes.h"
#include "UploadResult.h"
#include "Core/Utils/EnumUtils.h"
#include "ServerProfile.h"
#include "Core/TempFileDeleter.h"
#include "CommonTypes.h"

class CAbstractUploadEngine;
class UploadTask;
class UploadSession;
class CFileQueueUploader;
struct UploadProgressTimeInfo
{
    int64_t ms; //time
    int64_t bytes;
};
class UploadProgress {
public:
    std::string statusText;
    int stage;
    StatusType statusType;
    int64_t uploaded;
    int64_t totalUpload;
    uint64_t lastUpdateTime;
    bool isUploading;
    std::string speed;
    std::deque<UploadProgressTimeInfo> timeBytes;
    UploadProgress()
    {
        stage = 0;
        uploaded = 0;
        totalUpload = 0;
        lastUpdateTime = 0;
        isUploading = false;
        statusType = stNone;
    }
};

class UploadTaskAcceptor
{
public:
    virtual bool canAcceptUploadTask(UploadTask* task) = 0;
    virtual ~UploadTaskAcceptor() = default;
};

class UploadTask {
    public:
        UploadTask();
        explicit UploadTask(UploadTask* parentTask);
        virtual ~UploadTask();
        void setUploadManager(CFileQueueUploader* uploadManager);

        DEFINE_MEMBER_ENUM_WITH_STRING_CONVERSIONS(Role, (DefaultRole)(ThumbRole)(UrlShorteningRole));
        DEFINE_MEMBER_ENUM_WITH_STRING_CONVERSIONS(Type, (TypeFile)(TypeUrl)(TypeAuth)(TypeTest)(TypeFolder)(TypeSearchByImageUrl)(TypeSearchByImageFile));
        //enum Role { DefaultRole, ThumbRole, UrlShorteningRole };
        enum Status { StatusInQueue, StatusRunning, StatusStopped, StatusFinished, StatusFailure, StatusPostponed, StatusWaitingChildren };
        //typedef std::function<void(UploadTask*, bool)> TaskFinishedCallback;
        //typedef std::function<void(UploadTask*)> ChildTaskAddedCallback;

        virtual Type type() const = 0;
        virtual std::string getMimeType() const = 0;
        virtual int64_t getDataLength() const = 0;
        UploadTask* parentTask() const;
        std::shared_ptr<UploadTask> child(int index);
        bool isRunning();
        bool isRunningItself() const;
        void setSession(UploadSession* session);
        UploadSession* session() const;
        bool isFinished();
        bool isFinishedItself();
        virtual void finishTask(Status status = StatusFinished);
        virtual int retryLimit();
        //int getNextTask(UploadTaskAcceptor *acceptor, std::shared_ptr<UploadTask>& outTask);
        void addChildTask(std::shared_ptr<UploadTask> child);
        int childCount();
        int index() const;
        void setIndex(int index); // sort index
        UploadResult* uploadResult();
        UploadProgress* progress();

        template<typename F>
        boost::signals2::connection addTaskFinishedCallback(F&& f) {
            return onTaskFinished_.connect(std::forward<F>(f));
        }

        template<typename F>
        boost::signals2::connection addChildTaskAddedCallback(F&& f) {
            return onChildTaskAdded_.connect(std::forward<F>(f));
        }

        void setOnUploadProgressCallback(std::function<void(UploadTask*)> cb);
        void setOnStatusChangedCallback(std::function<void(UploadTask*)> cb);
        void setOnFolderUsedCallback(std::function<void(UploadTask*)> cb);

        std::string serverName() const;
        ServerProfile& serverProfile();
        void setServerProfile(ServerProfile profile);
        ServerProfile& urlShorteningServer();
        void setUrlShorteningServer(ServerProfile profile);
        void setUserData(void* data);
        void* userData() const;
        bool uploadSuccess(bool withChilds = true);
        void setUploadSuccess(bool success);
        Role role() const;
        void setRole(Role role);
        bool shorteningStarted() const;
        void setShorteningStarted(bool started);
        void stop(bool removeFromQueue = true);
        virtual std::string title() const = 0;
        bool isStopped() const;
        bool stopSignal() const;
        void clearStopFlag();
        void setStatus(Status status);
        void setStatusText(const std::string& text);
        Status status() const;
        virtual std::string toString() = 0;
        //static std::string UploaderStatusToString(StatusType status, int actionIndex, std::string param);
        TempFileDeleter* tempFileDeleter(bool create = true);
        void addTempFile(const std::string& fileName);
        void deletePostponedChilds();
        void uploadProgress(InfoProgress progress);
        void restartTask(bool fullReset = true);

        std::function<void(UploadTask*)> onFolderUsed_;
        friend class CUploader;

    protected:
        UploadTask* parentTask_;
        std::vector<std::shared_ptr<UploadTask>> childTasks_;
        
        //bool isRunning_;
        //bool isFinished_;
        //bool isStopped_;
        UploadResult uploadResult_;
        UploadProgress progress_;
        ServerProfile serverProfile_;
        ServerProfile urlShorteningProfile_;
        std::shared_ptr<CAbstractUploadEngine> currentUploadEngine_;
        void* userData_;
        void init();
        void childTaskFinished(UploadTask* child);
        void taskFinished();
        void statusChanged();
        void setCurrentUploadEngine(std::shared_ptr<CAbstractUploadEngine> currentUploadEngine);
        UploadSession* session_;
        std::recursive_mutex tasksMutex_;
        std::mutex finishMutex_;
        bool finishSignalSent_;
        Role role_;
        bool shorteningStarted_;
        volatile bool stopSignal_;
        bool uploadSuccess_;
        Status status_;
        TempFileDeleter* tempFileDeleter_;
        int index_;
        CFileQueueUploader* uploadManager_;
        std::function<void(UploadTask*)> onUploadProgress_;
        std::function<void(UploadTask*)> onStatusChanged_;
        boost::signals2::signal<void(UploadTask*, bool)> onTaskFinished_;
        boost::signals2::signal<void(UploadTask*)> onChildTaskAdded_;

};    

#endif
