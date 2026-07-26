#include "HistoryManager.h"

#include <algorithm>

CHistorySession::CHistorySession(const std::string& filename, const std::string&  sessionId) 
{
    deleteItems_ = true;
    m_historyXmlFileName = filename;
    m_sessId = sessionId;
    time(&m_timeStamp);
    dbEntryCreated_ = false;
}

size_t CHistorySession::entriesCount() const
{
    return m_entries.size();
}

const HistoryItem& CHistorySession::entry(size_t index) const
{
    return m_entries[index];
}

void  CHistorySession::setDeleteItems(bool doDelete) {
    deleteItems_ = doDelete;
}

bool CHistorySession::dbEntryCreated() const {
    return dbEntryCreated_;
}

void CHistorySession::setDbEntryCreated(bool created) {
    dbEntryCreated_ = created;
}

CHistorySession::~CHistorySession() {
}

void CHistorySession::sortByOrderIndex() {
    std::sort(m_entries.begin(), m_entries.end(), [](const HistoryItem& lhs, const HistoryItem& rhs) {
        return lhs.sortIndex < rhs.sortIndex;
    });

    if (!m_entries.empty()) {
        m_serverName = m_entries[0].serverName;
    }
}

std::string CHistorySession::serverName() const
{
    return m_serverName;
}

void CHistorySession::setServerName(const std::string& name) {
    m_serverName = name;
}

time_t CHistorySession::timeStamp() const
{
    return m_timeStamp;
}

void CHistorySession::setTimeStamp(time_t timeStamp) {
    m_timeStamp = timeStamp;
}

std::string  CHistorySession::sessionId() const {
    return m_sessId;
}

std::vector<HistoryItem>::iterator CHistorySession::begin() {
    return m_entries.begin();
}

std::vector<HistoryItem>::iterator CHistorySession::end() {
    return m_entries.end();
}
