#include "UploadTaskWrappers.h"

#include <boost/current_function.hpp>
#include <memory>
#include <utility>

#include "Core/Scripting/Squirrelnc.h"
#include "Core/Upload/FileUploadTask.h"
#include "Core/Upload/UrlShorteningTask.h"

namespace ScriptAPI {;

UploadTaskWrapper::UploadTaskWrapper()
{
}

std::string UploadTaskWrapper::role()
{
    checkNull(BOOST_CURRENT_FUNCTION);
    return UploadTask::EnumToString(task_->role());
}

void UploadTaskWrapper::setRole(const std::string& role)
{
    checkNull(BOOST_CURRENT_FUNCTION);
    task_->setRole(UploadTask::StringToEnumRole(role.c_str()));
}

std::string UploadTaskWrapper::type()
{
    checkNull(BOOST_CURRENT_FUNCTION);
    return UploadTask::EnumToString(task_->type());
}


std::string UploadTaskWrapper::getMimeType()
{   
    checkNull(BOOST_CURRENT_FUNCTION);
    return task_->getMimeType();
}

int64_t UploadTaskWrapper::getDataLength()
{
    checkNull(BOOST_CURRENT_FUNCTION);
    return task_->getDataLength();
}

UploadTaskWrapper UploadTaskWrapper::parentTask()
{
    checkNull(BOOST_CURRENT_FUNCTION);
    return UploadTaskWrapper(task_->parentTask());
}

UploadTaskWrapper UploadTaskWrapper::child(int index)
{
    checkNull(BOOST_CURRENT_FUNCTION);
    return task_->child(index);
}

int UploadTaskWrapper::childCount()
{
    checkNull(BOOST_CURRENT_FUNCTION);
    return task_->childCount();
}

UploadResult* UploadTaskWrapper::uploadResult()
{
    checkNull(BOOST_CURRENT_FUNCTION);
    return task_->uploadResult();
}

std::string UploadTaskWrapper::serverName() const
{
    checkNull(BOOST_CURRENT_FUNCTION);
    return task_->serverName();
}

std::string UploadTaskWrapper::profileName()
{
    checkNull(BOOST_CURRENT_FUNCTION);
    return task_->serverProfile().profileName();
}

void UploadTaskWrapper::setStatusText(const std::string& status)
{
    checkNull(BOOST_CURRENT_FUNCTION);
    task_->setStatusText(status);
}

ServerProfile UploadTaskWrapper::serverProfile()
{
    checkNull(BOOST_CURRENT_FUNCTION);
    return task_->serverProfile();
}

void UploadTaskWrapper::setServerProfile(const ServerProfile& profile)
{
    checkNull(BOOST_CURRENT_FUNCTION);
    task_->setServerProfile(profile);
}

std::string UploadTaskWrapper::toString()
{
    checkNull(BOOST_CURRENT_FUNCTION);
    return task_->toString();
}

void UploadTaskWrapper::addChildTask(UploadTaskWrapper* child)
{
    checkNull(BOOST_CURRENT_FUNCTION);
    task_->addChildTask(child->task_);
}

void UploadTaskWrapper::addTempFile(const std::string& fileName)
{
    checkNull(BOOST_CURRENT_FUNCTION);
    task_->addTempFile(fileName);
}

bool UploadTaskWrapper::isNull()
{
    return !task_;
}

void UploadTaskWrapper::checkNull(const char* func) const
{
    if (!task_)
    {
        throw std::runtime_error(std::string(func) + "\r\nUploadTask is null");
    }
}


FileUploadTaskWrapper::FileUploadTaskWrapper(const std::string& fileName, const std::string& displayName)
{
    task_ = std::make_shared<FileUploadTask>(fileName, displayName);
}


FileUploadTaskWrapper::FileUploadTaskWrapper(FileUploadTask* task)
    : UploadTaskWrapper(task)
{
}


FileUploadTaskWrapper::FileUploadTaskWrapper(std::shared_ptr<FileUploadTask> task)
    : UploadTaskWrapper(task)
{
}

std::string FileUploadTaskWrapper::getFileName() const
{
    checkNull(BOOST_CURRENT_FUNCTION);
    auto* fileTask = dynamic_cast<FileUploadTask*>(task_.get());
    if (!fileTask)
    {
        LOG(ERROR) << BOOST_CURRENT_FUNCTION << std::endl << "this is not a FileUploadTask";
        return {};
    }
    return fileTask->getFileName();
}

int64_t FileUploadTaskWrapper::getFileSize() const
{
    checkNull(BOOST_CURRENT_FUNCTION);
    auto* fileTask = dynamic_cast<FileUploadTask*>(task_.get());
    if (!fileTask)
    {
        LOG(ERROR) << BOOST_CURRENT_FUNCTION << std::endl << "this is not a FileUploadTask";
        return 0;
    }
    return fileTask->getFileSize();
}

void FileUploadTaskWrapper::setFileName(const std::string& fileName)
{
    checkNull(BOOST_CURRENT_FUNCTION);
    auto* fileTask = dynamic_cast<FileUploadTask*>(task_.get());
    if (!fileTask)
    {
        LOG(ERROR) << BOOST_CURRENT_FUNCTION << std::endl << "this is not a FileUploadTask";
        return;
    }
    fileTask->setFileName(fileName);
}

std::string FileUploadTaskWrapper::getDisplayName() const
{
    checkNull(BOOST_CURRENT_FUNCTION);
    auto* fileTask = dynamic_cast<FileUploadTask*>(task_.get());
    if (!fileTask)
    {
        LOG(ERROR) << BOOST_CURRENT_FUNCTION << std::endl << "this is not a FileUploadTask";
        return {};
    }
    return fileTask->getDisplayName();
}

void FileUploadTaskWrapper::setDisplayName(const std::string& name)
{
    checkNull(BOOST_CURRENT_FUNCTION);
    auto* fileTask = dynamic_cast<FileUploadTask*>(task_.get());
    if (!fileTask)
    {
        LOG(ERROR) << BOOST_CURRENT_FUNCTION << std::endl << "this is not a FileUploadTask";
        return;
    }
    fileTask->setDisplayName(name);
}

std::string FileUploadTaskWrapper::originalFileName() const
{
    checkNull(BOOST_CURRENT_FUNCTION);
    auto* fileTask = dynamic_cast<FileUploadTask*>(task_.get());
    if (!fileTask)
    {
        LOG(ERROR) << BOOST_CURRENT_FUNCTION << std::endl << "this is not a FileUploadTask";
        return {};
    }
    return fileTask->originalFileName();
}


bool FileUploadTaskWrapper::isImage() const{
    checkNull(BOOST_CURRENT_FUNCTION);
    auto* fileTask = dynamic_cast<FileUploadTask*>(task_.get());
    if (!fileTask) {
        LOG(ERROR) << BOOST_CURRENT_FUNCTION << std::endl
                   << "this is not a FileUploadTask";
        return false;
    }
    return fileTask->isImage();
}

bool FileUploadTaskWrapper::isVideo() const {
    checkNull(BOOST_CURRENT_FUNCTION);
    auto* fileTask = dynamic_cast<FileUploadTask*>(task_.get());
    if (!fileTask) {
        LOG(ERROR) << BOOST_CURRENT_FUNCTION << std::endl
                   << "this is not a FileUploadTask";
        return false;
    }
    return fileTask->isVideo();
}

UrlShorteningTaskWrapper::UrlShorteningTaskWrapper(UrlShorteningTask* task)
    : UploadTaskWrapper(task)
{
}


UrlShorteningTaskWrapper::UrlShorteningTaskWrapper(std::shared_ptr<UrlShorteningTask> task)
    : UploadTaskWrapper(task)
{
}

std::string UrlShorteningTaskWrapper::getUrl() const
{
    checkNull(BOOST_CURRENT_FUNCTION);
    auto* urlShorteningTask = dynamic_cast<UrlShorteningTask*>(task_.get());
    if (!urlShorteningTask)
    {
        LOG(ERROR) << BOOST_CURRENT_FUNCTION << std::endl << "this is not a UrlShorteningTask";
        return {};
    }
    return urlShorteningTask->getUrl();
}

void UrlShorteningTaskWrapper::setParentUrlType(const std::string& type)
{
    checkNull(BOOST_CURRENT_FUNCTION);
    auto* urlShorteningTask = dynamic_cast<UrlShorteningTask*>(task_.get());
    if (!urlShorteningTask)
    {
        LOG(ERROR) << BOOST_CURRENT_FUNCTION << std::endl << "this is not a UrlShorteningTask";
        return;
    }
    urlShorteningTask->setParentUrlType(UrlShorteningTask::StringToEnumParentUrlType(type.c_str())); 
}

std::string UrlShorteningTaskWrapper::parentUrlType()
{
    checkNull(BOOST_CURRENT_FUNCTION);
    auto* urlShorteningTask = dynamic_cast<UrlShorteningTask*>(task_.get());
    if (!urlShorteningTask)
    {
        LOG(ERROR) << BOOST_CURRENT_FUNCTION << std::endl << "this is not a UrlShorteningTask";
        return {};
    }
    return UrlShorteningTask::EnumToString(urlShorteningTask->parentUrlType());
}

UploadTaskWrapper::UploadTaskWrapper(UploadTask* task)
{
    release_deleter<UploadTask> deleter;
    deleter.release();
    task_.reset(task, deleter);
}

UploadTaskWrapper::UploadTaskWrapper(std::shared_ptr<UploadTask> task)
{
    task_ = task;
}

void RegisterUploadTaskWrappers(Sqrat::SqratVM& vm) {
    using namespace Sqrat;
    RootTable& root = vm.GetRootTable();
    HSQUIRRELVM hvm = vm.GetVM();

    Class<ServerProfile> serverProfileClass(hvm, "ServerProfile");
    root.Bind("ServerProfile", serverProfileClass);

    root.Bind("UploadResult", Class<UploadResult>(hvm, "UploadResult").
        Func("getDirectUrl", &UploadResult::getDirectUrl).
        Func("setDirectUrl", &UploadResult::setDirectUrl).
        Func("getDirectUrlShortened", &UploadResult::getDirectUrlShortened).
        Func("setDirectUrlShortened", &UploadResult::setDirectUrlShortened).
        Func("getThumbUrl", &UploadResult::getThumbUrl).
        Func("setThumbUrl", &UploadResult::setThumbUrl).
        Func("getDownloadUrl", &UploadResult::getDownloadUrl).
        Func("setDownloadUrl", &UploadResult::setDownloadUrl).
        Func("getDownloadUrlShortened", &UploadResult::getDownloadUrlShortened).
        Func("setDownloadUrlShortened", &UploadResult::setDownloadUrlShortened).
        Func("getServerName", &UploadResult::getServerName).
        Func("setServerName", &UploadResult::setServerName).
        Func("setServerName", &UploadResult::setServerName)
        );

    root.Bind("UploadTaskWrapper", Class<UploadTaskWrapper>(hvm, "UploadTaskWrapper").
        Func("role", &UploadTaskWrapper::role).
        Func("type", &UploadTaskWrapper::type).
        Func("setRole", &UploadTaskWrapper::setRole).
        Func("getMimeType", &UploadTaskWrapper::getMimeType).
        Func("getDataLength", &UploadTaskWrapper::getDataLength).
        Func("parentTask", &UploadTaskWrapper::parentTask).
        Func("addTempFile", &UploadTaskWrapper::addTempFile).
        Func("addChildTask", &UploadTaskWrapper::addChildTask).
        Func("serverName", &UploadTaskWrapper::serverName).
        Func("profileName", &UploadTaskWrapper::profileName).
        Func("serverProfile", &UploadTaskWrapper::serverProfile).
        Func("isNull", &UploadTaskWrapper::isNull).
        Func("setStatusText", &UploadTaskWrapper::setStatusText).
        Func("uploadResult", &UploadTaskWrapper::uploadResult)
        );

    root.Bind("FileUploadTaskWrapper", DerivedClass<FileUploadTaskWrapper, UploadTaskWrapper>(hvm, "FileUploadTaskWrapper").
        Ctor().
        Ctor<const std::string&, const std::string&>().
        Func("getFileName", &FileUploadTaskWrapper::getFileName).
        Func("getFileSize", &FileUploadTaskWrapper::getFileSize).
        Func("setFileName", &FileUploadTaskWrapper::setFileName).
        Func("getDisplayName", &FileUploadTaskWrapper::getDisplayName).
        Func("setDisplayName", &FileUploadTaskWrapper::setDisplayName).
        Func("originalFileName", &FileUploadTaskWrapper::originalFileName).
        Func("isVideo", &FileUploadTaskWrapper::isVideo).
        Func("isImage", &FileUploadTaskWrapper::isImage)
    );

    root.Bind("UrlShorteningTaskWrapper", DerivedClass<UrlShorteningTaskWrapper, UploadTaskWrapper>(hvm, "UrlShorteningTaskWrapper").
        Func("getUrl", &UrlShorteningTaskWrapper::getUrl).
        Func("setParentUrlType", &UrlShorteningTaskWrapper::setParentUrlType).
        Func("parentUrlType", &UrlShorteningTaskWrapper::parentUrlType)
        );

    root.Bind("UploadTaskUnion", Class<UploadTaskUnion>(hvm, "UploadTaskUnion")
        .Ctor().Func("type", &UploadTaskUnion::type)
        .Func("getTask", &UploadTaskUnion::getTask)
        .Func("getFileTask", &UploadTaskUnion::getFileTask)
        .Func("getUrlShorteningTask", &UploadTaskUnion::getUrlShorteningTask)
    );
}

UploadTaskUnion::UploadTaskUnion(std::shared_ptr<UploadTask> task) : task_(std::move(task)) {
}

UploadTaskUnion::UploadTaskUnion(UploadTask* task){
    release_deleter<UploadTask> deleter;
    deleter.release();
    task_.reset(task, deleter);
}


std::string UploadTaskUnion::type() const{
    return UploadTask::EnumToString(task_->type());
}

ScriptAPI::UploadTaskWrapper UploadTaskUnion::getTask() {
    return task_;
}

ScriptAPI::FileUploadTaskWrapper UploadTaskUnion::getFileTask() {
    auto res = std::dynamic_pointer_cast<FileUploadTask>(task_);
    if (!res) {
        throw std::runtime_error("Pointer cast failed. It is not FileUploadTask");
    }
    return res;
}


ScriptAPI::UrlShorteningTaskWrapper UploadTaskUnion::getUrlShorteningTask() {
    auto res = std::dynamic_pointer_cast<UrlShorteningTask>(task_);
    if (!res) {
        throw std::runtime_error("Pointer cast failed. It is not UrlShorteningTask");
    }
    return res;
}

}
