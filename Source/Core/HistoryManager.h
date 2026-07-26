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

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <random>

#include "Core/Utils/CoreTypes.h"

class CHistorySession;

struct HistoryItem
{
    std::string localFilePath;
    std::string directUrl;
    std::string directUrlShortened;
    std::string viewUrl;
    std::string viewUrlShortened;
    std::string thumbUrl;
    std::string editUrl;
    std::string deleteUrl;
    std::string displayName;

    std::string serverName;
    time_t timeStamp;
    int64_t uploadFileSize;
    int sortIndex;
    int id;
    CHistorySession* session;

    explicit HistoryItem(CHistorySession* historySession) : session(historySession){
        timeStamp = 0;
        time(&timeStamp);
        uploadFileSize = 0;
        sortIndex = 0;
        id = 0;
    }
};

class SimpleXmlNode;
class CHistoryReader;

class CHistorySession
{
    public:
        explicit CHistorySession(const std::string& filename, const std::string& sessionId);
        ~CHistorySession();
        size_t entriesCount() const;
        const HistoryItem& entry(size_t index) const;
        std::string serverName() const;
        void setServerName(const std::string& name);
        time_t timeStamp() const;
        void setTimeStamp(time_t timeStamp);
        std::string sessionId() const;
        std::vector<HistoryItem>::iterator begin();
        std::vector<HistoryItem>::iterator end();
        void setDeleteItems(bool doDelete);
        bool dbEntryCreated() const;
        void setDbEntryCreated(bool created);
        void sortByOrderIndex();
    private:
        std::string m_historyXmlFileName;
        std::string m_sessId;
        time_t m_timeStamp;
        std::string m_serverName;
        std::vector<HistoryItem> m_entries;
        bool dbEntryCreated_;
        bool deleteItems_;

        friend class IHistoryReader;
        friend class IHistoryManager;
        friend class CHistoryReader;
};

enum class HistoryClearPeriod { ClearAll, OlderThan30Days };

class IHistoryManager
{
    public:
        virtual bool openDatabase() = 0;
        virtual ~IHistoryManager() = default;
        virtual std::shared_ptr<CHistorySession> newSession() = 0;
        virtual bool clearHistory(HistoryClearPeriod period) = 0;
        virtual bool saveHistoryItem(HistoryItem* item) = 0;
        virtual bool saveSession(CHistorySession* session) = 0;

        /**
         * Load history files (*xml) into sqlite database
         */
        virtual bool convertHistory() = 0;
        friend class CHistoryReader;
};

class IHistoryReader {
public:
    virtual ~IHistoryReader() = default;
    // filename must be utf-8 encoded
    virtual bool loadFromFile(const std::string& filename) = 0;
    virtual bool loadFromDB(time_t from, time_t to, const std::string& filename, const std::string& url) = 0;
    virtual size_t getSessionCount() const = 0;
    // The pointer returned by this function is only valid
    //  during lifetime of CHistoryReader object
    virtual CHistorySession* getSession(size_t index) const = 0 ;
};
