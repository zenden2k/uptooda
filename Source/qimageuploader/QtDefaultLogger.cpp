#include "QtDefaultLogger.h"

#include "Gui/LogWindow.h"
#include "Core/CommonDefs.h"

QtDefaultLogger::QtDefaultLogger(LogWindow* logWindow) {
    logWindow_ = logWindow;
}

void QtDefaultLogger::write(LogMsgType msgType, const std::string& sender, const std::string& msg,
                            const std::string& info, const std::string&  fileName, bool fromSink) {
    logWindow_->writeLog(msgType, U2Q(sender), U2Q(msg), U2Q(info));
}

#ifdef _WIN32
void QtDefaultLogger::write(LogMsgType msgType, const wchar_t* sender, const wchar_t* msg, const wchar_t* info, const wchar_t*  fileName, bool fromSink) {
    logWindow_->writeLog(msgType, QString::fromWCharArray(sender), QString::fromWCharArray(msg),
                         QString::fromWCharArray(info));
}
#endif