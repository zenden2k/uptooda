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

void CEncodedPassword::DecodeString(const std::string& encodedString, std::string& Result, const char* code) {
    const size_t byteCount = encodedString.size() / 2;
    std::wstring decoded((byteCount + sizeof(wchar_t) - 1) / sizeof(wchar_t), L'\0');
    auto* bytes = reinterpret_cast<unsigned char*>(decoded.data());
    const size_t codeLength = strlen(code);

    for (size_t index = 0; index < byteCount; ++index) {
        const auto byte
            = static_cast<unsigned char>((encodedString[index * 2] - 'A') * 16 + (encodedString[index * 2 + 1] - 'A'));
        bytes[index] = byte ^ code[index % codeLength];
    }
    Result = IuCoreUtils::WstringToUtf8(decoded.c_str());
}

void CEncodedPassword::EncodeString(const std::string& plainText, std::string& Result, const char* code) {
    const std::wstring wideText = IuCoreUtils::Utf8ToWstring(plainText);
    const auto* bytes = reinterpret_cast<const unsigned char*>(wideText.data());
    const size_t codeLength = strlen(code);
    const size_t byteCount = wideText.size() * sizeof(wchar_t);
    Result.resize(byteCount * 2);

    for (size_t index = 0; index < byteCount; ++index) {
        const unsigned char byte = bytes[index] ^ code[index % codeLength];
        Result[index * 2] = static_cast<char>('A' + byte / 16);
        Result[index * 2 + 1] = static_cast<char>('A' + byte % 16);
    }
}
