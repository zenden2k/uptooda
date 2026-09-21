/*

    Uptooda - free application for uploading images/files to the Internet

    Copyright 2007-2025 Sergey Svistunov (zenden2k@gmail.com)

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

*/

#ifndef IU_COMMONTYPES_H
#define IU_COMMONTYPES_H

#include "Core/Utils/CoreUtils.h"

class CUploadEngineData;

struct InfoProgress {
    int64_t Uploaded, Total;
    bool IsUploading;
    void clear()
    {
        Uploaded = 0;
        Total = 0;
        IsUploading = false;
    }
};

enum StatusType {
    stNone = 0, stUploading, stWaitingAnswer,  stAuthorization, stPerformingAction, stCreatingFolder,
    stUserDescription
};

enum ErrorType {
    etNone, etOther, etRepeating, etRetriesLimitReached, etActionRepeating, etActionRetriesLimitReached,
    etRegularExpressionError, etNetworkError, etUserError
};

struct ErrorInfo
{
    enum MessageType {
        mtError, mtWarning, mtInformation
    };
    std::string error;
    std::string Url;
    const CUploadEngineData* uploadEngineData = nullptr;
    std::string ServerName;
    std::string FileName;
    std::string TopLevelFileName;
    int ActionIndex;
    MessageType messageType;
    ErrorType errorType;
    int RetryIndex;
    std::string sender;
    std::string Script;
    std::thread::id ThreadId;

    ErrorInfo()
    {
        RetryIndex = -1;
        ActionIndex = -1;
        messageType = mtError;
        errorType = etNone;
    }

    void Clear()
    {
        error.clear();
        Url.clear();
        ServerName.clear();
        FileName.clear();
        sender.clear();
        Script.clear();
        ActionIndex = -1;
        // messageType = mtNone;
        errorType = etNone;
        RetryIndex = -1;
    }
};
#endif
