#ifndef IU_FUNC_DEFAULTLOGGER_H
#define IU_FUNC_DEFAULTLOGGER_H

#pragma once

#include <mutex>
#include <vector>

#include "Core/Logging/Logger.h"

class CLogWindow;

class DefaultLogger : public ILogger {
public:
    struct LogEntry {
        LogMsgType MsgType;
        std::wstring Sender;
        std::wstring Msg;
        std::wstring Info;
        std::wstring FileName;
        std::wstring Time;
    };

    class Listener {
    public:
        virtual ~Listener() = default;
        virtual void onItemAdded(size_t rowIndex, const LogEntry&) = 0;
    };

    void addListener(Listener* listener);
    void removeListener(const Listener* listener);
    size_t entryCount() const;
    void clear();
    void getEntry(size_t itemIndex, LogEntry* out);
    std::mutex& getEntryMutex();
    void write(LogMsgType MsgType, const std::string& Sender, const std::string&  Msg, const std::string&  Info, const std::string&  FileName) override;
    void write(LogMsgType MsgType, const wchar_t* Sender, const wchar_t* Msg, const wchar_t*  Info, const wchar_t*  FileName) override;
    [[nodiscard]] std::vector<LogEntry>::const_iterator begin() const;
    [[nodiscard]] std::vector<LogEntry>::const_iterator end() const;
private:
    std::vector<LogEntry> entries_;
    std::vector<Listener*> listeners_;
    std::mutex entriesMutex_;
    static thread_local bool insideWriteFunction_;
};

#endif
