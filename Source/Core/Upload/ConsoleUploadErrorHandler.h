#ifndef IU_CORE_UPLOAD_CONSOLEUPLOADERRORHANDLER_H
#define IU_CORE_UPLOAD_CONSOLEUPLOADERRORHANDLER_H

#pragma once

#include "Core/Upload/UploadErrorHandler.h"

class CUploadEngineListBase;

class ConsoleUploadErrorHandler : public IUploadErrorHandler {
public:
    ConsoleUploadErrorHandler() = default;
    void ErrorMessage(const ErrorInfo& errorInfo) override;
    void DebugMessage(const std::string& msg, bool isResponseBody) override;

private:
    CUploadEngineListBase* engineList_;
};

#endif
