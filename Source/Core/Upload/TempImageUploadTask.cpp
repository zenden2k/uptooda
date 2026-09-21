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

#include "TempImageUploadTask.h"

#include <cassert>

#include "Core/Utils/CoreUtils.h"

TempImageUploadTask::TempImageUploadTask(const std::string& fileName, const std::string& displayName, UploadTask* parentTask)
    : FileUploadTask(fileName, displayName, parentTask) {
}

std::string TempImageUploadTask::getMimeType() const {
    return IuCoreUtils::GetFileMimeType(fileName_);
}

int64_t TempImageUploadTask::getDataLength() const {
    if (cachedFileSize_ != -1) {
        return cachedFileSize_;
    }
    cachedFileSize_ = IuCoreUtils::GetFileSize(fileName_);
    return cachedFileSize_;
}

void TempImageUploadTask::finishTask(Status status) {
    /* if (status == StatusFinished && role_ == ThumbRole) {
        assert(parentTask_);
        if (!parentTask_)
        {
            return;
        }
        //TODO: keep server thumbnail url
        parentTask_->uploadResult()->thumbUrl = uploadResult_.directUrl;
    }*/
    UploadTask::finishTask(status);
}

std::string TempImageUploadTask::toString() {
    return "TempImageUploadTask (" + fileName_ + ")";
}

std::string TempImageUploadTask::title() const {
    return IuCoreUtils::ExtractFileName(originalFileName());
}

const ServerProfile& TempImageUploadTask::searchServerProfile() const {
    return searchServerProfile_;
}

void TempImageUploadTask::setSearchServerProfile(const ServerProfile& profile) {
    searchServerProfile_ = profile;
}
