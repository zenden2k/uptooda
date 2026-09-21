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

#include "Logging.h"
#include "Core/Utils/CoreUtils.h"

std::ostream& operator<<(std::ostream& out, const wchar_t* str) {
    std::string msg = IuCoreUtils::WstringToUtf8(str);
    out << msg;
    return out;
}

std::ostream& operator<<(std::ostream& out, const std::wstring& str) {
    std::string msg = IuCoreUtils::WstringToUtf8(str);

    return operator<<(out, msg);
}

#ifdef _WIN32
std::ostream& operator<<(std::ostream& out, const RECT& rc) {
    char buffer[100];
    sprintf(buffer, "%ld %ld %ld %ld", rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
    return operator<<(out, buffer);
}

#endif
