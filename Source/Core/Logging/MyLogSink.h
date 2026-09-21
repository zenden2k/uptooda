#ifndef IU_CORE_MYLOGSINK_H
#define IU_CORE_MYLOGSINK_H

#pragma once

#include "Core/Utils/CoreTypes.h"
#include "Core/Logging.h"
#include "Core/Logging/Logger.h"

class MyLogSink: public google::LogSink {
public:
    MyLogSink(ILogger* logger);
    void send(google::LogSeverity severity, const char* full_filename,
                      const char* base_filename, int line,
                      const google::LogMessageTime& time, const char* message,
                      size_t message_len) override;

protected:
    DISALLOW_COPY_AND_ASSIGN(MyLogSink);
    ILogger* logger_;
    static thread_local bool insideSendFunction_;
}; 

#endif
