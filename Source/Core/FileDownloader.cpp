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

#include "Core/FileDownloader.h"

#include <cerrno>
#include <algorithm>

#include "Core/Utils/CoreUtils.h"
#include "Func/WinUtils.h"
#include "Core/3rdpart/UriParser.h"
#include "Utils/StringUtils.h"
#include "Network/NetworkClient.h"

// TODO: Use pimpl
CFileDownloader::CFileDownloader(std::shared_ptr<INetworkClientFactory> factory, std::string tempDirectory, bool createFilesBeforeDownloading)
    : tempDirectory_(std::move(tempDirectory)), networkClientFactory_(std::move(factory)), createFileBeforeDownloading_(createFilesBeforeDownloading)
{
    maxThreads_ = 3;
    stopSignal_ = false;
    isRunning_ = false;
    runningThreads_ = 0;
}

CFileDownloader::~CFileDownloader()
{
    waitForFinished();
}

void CFileDownloader::addFile(const std::string& url, void* id, const std::string& referer) {
    if (stopSignal_)
        return;  // Cannot add file to queue while stopping process
    std::lock_guard<std::mutex> guard(mutex_);

    DownloadFileListItem newItem;
    newItem.url = url;
    newItem.id = id;
    newItem.referer = referer;
    newItem.displayName = IuCoreUtils::ExtractFileNameFromUrl(url);
    fileList_.push_back(newItem);
}

void CFileDownloader::setThreadCount(int n)
{
    maxThreads_ = n;
}

void CFileDownloader::start() {
    stopSignal_ = false;
    std::lock_guard<std::mutex> guard(mutex_);

    const size_t numThreads = std::min(maxThreads_ - runningThreads_, int(fileList_.size()));
    for (size_t i = 0; i < numThreads; i++) {
        runningThreads_++;
        isRunning_ = true;
        threads_.emplace_back(&CFileDownloader::memberThreadFunc, this);
    }
}

void CFileDownloader::memberThreadFunc()
{
    std::unique_ptr<INetworkClient> nm(networkClientFactory_->create());

    // Providing callback function to stop downloading
    using namespace std::placeholders;
    nm->setProgressCallback([this](auto && PH1, auto && PH2, auto && PH3, auto && PH4, auto && PH5) {
        return ProgressFunc(std::forward<decltype(PH1)>(PH1), std::forward<decltype(PH2)>(PH2),
            std::forward<decltype(PH3)>(PH3), std::forward<decltype(PH4)>(PH4), std::forward<decltype(PH5)>(PH5)
            );
    });
    {
        std::lock_guard<std::mutex> lk(mutex_);
        if (onConfigureNetworkClient_) {
            onConfigureNetworkClient_(nm.get());
        }
    }

    for (;;)
    {
        DownloadFileListItem curItem;
        if (!getNextJob(curItem)) {
            break;
        }

        std::string url = curItem.url;
        if (url.empty()) {
            break;
        }
        url = IuStringUtils::Replace(url, " ", "%20");

        nm->setOutputFile(curItem.fileName);
        if ( !curItem.referer.empty() ) {
            nm->setReferer(curItem.referer);
        }
        try {
            nm->doGet(url);
        } catch (INetworkClient::AbortedException&) {
            break;
        }

        if (stopSignal_) {
            break;
        }

        {
            std::lock_guard<std::mutex> lk(mutex_);

            bool success = nm->responseCode() >= 200 && nm->responseCode() <= 299;

            if (onFileFinished_) {
                onFileFinished_(success, nm->responseCode(), curItem);
            }

            if (stopSignal_) {
                fileList_.clear();
            }
        }
    }

    {
        std::lock_guard<std::mutex> lk(mutex_);
        runningThreads_--;

        if (stopSignal_) {
            fileList_.clear();
        }
    }

    {
        std::lock_guard<std::mutex> lk(threadsStatusMutex_);
        // otherwise we may get a deadlock
        if (!runningThreads_ && isRunning_) {
            isRunning_ = false;
            stopSignal_ = false;
            if (onQueueFinished_) {
                onQueueFinished_();
            }
        }
    }
}

bool CFileDownloader::getNextJob(DownloadFileListItem& item)
{
    bool result = false;
    std::lock_guard<std::mutex> guard(mutex_);

    if (!fileList_.empty() && !stopSignal_)
    {
        item = *fileList_.begin();

        std::string url = item.url;
        fileList_.erase(fileList_.begin());

        uriparser::Uri uri(url);
        std::string path = uri.path();
        NetworkClient nc;
        std::string fileName = nc.urlDecode(IuCoreUtils::ExtractFileName(path));

        if (fileName.length() > 50) {
            size_t pos = fileName.find_last_of('.');
            if (pos != std::string::npos) {
                fileName = fileName.substr(0, 46) + "." + fileName.substr(pos);
            } else {
                fileName = fileName.substr(0, 50);
            }
        }

        CString possiblePath = U2W(tempDirectory_) + Utf8ToWstring(fileName).c_str();
        if (possiblePath.GetLength() > MAX_PATH - 4) {
            possiblePath = possiblePath.Left(MAX_PATH - 4);
        }
        CString wFileName = WinUtils::GetUniqFileName(possiblePath);
        std::string filePath = WCstringToUtf8(wFileName);

        if (createFileBeforeDownloading_) {
            // Creating file
            FILE* f = _tfopen(wFileName, L"wb");
            if (f) {
                fclose(f);
            } else {
                LOG(ERROR) << "Unable to create file:" << std::endl << wFileName << std::endl
                    << "Error: " << strerror(errno);
                return false;
            }
        }
        item.fileName = std::move(filePath);
        result = true;
    }

    return result;
}

void CFileDownloader::stop()
{
    stopSignal_ = true;
}

bool CFileDownloader::isRunning() const
{
    return isRunning_;
}

void CFileDownloader::setOnFileFinishedCallback(decltype(onFileFinished_) callback) {
    onFileFinished_ = std::move(callback);
}

void CFileDownloader::setOnQueueFinishedCallback(decltype(onQueueFinished_) callback) {
    onQueueFinished_ = std::move(callback);
}

void CFileDownloader::setOnConfigureNetworkClientCallback(decltype(onConfigureNetworkClient_) callback) {
    onConfigureNetworkClient_ = std::move(callback);
}

bool CFileDownloader::waitForFinished()
{
    if (threads_.empty()) {
        return true;
    }

    for (auto& th : threads_) {
        if (th.joinable()) {
            th.join();
        }
    }
    return true;
}

int CFileDownloader::ProgressFunc(INetworkClient* userData, int64_t dltotal, int64_t dlnow, int64_t ultotal, int64_t ulnow) {
    if (stopSignal_) {
        return -1;
    }

    return 0;
}
