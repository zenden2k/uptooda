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
#include "CmdLine.h"

#include <ShellAPI.h>
#include "Func/WinUtils.h"

CCmdLine::CCmdLine()
{
    Parse(GetCommandLine());
}

CCmdLine::CCmdLine(LPCTSTR szCmdLine)
{
    Parse(szCmdLine);
}

void CCmdLine::Parse(LPCTSTR szCmdLine)
{
    params_.clear();
    onlyParams_.Empty();

    // If szCmdLine is an empty string the CommandLineToArgvW function returns the path to the current executable file.
    // but this is not what we need
    if (szCmdLine && *szCmdLine) {
        int argCount;

        LPWSTR* szArgList = CommandLineToArgvW(szCmdLine, &argCount);
        std::wstring cmdLine;

        for (int i = 0; i < argCount; i++) {
            AddParam(szArgList[i]);
            if (i) {
                WinUtils::ArgvQuote(szArgList[i], cmdLine, false);
                if (i < argCount - 1) {
                    cmdLine.push_back(L' ');
                }
            }
        }

        onlyParams_ = cmdLine.c_str();

        LocalFree(szArgList);
    } else {
        params_.emplace_back(_T(""));
    }
}

size_t CCmdLine::AddParam(LPCTSTR szParam)
{
    params_.emplace_back(szParam);
    return params_.size() - 1;
}

CString CCmdLine::operator[](size_t nIndex) const
{
    if (nIndex + 1 > params_.size() ) {
        return _T("");
    }
    return params_[nIndex];
}

CString CCmdLine::OnlyParams() const
{
    return onlyParams_;
}

CString CCmdLine::ModuleName() const
{
    return params_.empty() ? CString(_T("")): params_[0];
}

bool CCmdLine::GetNextFile(CString& FileName, size_t& nIndex) const
{
    for (size_t i = nIndex + 1; i < params_.size(); i++)
    {
        if (params_[i].GetLength() && params_[i][0] != _T('/'))
        {
            FileName = params_[i];
            nIndex = i;
            return true;
        }
    }

    return false;
}

CCmdLine &CCmdLine::operator=(const CCmdLine& p)
{
    if (this != &p) {
        params_ = p.params_;
        onlyParams_ = p.onlyParams_;
    }
    return *this;
}

bool CCmdLine::IsOption(LPCTSTR Option, bool bUsePrefix) const
{
    CString Temp;
    if (bUsePrefix)
        Temp = CString("/");
    Temp += Option;
    for (size_t i = 1; i < params_.size(); i++)
    {
        if (lstrcmpi(params_[i], Temp) == 0)
            return true;
    }

    return false;
}

bool CCmdLine::RemoveOption(LPCTSTR option) {
    CString temp = CString(_T("/")) + option;
    bool res = false;
    for (auto it = params_.begin(); it != params_.end();) {
        if (lstrcmpi(*it, temp) == 0) {
            it = params_.erase(it);
            res = true;
        } else {
            ++it;
        }
    }
    return res;
}

size_t CCmdLine::GetCount() const
{
    return params_.size();
}

std::vector<CString>::const_iterator CCmdLine::begin() const {
    return params_.begin();
}

std::vector<CString>::const_iterator CCmdLine::end() const {
    return params_.end();
}

CCmdLine CmdLine;
