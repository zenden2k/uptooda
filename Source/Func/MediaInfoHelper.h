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


#ifndef IU_FUNC_FILEINFOHELPER_H
#define IU_FUNC_FILEINFOHELPER_H
#pragma once

#include "atlheaders.h"

namespace MediaInfoHelper {

bool FindMediaInfoDllPath();
bool IsMediaInfoAvailable();
bool GetMediaFileInfo(LPCWSTR fileName, CString &buffer, CString& fullInfo, bool enableLocalization = true);
CString GetLibraryVersion();
CString GetLibraryPath();

}

#endif  // IU_FUNC_FILEINFOHELPER_H
