#include "UrlShorteningTask.h"

#include <cassert>

#include "Core/Utils/CoreUtils.h"


UrlShorteningTask::UrlShorteningTask(const std::string& url, UploadTask* parentTask) : UploadTask(parentTask) {
    url_ = url;
    parentUrlType_ = None;
}

UploadTask::Type UrlShorteningTask::type() const {
    return TypeUrl;
}

std::string UrlShorteningTask::getMimeType() const {
    return "text/plain";
}

int64_t UrlShorteningTask::getDataLength() const {
    return url_.length();
}

std::string UrlShorteningTask::getUrl() const {
    return url_;
}

void UrlShorteningTask::finishTask(Status status) 
{
    if (status == StatusFinished && role_ == UrlShorteningRole && uploadSuccess())
    {
        assert(parentTask_);
        if (!parentTask_)
        {
            return;
        }
        //TODO: keep server thumbnail url
        if (parentUrlType_ == DirectUrl)
        {
            parentTask_->uploadResult()->directUrlShortened = uploadResult_.directUrl;
        }
        else if (parentUrlType_ == DownloadUrl)
        {
            parentTask_->uploadResult()->downloadUrlShortened = uploadResult_.directUrl;
        }
        
    }
    UploadTask::finishTask(status);
}

void UrlShorteningTask::setParentUrlType(ParentUrlType type)
{
    parentUrlType_ = type;
}

UrlShorteningTask::ParentUrlType UrlShorteningTask::parentUrlType() const {
    return parentUrlType_;
}

std::string UrlShorteningTask::toString()
{
    return "UrlShorteningTask(" + url_ + ")";
}

std::string UrlShorteningTask::title() const
{
    return url_;
}