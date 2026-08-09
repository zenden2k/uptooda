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

#include "FileUploadTask.h"

#include <cassert>

#include "Core/Utils/CoreUtils.h"

FileUploadTask::FileUploadTask(const std::string& fileName, const std::string& displayName, UploadTask* parentTask) :
    UploadTask(parentTask),
    originalFileName_(fileName),
    fileName_(fileName)
{
    tempFileDeleter_ = nullptr;
    isImage_ = false;
    isVideo_ = false;
    cachedFileSize_ = IuCoreUtils::GetFileSize(fileName);
    if ( displayName.empty() ) {
        displayName_ = IuCoreUtils::ExtractFileName(fileName);
    } else {
        displayName_ = displayName;
    }
    fileIndex_ = 0;
}

UploadTask::Type FileUploadTask::type() const {
    return TypeFile;
}

std::string FileUploadTask::getMimeType() const {
    return IuCoreUtils::GetFileMimeType(fileName_);
}

int64_t FileUploadTask::getDataLength() const {
    if (cachedFileSize_ != -1) {
        return cachedFileSize_;
    }
    cachedFileSize_ = IuCoreUtils::GetFileSize(fileName_);
    return cachedFileSize_;
}

std::string FileUploadTask::getFileName() const {
    return fileName_;
}

void FileUploadTask::setFileName(const std::string& fileName)
{
    fileName_ = fileName;
    cachedFileSize_ = IuCoreUtils::GetFileSize(fileName_);
}

std::string FileUploadTask::getDisplayName() const {
    return displayName_;
}

void FileUploadTask::setDisplayName(const std::string& name)
{
    displayName_ = name;
}

std::string FileUploadTask::originalFileName() const
{
    return originalFileName_;
}

void FileUploadTask::finishTask(Status status)
{
    if (status == StatusFinished && role_ == ThumbRole )
    {
        assert(parentTask_);
        if (!parentTask_)
        {
            return;
        }
        //TODO: keep server thumbnail url
        parentTask_->uploadResult()->thumbUrl = uploadResult_.directUrl;
    }
    UploadTask::finishTask(status);
}

std::string FileUploadTask::toString()
{
    return "FileUploadTask (" + fileName_ + ")";
}

std::string FileUploadTask::title() const
{
    return IuCoreUtils::ExtractFileName(originalFileName());
}

bool FileUploadTask::isImage() const {
    return isImage_;
}

void FileUploadTask::setIsImage(bool image) {
    isImage_ = image;
}

bool FileUploadTask::isVideo() const{
    return isVideo_;
}

void FileUploadTask::setIsVideo(bool isVid) {
    isVideo_ = isVid;
}

int64_t FileUploadTask::getFileSize() const
{
    return getDataLength();
}

void FileUploadTask::setFileIndex(size_t index) {
    fileIndex_ = index;
}
size_t FileUploadTask::fileIndex() const {
    return fileIndex_;
}
