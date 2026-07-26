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

#ifndef IU_CORE_HISTORY_MANAGER_H
#define IU_CORE_HISTORY_MANAGER_H

#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <random>

#include "Core/HistoryManager.h"

class SimpleXmlNode;
class CHistoryReader_impl;
class CHistoryReader;
struct sqlite3;
struct sqlite3_stmt;

class CHistoryManager: public IHistoryManager
{
    public:
        explicit CHistoryManager(const std::string& directory);
        bool openDatabase() override;
        ~CHistoryManager() override;

        std::shared_ptr<CHistorySession> newSession() override;

        bool clearHistory(HistoryClearPeriod period) override;
        bool saveHistoryItem(HistoryItem* item) override;
        bool saveSession(CHistorySession* session) override;
        /**
         * Load history files (*xml) into sqlite database
         */
        bool convertHistory() override;
        static const char globalMutexName[];
    private:
        DISALLOW_COPY_AND_ASSIGN(CHistoryManager);
        std::string m_historyFilePath;
        std::string m_historyFileNamePrefix;
        sqlite3* db_;
        std::random_device rd_;
        std::mt19937 mt_;
        std::mutex sessionMutex_;
        bool bindString(sqlite3_stmt* stmt, int index, const std::string& val);
        friend class CHistoryReader;
};

class CHistoryReader : public IHistoryReader {
    public:
        explicit CHistoryReader(CHistoryManager* mgr);
        ~CHistoryReader() override;
        // filename must be utf-8 encoded
        bool loadFromFile(const std::string& filename) override;
        bool loadFromDB(time_t from, time_t to, const std::string& filename, const std::string& url) override;
        size_t getSessionCount() const override;
        void loadSessionFromXml(CHistorySession* session, SimpleXmlNode& sessionNode);

        // The pointer returned by this function is only valid
        //  during lifetime of CHistoryReader object
        CHistorySession* getSession(size_t index) const override;

        std::vector<std::unique_ptr<CHistorySession>>::iterator begin();
        std::vector<std::unique_ptr<CHistorySession>>::iterator end();

    private:
        DISALLOW_COPY_AND_ASSIGN(CHistoryReader);
        std::unique_ptr<CHistoryReader_impl> d_ptr;

        static int selectCallback(void* userData, int argc, char **argv, char **azColName);
        static int selectCallback2(void* userData, int argc, char **argv, char **azColName);
};
#endif
