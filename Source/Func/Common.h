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

#ifndef IU_FUNC_COMMON_H
#define IU_FUNC_COMMON_H

#include "atlheaders.h"
#include "Gui/Constants.h"

class CCmdLine;

struct CUrlListItem
{
    CString FileName;
    CString ImageUrl;
    CString ImageUrlShortened;
    CString ThumbUrl;
    CString ThumbUrlShortened;
    CString DownloadUrl;
    CString DownloadUrlShortened;
    CString ServerName;

    // Index in original file list
    int FileIndex = -1;

    CString getDownloadUrl(bool shortened = false) const {
        return (shortened && !DownloadUrlShortened.IsEmpty()) ? DownloadUrlShortened : DownloadUrl;
    }

    CString getImageUrl(bool shortened = false) const {
        return (shortened && !ImageUrlShortened.IsEmpty()) ? ImageUrlShortened : ImageUrl;
    }

    CString getThumbUrl(bool shortened = false) const {
        return (shortened && !ThumbUrlShortened.IsEmpty()) ? ThumbUrlShortened : ThumbUrl;
    }

    bool isNull() const
    {
        return ImageUrl.IsEmpty() && DownloadUrl.IsEmpty();
    }
};

extern CCmdLine CmdLine;

bool IULaunchCopy(CString params, const CAtlArray<CString> &files);
bool IULaunchCopy(CString additionalParams=_T(""));

void IU_RunElevated(CString params);

void DecodeString(LPCTSTR szSource, CString &Result, LPSTR code = "{DAb[]=_T('')+b/16;H3N SHJ");
void EncodeString(LPCTSTR szSource, CString &Result, LPSTR code = "{DAb[]=_T('')+b/16;H3N SHJ");
CString HotkeyToString(CString funcName, CString menuItemText);

void OpenDocumentation(HWND parent, const CString& file, const CString& id = {});
bool DisableWindowsPrintScreenKeyInterception();
typedef CAtlArray<CString> CStringList;
#endif
