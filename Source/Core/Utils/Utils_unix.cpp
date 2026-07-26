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

#include "CoreUtils.h"

#include <stdio.h>
#include <errno.h>

#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */
#include <sys/stat.h>
#include <sstream>  // ostringstream
#include <iostream>
#include <string>
#include <boost/filesystem.hpp>
#include "Core/3rdpart/utf8.h"
#include "Core/Utils/StringUtils.h"

typedef struct stat Stat;

namespace IuCoreUtils {

std::wstring Utf8ToWstring(const std::string &str) {
    using namespace utf8;
    std::wstring res;
    try {
        if (sizeof(wchar_t) == 2)
            utf8to16(str.begin(), str.end(), back_inserter(res));
        else if (sizeof(wchar_t) == 4)
            utf8to32(str.begin(), str.end(), back_inserter(res));
    }
    catch(...) { }
    return res;
}

std::string WstringToUtf8(const std::wstring &str) {
    using namespace utf8;
    std::string res;
    try {
      if (sizeof(wchar_t) == 2)
         utf16to8(str.begin(), str.end(), back_inserter(res));
      else if (sizeof(wchar_t) == 4)
          utf32to8(str.begin(), str.end(), back_inserter(res));
    }
    catch(...) { }
    return res;
}

std::string Utf16ToUtf8(const std::u16string& src) {
    using namespace utf8;
    std::string res;
    try {
        utf16to8(src.begin(), src.end(), back_inserter(res));
    } catch (...) {
    }
    return res;
}

std::string ConvertToUtf8(const std::string &text, const std::string& codePage) {
    // FIXME: stub
    return text;
}

std::string WstringToSystemLocale(const std::wstring &str) {
    // FIXME
    return WstringToUtf8(str);
}

bool MoveFileOrFolder(const std::string& from, const std::string& to) {
    return rename(from.c_str() ,to.c_str())==0;
}

}
