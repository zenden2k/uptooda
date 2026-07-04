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

#include "MyUtils.h"

#include <set>
#include <string>

#include "Func/WinUtils.h"
#include "Core/Utils/CoreUtils.h"
#include "Core/Utils/StringUtils.h"
#include "Core/Video/VideoUtils.h"

bool IsFileOfType(LPCWSTR szFileName, const std::set<std::string>& extensionsSet) {
    if (!szFileName) {
        return false;
    }
    CString extension = WinUtils::GetFileExt(szFileName);

    if (extension.IsEmpty()) {
        return false;
    }

    extension.MakeLower();
    std::string extensionA(CW2A(extension, CP_UTF8));
    return extensionsSet.find(extensionA) != extensionsSet.end();
}

CString PrepareVideoDialogFilters() {
    CString result;
    for (const auto& ex : VideoUtils::instance().videoFilesExtensionsSet) {
        result += _T("*.");
        result += ex.c_str();
        result += _T(";");
    }
    return result;
}

CString PrepareAudioDialogFilters() {
    CString result;
    for (const auto& ex : VideoUtils::instance().audioFilesExtensionsSet) {
        result += _T("*.");
        result += ex.c_str();
        result += _T(";");
    }
    return result;
}