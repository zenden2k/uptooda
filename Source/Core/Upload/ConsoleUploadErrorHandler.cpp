#include "ConsoleUploadErrorHandler.h"

#include <iostream>

#include "Core/Upload/UploadEngine.h"

void ConsoleUploadErrorHandler::ErrorMessage(const ErrorInfo& errorInfo) {
    std::string errorMsg;

    std::string infoText;
    if (!errorInfo.FileName.empty())
        infoText += "File: " + errorInfo.FileName + "\n";

    if (!errorInfo.ServerName.empty()) {
        std::string serverName = errorInfo.uploadEngineData ? CUploadEngineListBase::getServerDisplayName(errorInfo.uploadEngineData) : errorInfo.ServerName;
        if (!errorInfo.sender.empty())
            serverName += "(" + errorInfo.sender + ")";
        infoText += "Server: " + serverName + "\n";
    }

    if (!errorInfo.Url.empty())
        infoText += "URL: " + errorInfo.Url + "\n";


    if (errorInfo.ActionIndex != -1)
        infoText += "Action: #" + std::to_string(errorInfo.ActionIndex);


    if (!errorInfo.error.empty()) {
        errorMsg += errorInfo.error;

    }
    else {
        if (errorInfo.errorType == etRepeating) {
            char buf[256];
            sprintf(buf, "Upload failed. Another retry (%d)", errorInfo.RetryIndex);
            errorMsg = buf;
        }
        else if (errorInfo.errorType == etRetriesLimitReached) {
            errorMsg = "Upload failed! (retry limit reached)";
        }
    }

    std::cerr << std::endl << IuCoreUtils::Utf8ToSystemLocale(infoText) << std::endl;
    std::cerr << IuCoreUtils::Utf8ToSystemLocale(errorMsg) << std::endl;
    std::cerr << "---------------------" << std::endl;
}

void ConsoleUploadErrorHandler::DebugMessage(const std::string& msg, bool isResponseBody) {
#ifdef _WIN32
    std::wcerr << IuCoreUtils::Utf8ToWstring(msg) << std::endl;
#else
    std::cerr << IuCoreUtils::Utf8ToSystemLocale(msg) << std::endl;
#endif
}
