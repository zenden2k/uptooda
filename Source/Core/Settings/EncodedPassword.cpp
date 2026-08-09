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

#include "EncodedPassword.h"

#include "Core/Utils/CoreUtils.h"
#include <wchar.h>

CEncodedPassword::CEncodedPassword() {
}

CEncodedPassword::CEncodedPassword(const std::string& d) {
    data_ = d;
}

std::string CEncodedPassword::toEncodedData() const {
    std::string res;
    EncodeString(data_, res);
    return res;
}
/*
CEncodedPassword::operator const wchar_t*() {
    return data_;
}*/

#ifdef _WIN32
CEncodedPassword& CEncodedPassword::operator=(const CString& text) {
    data_ = IuCoreUtils::WstringToUtf8(static_cast<LPCTSTR>(text));
    return *this;
}
#endif

void CEncodedPassword::DecodeString(const std::string& encodedString, std::string & Result, const char* code) {
    std::wstring szSource = IuCoreUtils::Utf8ToWstring(encodedString);
    wchar_t szDestination[1024];
    int br = strlen(code);
    int n = szSource.length() / 2;
    int j = 0;
    memset(szDestination, 0, sizeof(szDestination));

    int i;
    uint8_t* data = (uint8_t*)szDestination;
    *szDestination = 0;

    for (i = 0; i < n; i++) {
        if (j >= br)
            j = 0;

        uint8_t b;
        b = static_cast<uint8_t>((szSource[i * 2] - L'A') * 16 + (szSource[i * 2 + 1] - L'A'));
        b = b ^ code[j];
        data[i] = b;
        j++;
    }
    data[i] = 0;
    Result = IuCoreUtils::WstringToUtf8(szDestination);
}

void CEncodedPassword::EncodeString(const std::string& plainText, std::string& Result, const char* code)
{
    std::wstring szSource = IuCoreUtils::Utf8ToWstring(plainText);
    wchar_t szDestination[1024];
    int br = strlen(code);
    int n = szSource.length() * 2;
    int j = 0;

    auto* data = reinterpret_cast<uint8_t*>(&szSource[0]);
    *szDestination = 0;
    for (int i = 0; i < n; i++) {
        if (j >= br)
            j = 0;

        uint8_t b;
        b = data[i] ^ code[j];
        wchar_t bb[2] = { 0, 0 };
        bb[0] = L'A' + b / 16;
        wcscat(szDestination, bb);
        bb[0] = L'A' + b % 16;
        wcscat(szDestination, bb);
        j++;
    }
    Result = IuCoreUtils::WstringToUtf8(szDestination);
}
