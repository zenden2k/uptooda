#include "MyLogSink.h"

#include "Core/Utils/CoreUtils.h"

thread_local bool MyLogSink::insideSendFunction_  = false;

MyLogSink::MyLogSink(ILogger* logger) {
    logger_ = logger;
}

void MyLogSink::send(google::LogSeverity severity, const char* full_filename, const char* base_filename, int line,
                      const google::LogMessageTime& time, const char* message, size_t message_len)
{
    if (insideSendFunction_) { // Prevent recursion
        return;
    }
    insideSendFunction_ = true;
    defer d([&] { // Run at function exit
        insideSendFunction_ = false;
    });
    std::string sender = base_filename;
    sender += ":" + std::to_string(line);
    std::string msg(message, message_len);

    ILogger::LogMsgType msgType;

    switch (severity) {
        case google::GLOG_ERROR:
            msgType = ILogger::logError;
            break;
        case google::GLOG_INFO:
            msgType = ILogger::logInformation;
            break;
        case google::GLOG_WARNING:
        default:
            msgType = ILogger::logWarning;
    }
    logger_->write(msgType, sender, msg, {}, {}, true);
}

