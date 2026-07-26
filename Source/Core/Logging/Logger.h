#ifndef IU_CORE_LOGGIN_LOGGER_H
#define IU_CORE_LOGGIN_LOGGER_H

#pragma once
#include <string>

class ILogger {
public:
    enum LogMsgType { logError = 1, logWarning, logInformation };
    virtual ~ILogger(){};
    virtual void write(LogMsgType MsgType, const std::string&  Sender, const std::string&  Msg, const std::string&  Info = "", const std::string&  FileName = "", bool fromSink = false) = 0;
#ifdef _WIN32
    virtual void write(LogMsgType MsgType, const wchar_t*  Sender, const wchar_t*   Msg, const wchar_t*  Info = L"", const wchar_t*  FileName = L"", bool fromSink = false) = 0;
#endif
};

#endif
