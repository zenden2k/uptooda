#include "DownloadTask.h"

#include <cstring>
#include <boost/filesystem.hpp>

#include "Core/Utils/CoreUtils.h"
#include "Core/3rdpart/UriParser.h"
#include "Core/Utils/StringUtils.h"

DownloadTask::DownloadTask(std::shared_ptr<INetworkClientFactory> factory, std::string tempDirectory, const std::vector<DownloadItem>& files):
    factory_(std::move(factory)), files_(files), tempDirectory_(std::move(tempDirectory))
{
    isCanceled_ = false;
    isInProgress_ = false;
}

void DownloadTask::run() {
    if (isCanceled_) {
        onTaskFinished(this);
        return;
    }
    isInProgress_ = true;
    std::unique_ptr<INetworkClient> nm(factory_->create());
    using namespace std::placeholders;
    nm->setProgressCallback([this](INetworkClient* /* userData*/, int64_t /*dltotal*/, int64_t /*dlnow*/, int64_t /* ultotal*/, int64_t /*ulnow*/) -> int {
        if (isCanceled_) {
            return -1;
        }

        return 0;
    });

    for (auto& file : files_) {
        if (isCanceled_) {
            break;
        }
        uriparser::Uri uri(file.url);
        std::string path = uri.path();
        std::string fileName = nm->urlDecode(IuCoreUtils::ExtractFileName(path));

        std::string ext = IuCoreUtils::ExtractFileExt(fileName);
        if (fileName.length() > 30) {
            fileName = fileName.substr(0, 30);
        }
        file.displayName = fileName;
        std::string filePath;

        if (file.fileName.empty()) {
            boost::filesystem::path temp = boost::filesystem::unique_path();
            const auto& tempstr = temp.native();

#ifdef _WIN32
            filePath = IuCoreUtils::WstringToUtf8(tempstr);
#else
            filePath = tempstr;
#endif
            if (!ext.empty()) {
                filePath += "." + ext;
            }
        }
        else {
            filePath = file.fileName;
        }

        //if (createFileBeforeDownloading_) {
            // Creating file
        std::string fullFilePath = tempDirectory_ + filePath;
        FILE* f = IuCoreUtils::FopenUtf8(fullFilePath.c_str(), "wb");
        if (f) {
            fclose(f);
        } else {
            std::error_code ec(errno, std::generic_category());

            LOG(ERROR) << "Unable to create file:" << std::endl << filePath << std::endl
                << "Error: " << ec.message();
            return;
        }
        std::string url = file.url;
        url = IuStringUtils::Replace(url, " ", "%20");

        nm->setOutputFile(fullFilePath);
        if (!file.referer.empty()) {
            nm->setReferer(file.referer);
        }
        try {
            nm->doGet(url);
        }
        catch (INetworkClient::AbortedException&) {

        }

        bool success = nm->responseCode() >= 200 && nm->responseCode() <= 299;
        file.fileName = fullFilePath;
        onFileFinished(success, nm->responseCode(), file);
        //}
    }
    isInProgress_ = false;
    onTaskFinished(this);
}

void DownloadTask::cancel() {
    isCanceled_ = true;
}

bool DownloadTask::isCanceled() {
    return isCanceled_;
}

bool DownloadTask::isInProgress() {
    return isInProgress_;
}
