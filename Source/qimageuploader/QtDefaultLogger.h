#ifndef QIU_QTDEFAULTLOGGER_H
#define QIU_QTDEFAULTLOGGER_H

#pragma once

#include "Core/Logging/Logger.h"
class LogWindow;

class QtDefaultLogger : public ILogger {
public:
    QtDefaultLogger(LogWindow* logWindow);
    void write(LogMsgType MsgType, const std::string&  Sender, const std::string&  Msg, const std::string&  Info, const std::string&  FileName, bool fromSink) override;
#ifdef _WIN32
    void write(LogMsgType MsgType, const wchar_t*  Sender, const wchar_t*   Msg, const wchar_t*  Info, const wchar_t*  FileName, bool fromSink) override;
#endif
protected:
    LogWindow* logWindow_;
};

#endif