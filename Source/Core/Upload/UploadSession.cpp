#include "UploadSession.h"

#include <algorithm>
#include "UploadTask.h"
#include "Core/Upload/UploadManager.h"

UploadSession::UploadSession(bool enableHistory) : enableHistory_(enableHistory), service_(false) {
    finishedSignalSent_ = false;
    stopSignal_ = true;
    finishedCount_ = 0;
    isStopped_ = false;
    userData_ = nullptr;
}

UploadSession::~UploadSession() {
    
}

void UploadSession::addTask(std::shared_ptr<UploadTask> task)
{
    //tasksMutex_.lock();
    task->setSession(this);
    tasks_.push_back(task);
    //tasksMutex_.unlock();
    notifyTaskAdded(task.get());
}

void UploadSession::removeTask(std::shared_ptr<UploadTask> task)
{
    //std::lock_guard<std::recursive_mutex> lock(tasksMutex_);
    auto it = std::find(tasks_.begin(), tasks_.end(), task);
    if (it != tasks_.end() )
        tasks_.erase(it);
}

bool UploadSession::moveTask(int fromIndex, int toIndex)
{
    if (!isFinished() || fromIndex < 0 || fromIndex >= tasks_.size() || toIndex < 0 || toIndex >= tasks_.size()) {
        return false;
    }
    if (fromIndex != toIndex) {
        auto task = tasks_[fromIndex];
        tasks_.erase(tasks_.begin() + fromIndex);
        tasks_.insert(tasks_.begin() + toIndex, std::move(task));
    }
    for (int i = 0; i < tasks_.size(); ++i) {
        tasks_[i]->setIndex(i);
    }
    return true;
}

bool UploadSession::isRunning()
{
    //std::lock_guard<std::recursive_mutex> lock(tasksMutex_);
    /*for (auto it = tasks_.begin(); it != tasks_.end(); ++it)
    {
        if (!it->get()->isFinished())
        {
            return true;
        }
    }
    return false;*/
    return !isFinished();
}

bool UploadSession::isFinished()
{
    return finishedCount_ == tasks_.size();

    try {
//        std::lock_guard<std::recursive_mutex> lock(tasksMutex_);
        //return tasks_.size() == finishedCount_;
        if (tasks_.empty())
        {
            return false;
        }
        for (size_t i = 0; i < tasks_.size(); i++)
        {
            auto it = tasks_[i];
            //tasksMutex_.unlock(); // FIXME!!! How to avoid deadlocks???
            if (!it->isFinished())
            {
                //tasksMutex_.lock();
                return false;
            }
            //tasksMutex_.lock();
        }
        return true;
    }
    catch (std::exception& ex)
    {
        LOG(ERROR) << ex.what();
    }
    return false;
    //return isFinished_;
}

/*int UploadSession::pendingTasksCount(UploadTaskAcceptor* acceptor)
{
    //std::lock_guard<std::recursive_mutex> lock(tasksMutex_);
    int res = 0;
    for (auto it = tasks_.begin(); it != tasks_.end(); it++)
    {
        UploadTask* uploadTask = it->get();
        if (uploadTask->status() == UploadTask::StatusInQueue && acceptor->canAcceptUploadTask(uploadTask)) {
            res++;
        }
        res += it->get()->pendingTasksCount(acceptor);
    }
    return res;
}*/

int UploadSession::taskCount()
{
    /*try {
        std::lock_guard<std::recursive_mutex> lock(tasksMutex_);
        return tasks_.size();
    } catch (std::exception& ex) {
        LOG(ERROR) << ex.what();
    }*/
    return tasks_.size();
}

int UploadSession::finishedTaskCount(UploadTask::Status status)
{
    int res = 0;
    try {
        //std::lock_guard<std::recursive_mutex> lock(tasksMutex_);
    
        for (auto it = tasks_.begin(); it != tasks_.end(); ++it)
        {
            if (it->get()->isFinished() && it->get()->status() == status)
            {
                res++;
            }
        }
    }
    catch (std::exception& ex) {
        LOG(ERROR) << ex.what();
    }
    return res;
}

bool UploadSession::isStopped() const
{
    return isStopped_;
}

void UploadSession::addSessionFinishedCallback(const SessionFinishedCallback& callback)
{
    sessionFinishedCallbacks_.push_back(callback);
}

void UploadSession::addTaskAddedCallback(const TaskAddedCallback& callback)
{
    taskAddedCallbacks_.push_back(callback);
}

void UploadSession::stop(bool removeFromQueue)
{
    stopSignal_ = true;
    //std::lock_guard<std::recursive_mutex> lock(tasksMutex_);
    for (const auto& it: tasks_) {
        it->stop(removeFromQueue);
    }
}

void UploadSession::clearStopFlag() {
    stopSignal_ = false;
    for (auto it = tasks_.begin(); it != tasks_.end(); ++it) {
        it->get()->clearStopFlag();

    }
}

void UploadSession::restartFailedTasks(CFileQueueUploader* uploadManager) {
    if (isRunning()) {
        return;
    }
    finishedSignalSent_ = false;
    int finishedCount = 0;
    for (auto it = tasks_.begin(); it != tasks_.end(); ++it) {
        auto task = *it;
        auto status = task->status();

        if (status == UploadTask::StatusFailure || status == UploadTask::StatusStopped) {
            task->restartTask();
            //uploadManager->addTaskToQueue(task);
        } else {
            finishedCount++;
        }
    }
    finishedCount_ = finishedCount; 
}

bool UploadSession::isFatalErrorSet(const std::string& serverName, const std::string& profileName)
{
    std::lock_guard<std::mutex> lock(serverFatalErrorsMutex_);
    auto it = serverFatalErrors_.find(std::make_pair(serverName, profileName));
    if (it != serverFatalErrors_.end())
    {
        return it->second;
    }
    return false;
}

void UploadSession::setFatalErrorForServer(const std::string& serverName, const std::string& profileName)
{
    {
        std::lock_guard<std::mutex> lock(serverFatalErrorsMutex_);
        serverFatalErrors_[std::make_pair(serverName, profileName)] = true;
    }

    for (auto it = tasks_.begin(); it != tasks_.end(); ++it) {
        auto task = it->get();
        if (task->status() == UploadTask::StatusInQueue && (task->serverName() == serverName && task->serverProfile().profileName() == profileName)) {
            task->setStatus(UploadTask::StatusStopped); 
        }
    }
}

void UploadSession::clearErrorsForServer(const std::string& serverName, const std::string& profileName)
{
    std::lock_guard<std::mutex> lock(serverFatalErrorsMutex_);
    auto it = serverFatalErrors_.find(std::make_pair(serverName, profileName));
    if (it != serverFatalErrors_.end())
    {
        serverFatalErrors_.erase(it);
    }
}

std::shared_ptr<UploadTask> UploadSession::getTask(int index)
{
    //std::lock_guard<std::recursive_mutex> lock(tasksMutex_);
    return tasks_[index];
}

void UploadSession::taskFinished(UploadTask* task)
{
    if (task->parentTask() != nullptr) {
        return; 
    }
    ++finishedCount_;
    std::lock_guard<std::mutex> lock(finishMutex_);
    if (!finishedSignalSent_ && finishedCount_ == tasks_.size()) {
        // TODO: use std::call_once
        for (const auto& it : sessionFinishedCallbacks_) {
            it(this);
        }
        finishedSignalSent_ = true;
    }
}

void UploadSession::childTaskAdded(UploadTask* task)
{
    finishedSignalSent_ = false;
    notifyTaskAdded(task);
}

bool UploadSession::stopSignal()
{
    return stopSignal_;
}

void UploadSession::notifyTaskAdded(UploadTask* task)
{
    for (size_t i = 0; i < taskAddedCallbacks_.size(); i++)
    {
        taskAddedCallbacks_[i](this, task);
    }
}

void UploadSession::setUserData(void* data) {
	userData_ = data;
}
void* UploadSession::userData() const { return userData_; }

bool UploadSession::isHistoryEnabled() const { return enableHistory_; }

void UploadSession::setService(bool service) { service_ = service; }

bool UploadSession::isService() const { return service_; }

void UploadSession::recalcFinishedCount() {
    int res = 0;
    try {
        //        std::lock_guard<std::recursive_mutex> lock(tasksMutex_);
        if (tasks_.empty()) {
            finishedCount_ = 0;
            return;
        }
        for (size_t i = 0; i < tasks_.size(); i++) {
            auto it = tasks_[i];
            //tasksMutex_.unlock(); // FIXME!!! How to avoid deadlocks???
            if (it->isFinished()) {
                res++;
            }
            //tasksMutex_.lock();
        }
    }
    catch (std::exception& ex) {
        LOG(ERROR) << ex.what();
    }
    finishedCount_ = res;
}

std::vector<std::shared_ptr<UploadTask>>::const_iterator UploadSession::begin() {
    return tasks_.begin();
}

std::vector<std::shared_ptr<UploadTask>>::const_iterator UploadSession::end() {
    return tasks_.end();
}
