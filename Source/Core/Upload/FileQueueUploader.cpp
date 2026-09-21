/*

    Uptooda - free application for uploading images/files to the Internet

    Copyright 2007-2025 Sergey Svistunov (zenden2k@gmail.com)

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

 */


#include "FileQueueUploader.h"

#include <utility>

#include "Core/Upload/UploadTask.h"
#include "FileQueueUploaderPrivate.h"

CFileQueueUploader::CFileQueueUploader(UploadEngineManager* uploadEngineManager,
    ScriptsManager* scriptsManager, std::shared_ptr<IUploadErrorHandler> uploadErrorHandler,
    std::shared_ptr<INetworkClientFactory> networkClientFactory, BasicSettings* settings, int maxThreads)
{
    _impl = new FileQueueUploaderPrivate(this, uploadEngineManager, scriptsManager, std::move(uploadErrorHandler), std::move(networkClientFactory), settings, maxThreads);
}

void CFileQueueUploader::addSession(std::shared_ptr<UploadSession> uploadSession)
{
    _impl->AddSession(std::move(uploadSession));
}

bool CFileQueueUploader::start()
{
    _impl->start();
    return true;
}

void CFileQueueUploader::stop()
{
    _impl->stopSignal_ = true;
}

bool CFileQueueUploader::IsRunning() const {
    return _impl->isRunning_;
}

CFileQueueUploader::~CFileQueueUploader() {
    delete _impl;
}

void CFileQueueUploader::setMaxThreadCount(int threadCount) {
    _impl->setMaxThreadCount(threadCount);
}

bool CFileQueueUploader::isSlotAvailableForServer(const std::string& serverName, int maxThreads) {
    int threads = _impl->serverThreads_[serverName].runningThreads + _impl->serverThreads_[serverName].waitingFileCount;
    return threads < maxThreads && threads < _impl->threadCount_;
}

void CFileQueueUploader::addUploadFilter(UploadFilter* filter)
{
    _impl->addUploadFilter(filter);
}

void CFileQueueUploader::removeUploadFilter(UploadFilter* filter)
{
    _impl->removeUploadFilter(filter);
}

int CFileQueueUploader::sessionCount()
{
    return _impl->sessionCount();
}

std::shared_ptr<UploadSession> CFileQueueUploader::session(int index)
{
    return _impl->session(index);
}

void CFileQueueUploader::setOnQueueFinishedCallback(std::function<void(CFileQueueUploader*)> cb) {
    _impl->onQueueFinishedCallback_ = std::move(cb);
}

void CFileQueueUploader::setOnSessionAddedCallback(std::function<void(UploadSession*)> cb) {
    _impl->onSessionAddedCallback_ = std::move(cb);
}

void CFileQueueUploader::setOnTaskAddedCallback(std::function<void(UploadTask*)> cb) {
    _impl->onTaskAddedCallback_ = std::move(cb);
}

void CFileQueueUploader::setOnConfigureNetworkClient(std::function<void(CFileQueueUploader*, INetworkClient*)> cb) {
    _impl->onConfigureNetworkClientCallback_ = std::move(cb);
}

void CFileQueueUploader::sessionAdded(UploadSession* session)
{
    if (_impl->onSessionAddedCallback_)
    {
        _impl->onSessionAddedCallback_(session);
    }
}

void CFileQueueUploader::taskAdded(UploadTask* task)
{
    if (_impl->onTaskAddedCallback_)
    {
        _impl->onTaskAddedCallback_(task);
    }
}

void CFileQueueUploader::addSingleTask(std::shared_ptr<UploadTask> uploadTask) {
    _impl->AddSingleTask(uploadTask);
}

void CFileQueueUploader::addTaskToQueue(std::shared_ptr<UploadTask> task) {
    _impl->AddTaskToQueue(task);
}

void CFileQueueUploader::insertTaskAfter(UploadTask* after, std::shared_ptr<UploadTask> task) {
    _impl->insertTaskAfter(after, task);
}

bool CFileQueueUploader::removeTaskFromQueue(UploadTask* task) {
    return _impl->removeTaskFromQueue(task);
}

void CFileQueueUploader::removeSession(std::shared_ptr<UploadSession> uploadSession)
{
    _impl->removeSession(uploadSession);
}

void CFileQueueUploader::retrySession(std::shared_ptr<UploadSession> uploadSession) {
    _impl->retrySession(uploadSession);
}

void CFileQueueUploader::stopSession(UploadSession* uploadSession) {
    _impl->stopSession(uploadSession);
}
