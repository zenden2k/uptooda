#include "LocalFileCache.h"

#include <mutex>

#include <boost/filesystem.hpp>
#include "Core/Settings/BasicSettings.h"
#include "HistoryManager.h"
#include "Core/3rdpart/pcreplusplus.h"
#include "ServiceLocator.h"

LocalFileCache::LocalFileCache() {
    historyParsed = false;
}

bool LocalFileCache::ensureHistoryParsed() {
    if (!historyParsed) {
        parseHistory();
        historyParsed = true;
    }

    return true;
}

bool LocalFileCache::parseHistory() {
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    std::vector<std::string> files;
    auto settings = ServiceLocator::instance()->basicSettings();
    std::string historyFolder = settings->SettingsFolder + "/History/";
    boost::filesystem::directory_iterator endItr; // Default ctor yields past-the-end

    pcrepp::Pcre regexp("^history.+\\.xml$");
    for (boost::filesystem::directory_iterator i(historyFolder,
                                                 boost::filesystem::directory_options::skip_permission_denied); i !=
         endItr; ++i) {
        // Skip if not a file
        if (!boost::filesystem::is_regular_file(i->status())) {
            continue;
        }

        // Skip if no match
        if (!regexp.search(i->path().filename().string())) {
            continue;
        }

        // File matches, store it
        files.push_back(i->path().string());
    }
    //WinUtils::GetFolderFileList(files, historyFolder , _T("history*.xml"));

    for (const auto& file : files) {
        parseHistoryFile(historyFolder + file);
    }
    return true;
}

bool LocalFileCache::parseHistoryFile(const std::string& fileName) {
    /* CHistoryReader m_historyReader(ServiceLocator::instance()->historyManager());
    if ( !m_historyReader.loadFromFile(fileName) ) {
        return false;
    }
    int nSessionsCount = m_historyReader.getSessionCount();
    int nFilesCount = 0;

    for( int i = 0; i < nSessionsCount; i++ ) {
        CHistorySession* ses = m_historyReader.getSession(i);
        int nCount = ses->entriesCount();
        for ( int j = 0; j < nCount; j++ ){
            nFilesCount++;
            HistoryItem historyItem = ses->entry(j);
            if ( !historyItem.directUrl.empty() ) {
                addFile(historyItem.directUrl, historyItem.localFilePath);
            }

            if ( !historyItem.thumbUrl.empty() ) {
                addThumb(historyItem.thumbUrl, historyItem.localFilePath);
            }
        }
    }*/
    return true;
}

bool LocalFileCache::addFile(const std::string& url, const std::string& localFileName) {
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    cache_[url] = localFileName;
    return true;
}

std::string LocalFileCache::get(const std::string& url) {
    std::lock_guard<std::mutex> guard(cacheMutex_);
    std::map<std::string, std::string>::const_iterator foundItem = cache_.find(url);

    if (foundItem != cache_.end()) {
        if (IuCoreUtils::FileExists(foundItem->second)) {
            return foundItem->second;
        } else {
            // if file not found on disk, delete it from cache
            cache_.erase(foundItem);
        }
    }
    return {};
}

bool LocalFileCache::addThumb(const std::string& url, const std::string& localFileName) {
    std::lock_guard<std::recursive_mutex> guard(mutex_);
    thumbCache_[url] = localFileName;
    return true;
}

std::string LocalFileCache::getThumb(const std::string& url) {
    std::lock_guard<std::mutex> guard(cacheMutex_);
    std::map<std::string, std::string>::const_iterator foundItem = thumbCache_.find(url);

    if (foundItem != thumbCache_.end()) {
        if (IuCoreUtils::FileExists(foundItem->second)) {
            return foundItem->second;
        } else {
            // if file not found on disk, delete it from cache
            thumbCache_.erase(foundItem);
        }
    }
    return {};
}
