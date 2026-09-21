#include "ConsoleLogger.h"

#include <iostream>
#include <mutex>

#include "Core/Utils/CoreUtils.h"
#include "Core/Utils/ConsoleUtils.h"

void ConsoleLogger::write(LogMsgType MsgType, const std::string& Sender, const std::string& Msg, const std::string& Info, const std::string& FileName, bool fromSink) {
    if (fromSink) {
        return;
    }
    std::lock_guard<std::mutex> guard(ConsoleUtils::instance()->getOutputMutex());
//#ifdef _WIN32
    std::wcerr << std::endl << IuCoreUtils::Utf8ToWstring(Msg) << std::endl;
/*#else
    std::cerr << IuCoreUtils::Utf8ToSystemLocale(Msg) << std::endl;
#endif*/
}

#ifdef _WIN32
void ConsoleLogger::write(LogMsgType MsgType, const wchar_t* Sender, const wchar_t* Msg, const wchar_t* Info, const wchar_t* FileName, bool fromSink = false) {
    if (fromSink) {
        return;
    }
    std::lock_guard<std::mutex> guard(ConsoleUtils::instance()->getOutputMutex());
    std::cerr << ( MsgType == logError ? "error" : "warning" ) << " : ";
//#ifdef _WIN32
    std::wcerr << std::endl << Msg << std::endl;
/*#else
    std::cerr << IuCoreUtils::Utf8ToSystemLocale(IuCoreUtils::WstringToUtf8(Msg)) << std::endl;
#endif*/
}

#endif
