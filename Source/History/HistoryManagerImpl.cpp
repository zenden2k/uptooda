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

#include "HistoryManagerImpl.h"

#include <ctime>
#include <algorithm>

#include <boost/format.hpp>
#include <boost/filesystem.hpp>
#include <sqlite3.h>
#include "Core/Utils/SimpleXml.h"
#include "Core/Utils/CoreUtils.h"
#include "Core/Utils/CryptoUtils.h"
#include "Core/Utils/GlobalMutex.h"
#include "Core/3rdpart/pcreplusplus.h"
#include "Core/Utils/StringUtils.h"

class CHistoryReader_impl
{
    public:
        SimpleXml m_xml;
        std::vector<std::unique_ptr<CHistorySession>> m_sessions;
        std::map<std::string, size_t> keyToIndex_;
        CHistoryManager* mgr_{};
};

const char CHistoryManager::globalMutexName[] = "IuHistoryFileSessionMutex";

CHistoryManager::CHistoryManager(const std::string& directory)
    : db_(nullptr)
    , mt_(rd_()) {
    m_historyFileNamePrefix = "history";
    m_historyFilePath = directory;
}

CHistoryManager::~CHistoryManager()
{
    if (db_) {
        sqlite3_close(db_);
    }
}

bool CHistoryManager::openDatabase() {
    IuCoreUtils::CreateDir(m_historyFilePath);
    if (!db_ && sqlite3_open((m_historyFilePath + "history.db").c_str(), &db_) != SQLITE_OK) {
        LOG(ERROR) << "unable to open database: ";
        return false;
    }
    sqlite3_busy_timeout(db_, 1000); // Wait 1 second before SQLITE_BUSY is returned
    const char* sql = "CREATE TABLE IF NOT EXISTS upload_sessions(id PRIMARY KEY NOT NULL,created_at INT NOT NULL)";
    char* err = nullptr;
    if (sqlite3_exec(db_, sql, nullptr, nullptr, &err) != SQLITE_OK) {
        LOG(ERROR) << "SQL error: " << err;
        sqlite3_free(err);
        return false;
    }
    const char* sql2 = "CREATE TABLE IF NOT EXISTS uploads(`id` INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,session_id,created_at INTEGER,"
                       "local_file_path, server_name, direct_url, thumb_url,"
                       "view_url,direct_url_shortened,view_url_shortened, edit_url, delete_url, display_name, size INTEGER, sort_index INTEGER);"
                       "CREATE INDEX IF NOT EXISTS idx_uploads_session_id ON uploads(session_id);"
                       "CREATE INDEX IF NOT EXISTS idx_uploads_created_at ON uploads(created_at);";

    if (sqlite3_exec(db_, sql2, nullptr, nullptr, &err) != SQLITE_OK) {
        LOG(ERROR) << "SQL error: " << err;
        sqlite3_free(err);
        return false;
    }
    return true;
}

bool CHistoryManager::saveSession(CHistorySession* session) {
    if (session->dbEntryCreated()) {
        return true;
    }
    IuCoreUtils::ZGlobalMutex mutex(globalMutexName);
    if (session->dbEntryCreated()) {
        return true;
    }
    const char* sql = "INSERT INTO upload_sessions(id,created_at) VALUES(?,?)";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        LOG(ERROR) << "SQL error: Could not prepare statement.";
        return false;
    }
    std::string id = session->sessionId();
    if (sqlite3_bind_text(stmt, 1 /*Index of wildcard*/, id.c_str(), -1, nullptr) != SQLITE_OK) {
        LOG(ERROR) << "SQL error: Could not bind value.";
        return false;
    }
    if (sqlite3_bind_int64(stmt, 2, session->timeStamp()) != SQLITE_OK) {
        LOG(ERROR) << "SQL error: Could not bind value.";
        return false;
    }
    /*if (sqlite3_bind_text(stmt, 3 , serverName.c_str(), -1, nullptr) != SQLITE_OK) {
    LOG(ERROR) << "SQL error: Could not bind value.";
    }*/
    int retCode = sqlite3_step(stmt);
    if (retCode  != SQLITE_DONE) {
        int i = 3;
        while(retCode == SQLITE_BUSY && i-- > 0) {
            retCode = sqlite3_step(stmt);
        }
        if (retCode != SQLITE_DONE) {
            LOG(ERROR) << "SQL error: Could not execute statement, return code=" << retCode << std::endl << sqlite3_errmsg(db_);
        }
    }
    /*sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);*/
    sqlite3_finalize(stmt);
    session->setDbEntryCreated(true);

    return true;
}

bool CHistoryManager::saveHistoryItem(HistoryItem* ht) {
    saveSession(ht->session);
    IuCoreUtils::ZGlobalMutex mutex(globalMutexName);

    const char* sql = "INSERT INTO uploads(session_id,created_at,local_file_path,server_name,direct_url,thumb_url,"
        "view_url,direct_url_shortened,view_url_shortened, edit_url, delete_url, display_name, size, sort_index) VALUES(?,?,?,?,?,?,?,?,?,?,?,?,?,?); ";
    sqlite3_stmt *stmt;

    if (sqlite3_prepare_v3(db_, sql, -1, 0, &stmt, nullptr) != SQLITE_OK) {
        LOG(ERROR) << "SQL error: Could not prepare statement." << sqlite3_errmsg(db_);
        return false;
    }
    bindString(stmt, 1, ht->session->sessionId());
    if (sqlite3_bind_int64(stmt, 2 /*Index of wildcard*/, ht->timeStamp) != SQLITE_OK) {
        LOG(ERROR) << "SQL error: Could not bind value.";
        return false;
    }
    bindString(stmt, 3, ht->localFilePath);
    bindString(stmt, 4, ht->serverName);
    bindString(stmt, 5, ht->directUrl);
    bindString(stmt, 6, ht->thumbUrl);
    bindString(stmt, 7, ht->viewUrl);
    bindString(stmt, 8, ht->directUrlShortened);
    bindString(stmt, 9, ht->viewUrlShortened);
    bindString(stmt, 10, ht->editUrl);
    bindString(stmt, 11, ht->deleteUrl);
    bindString(stmt, 12, ht->displayName);

    if (sqlite3_bind_int64(stmt, 13 /*Index of wildcard*/, ht->uploadFileSize) != SQLITE_OK) {
        LOG(ERROR) << "SQL error: Could not bind value.";
        return false;
    }
    if (sqlite3_bind_int(stmt, 14 /*Index of wildcard*/, ht->sortIndex) != SQLITE_OK) {
        LOG(ERROR) << "SQL error: Could not bind value.";
        return false;
    }
    int retCode = sqlite3_step(stmt);
    if (retCode != SQLITE_DONE) {
        int i = 3;

        while (retCode == SQLITE_BUSY && i-- > 0) {
            retCode = sqlite3_step(stmt);
        }
        if (retCode != SQLITE_DONE) {
            LOG(ERROR) << "SQL error: Could not execute statement, return code=" << retCode << std::endl << sqlite3_errmsg(db_);
        }
        return false;
    }
    /*sqlite3_reset(stmt);
    sqlite3_clear_bindings(stmt);*/
    sqlite3_finalize(stmt);

    return true;
}

std::shared_ptr<CHistorySession> CHistoryManager::newSession()
{
    time_t t = time(nullptr);

    // Используем потокобезопасную версию localtime
    tm timeinfo;
#ifdef _WIN32
    localtime_s(&timeinfo, &t);
#else
    localtime_r(&t, &timeinfo);
#endif

    std::string fileName = m_historyFilePath + m_historyFileNamePrefix + "_"
        + std::to_string(1900 + timeinfo.tm_year) + "_"
        + std::to_string(timeinfo.tm_mon + 1) + ".xml";
    std::lock_guard<std::mutex> lock(sessionMutex_);

    std::uniform_int_distribution<int> dist1(0, 256 * 256 - 1);
    std::uniform_int_distribution<int> dist2(0, 256 - 1);

    std::string str = std::to_string(dist1(mt_)) + std::to_string(static_cast<int>(t));
    std::string id = IuCoreUtils::CryptoUtils::CalcMD5HashFromString(
        str + std::to_string(dist2(mt_)))
                         .substr(0, 16);

    auto res = std::make_shared<CHistorySession>(fileName, id);
    res->setTimeStamp(t);

    return res;
}

bool CHistoryManager::bindString(sqlite3_stmt* stmt, int index, const std::string& val) {
    if (!val.empty()) {
        char* str = new char[val.size() + 1];
        strcpy(str, val.c_str());
        if (sqlite3_bind_text(stmt, index /*Index of wildcard*/, str, -1, [](void* s) {
                delete[] static_cast<char*>(s);
            })
            != SQLITE_OK) {
            LOG(ERROR) << "SQL error: Could not bind value.";
            return false;
        }
    } else {
        if (sqlite3_bind_null(stmt, index) != SQLITE_OK) {
            LOG(ERROR) << "SQL error: Could not bind null value.";
            return false;
        }
    }
    return true;
}

bool CHistoryManager::clearHistory(HistoryClearPeriod period) {
    IuCoreUtils::ZGlobalMutex mutex(globalMutexName);

    std::string condition;
    if (period == HistoryClearPeriod::OlderThan30Days) {
        time_t now;
        time(&now);
        tm* timeinfo = localtime(&now);
        IuCoreUtils::DatePlusDays(timeinfo, -30);
        time_t startTime = mktime(timeinfo);
        condition = str(boost::format(" AND created_at < %d") % startTime);
    }

    std::string sql = str(boost::format("DELETE from uploads WHERE TRUE %1% ; DELETE from upload_sessions WHERE TRUE  %1%") % condition);
    char *err = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
        LOG(ERROR) << "SQL error occured while clearing history: " << std::endl << err;
        sqlite3_free(err);
    }

    return true;
}

bool CHistoryManager::convertHistory() {
    IuCoreUtils::ZGlobalMutex mutex(globalMutexName);
    std::string historyFolder = m_historyFilePath;
    std::uniform_int_distribution<int> dist(1000000);
    pcrepp::Pcre regexp(R"(^history_(\d+)_(\d+)\.xml$)");
    try {
        boost::filesystem::directory_iterator endItr;

        for (boost::filesystem::directory_iterator i(historyFolder, boost::filesystem::directory_options::skip_permission_denied); i != endItr; ++i) {
            // Skip if not a file
            if (!boost::filesystem::is_regular_file(i->status())) {
                continue;
            }

            // Skip if no match

            const boost::filesystem::path& path = i->path();
            if (!regexp.search(path.filename().string())) {
                continue;
            }
            int year = atoi(regexp[1].c_str());
            int month = atoi(regexp[2].c_str());

            std::string fileName = path.string();

            CHistoryReader reader(this);
            if (!reader.loadFromFile(fileName)) {
                LOG(ERROR) << "Failed to convert history file " << fileName;
                continue;
            }


            for (auto& session : reader) {
                for (auto& item : *session) {
                    saveHistoryItem(&item);
                }
            }

            std::string newName = historyFolder + i->path().filename().string() + std::to_string(dist(mt_)) + ".bak";
            if (!IuCoreUtils::MoveFileOrFolder(i->path().string(), newName)) {
                LOG(ERROR) << "Unable to rename file " << i->path();
            }
        }
    } catch (boost::filesystem::filesystem_error& e) {
        LOG(ERROR) << "filesystem_error:" << e.what();
    }
    return true;
}

// class CHistoryReader
//
CHistoryReader::CHistoryReader(CHistoryManager* mgr)
{
    d_ptr = std::make_unique<CHistoryReader_impl>();
    d_ptr->mgr_ = mgr;
}

size_t CHistoryReader::getSessionCount() const
{
    return d_ptr->m_sessions.size();
}

CHistorySession* CHistoryReader::getSession(size_t index) const
{
    if (index >= d_ptr->m_sessions.size()) {
        return {};
    }
    return d_ptr->m_sessions[index].get();
}

bool CHistoryReader::loadFromFile(const std::string& filename)
{
    if(IuCoreUtils::FileExists(filename))
    {
        if (!d_ptr->m_xml.LoadFromFile(filename)) {
            return false;
        }
    }
    else {
        return false;
    }
    SimpleXmlNode root = d_ptr->m_xml.getRoot("History");
    std::vector<SimpleXmlNode> allSessions;
    root.GetChilds("Session", allSessions);

    for(auto & allSession : allSessions)
    {
        std::string fName = filename;
        auto session = std::make_unique<CHistorySession>(fName, "0");
        loadSessionFromXml(session.get(), allSession);
        d_ptr->m_sessions.push_back(std::move(session));
    }
    return true;
}

bool CHistoryReader::loadFromDB(time_t from, time_t to, const std::string& filename, const std::string& url) {
    sqlite3* db = d_ptr->mgr_->db_;

    std::string condition, condition2;

    if (from && to) {
        condition += boost::str(boost::format(" and  created_at between %d and %d") % from % to);
    }

    std::string sql = "SELECT * from upload_sessions WHERE true " + condition;

    char *err = nullptr;
    if (sqlite3_exec(db, sql.c_str(), selectCallback, this, &err) != SQLITE_OK) {
        LOG(ERROR) << "SQL error: " << err;
        sqlite3_free(err);
    }

    if (!filename.empty()) {
        std::string preparedFilename = IuStringUtils::Replace(filename, "'", "''");
        preparedFilename = IuStringUtils::Replace(preparedFilename, "_", "\\_");
        condition2 = boost::str(boost::format(" AND(local_file_path LIKE '%%%s%%' ESCAPE '\\')")%preparedFilename);
    }

    if (!url.empty()) {
        std::string preparedURL = IuStringUtils::Replace(url, "'", "''");
        preparedURL = IuStringUtils::Replace(preparedURL, "_", "\\_");
        std::string subExpr = "FALSE ";
        std::string fields[]{
            "direct_url", "view_url", "thumb_url", "direct_url_shortened", "view_url_shortened"
        };
        for (const auto& field : fields) {
            subExpr += boost::str(boost::format(" OR %1% LIKE '%%%2%%%' ESCAPE '\\'") % field %  preparedURL);
        }
        condition2 += boost::str(boost::format(" AND(%s)") % subExpr);
    }
    std::string query2 = "SELECT * from uploads WHERE true " + condition + condition2;

    if (sqlite3_exec(db, query2.c_str(), selectCallback2, this, &err) != SQLITE_OK) {
        LOG(ERROR) << "SQL error: " << err;
        sqlite3_free(err);
        return false;
    }

    for (auto it = d_ptr->m_sessions.begin(); it != d_ptr->m_sessions.end(); ) {
        if (!(*it)->entriesCount()) {
            it = d_ptr->m_sessions.erase(it);
        } else {
            (*it)->sortByOrderIndex();
            ++it;
        }
    }
    return true;
}

int CHistoryReader::selectCallback(void* userData, int argc, char **argv, char **azColName){
    auto pthis = static_cast<CHistoryReader*>(userData);

    std::string sessionId;
    for (int i = 0; i < argc; i++) {
        if (strcmp(azColName[i], "id") == 0) {
            sessionId = argv[i] ? argv[i] : "";
            break;
        }
    }

    auto session = std::make_unique<CHistorySession>("", sessionId);
    for (int i = 0; i < argc; i++) {
        const char* val = argv[i] ? argv[i] : "";
        if (strcmp(azColName[i], "created_at") == 0) {
            session->setTimeStamp(IuCoreUtils::StringToInt64(val));
            break;
        } else if (strcmp(azColName[i], "server_name") == 0) {
            session->setServerName(val);
            break;
        }
    }

    pthis->d_ptr->m_sessions.push_back(std::move(session));
    size_t index = pthis->d_ptr->m_sessions.size() - 1;
    pthis->d_ptr->keyToIndex_[sessionId] = index;
    return 0;
}

int CHistoryReader::selectCallback2(void* userData, int argc, char **argv, char **azColName){

    auto pthis = static_cast<CHistoryReader*>(userData);

    std::string sessionId;
    for (int i = 0; i < argc; i++) {
        if (strcmp(azColName[i], "session_id") == 0) {
            sessionId = argv[i] ? argv[i] : std::string();
            break;
        }
    }

    const auto it = pthis->d_ptr->keyToIndex_.find(sessionId);
    if (it == pthis->d_ptr->keyToIndex_.end()) {
        return 0; // No session with such id
    }
    CHistorySession* session = pthis->d_ptr->m_sessions[it->second].get();
    HistoryItem item(session);

    for (int i = 0; i < argc; i++) {
        const char *val = argv[i] ? argv[i] : "";
        if (strcmp(azColName[i], "id") == 0) {
            item.id = static_cast<int>(IuCoreUtils::StringToInt64(val));
        } else if (strcmp(azColName[i], "direct_url") == 0) {
            item.directUrl = val;
        } else if (strcmp(azColName[i], "view_url") == 0) {
            item.viewUrl = val;
        } else if (strcmp(azColName[i], "thumb_url") == 0) {
            item.thumbUrl = val;
        } else if (strcmp(azColName[i], "direct_url_shortened") == 0) {
            item.directUrlShortened = val;
        } else if (strcmp(azColName[i], "view_url_shortened") == 0) {
            item.viewUrlShortened = val;
        } else if (strcmp(azColName[i], "delete_url") == 0) {
            item.deleteUrl = val;
        } else if (strcmp(azColName[i], "edit_url") == 0) {
            item.editUrl = val;
        } else if (strcmp(azColName[i], "display_name") == 0) {
            item.displayName = val;
        } else if (strcmp(azColName[i], "local_file_path") == 0) {
            item.localFilePath = val;
        } else if (strcmp(azColName[i], "created_at") == 0) {
            item.timeStamp = IuCoreUtils::StringToInt64(val);
        } else if (strcmp(azColName[i], "sort_index") == 0) {
            item.sortIndex = static_cast<int>(IuCoreUtils::StringToInt64(val));
        } else if (strcmp(azColName[i], "size") == 0) {
            item.uploadFileSize = IuCoreUtils::StringToInt64(val);
        } else if (strcmp(azColName[i], "server_name") == 0) {
            item.serverName = val;
        }
    }

    session->m_entries.push_back(std::move(item));

    return 0;
}

std::vector<std::unique_ptr<CHistorySession>>::iterator CHistoryReader::begin() {
    return d_ptr->m_sessions.begin();
}
std::vector<std::unique_ptr<CHistorySession>>::iterator CHistoryReader::end() {
    return d_ptr->m_sessions.end();
}

CHistoryReader::~CHistoryReader() {
}

void CHistoryReader::loadSessionFromXml(CHistorySession* session, SimpleXmlNode& sessionNode) {
    session->m_timeStamp = sessionNode.AttributeInt("TimeStamp");
    session->m_serverName = sessionNode.Attribute("ServerName");
    session->m_sessId = sessionNode.Attribute("ID");
    std::vector<SimpleXmlNode> allEntries;
    sessionNode.GetChilds("Entry", allEntries);

    for (size_t i = 0; i < allEntries.size(); i++) {
        SimpleXmlNode& item = allEntries[i];
        HistoryItem ht(session);
        ht.localFilePath = item.Attribute("LocalFilePath");
        ht.serverName = item.Attribute("ServerName");
        ht.timeStamp = item.AttributeInt("TimeStamp");
        ht.directUrl = item.Attribute("DirectUrl");
        ht.thumbUrl = item.Attribute("ThumbUrl");
        ht.viewUrl = item.Attribute("ViewUrl");
        ht.editUrl = item.Attribute("EditUrl");
        ht.deleteUrl = item.Attribute("DeleteUrl");
        ht.displayName = item.Attribute("DisplayName");
        ht.uploadFileSize = item.AttributeInt64("UploadFileSize");
        std::string sortIndex = item.Attribute("Index");
        ht.sortIndex = sortIndex.empty() ? i : static_cast<int>(IuCoreUtils::StringToInt64(sortIndex));
        // Fix invalid file size
        if (ht.uploadFileSize > 1000000000000 || ht.uploadFileSize < 0) {
            ht.uploadFileSize = 0;
        }
        session->m_entries.push_back(ht);
    }
}
