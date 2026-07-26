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
void QtDefaultLogger::write(LogMsgType MsgType, const wchar_t* Sender, const wchar_t* Msg, const wchar_t* Info, const wchar_t*  FileName, bool fromSink) {
    logWindow_->writeLog(MsgType, QString::fromWCharArray(Sender), QString::fromWCharArray(Msg),
                         QString::fromWCharArray(Info));
}
#endif