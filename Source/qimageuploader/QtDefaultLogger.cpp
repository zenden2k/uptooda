#include "QtDefaultLogger.h"

#include "Gui/LogWindow.h"
#include "Core/CommonDefs.h"

QtDefaultLogger::QtDefaultLogger(LogWindow* logWindow) {
    logWindow_ = logWindow;
}

void QtDefaultLogger::write(LogMsgType MsgType, const std::string& Sender, const std::string& Msg,
                            const std::string& Info, const std::string&  FileName, bool fromSink) {
    logWindow_->writeLog(MsgType, U2Q(Sender), U2Q(Msg), U2Q(Info));
}

#ifdef _WIN32
void QtDefaultLogger::write(LogMsgType msgType, const wchar_t* sender, const wchar_t* msg, const wchar_t* info, const wchar_t*  fileName, bool fromSink) {
    logWindow_->writeLog(msgType, QString::fromWCharArray(sender), QString::fromWCharArray(msg),
                         QString::fromWCharArray(info));
}
#endif