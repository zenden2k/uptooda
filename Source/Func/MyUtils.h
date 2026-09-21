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

#ifndef IU_FUNC_MYUTILS_H
#define IU_FUNC_MYUTILS_H

#include <set>
#include "atlheaders.h"
#include <tchar.h>
#include "Core/Video/VideoUtils.h"

#define VIDEO_FORMATS _T("avi\0mpg\0mpeg\0vob\0divx\0flv\0wmv\0asf\0mkv\0mov\0ts\0mp2\0mp4\0") _T("3gp\0rm\0mpeg2ts\0\0")

#define LOADICO(ico) LoadIcon(GetModuleHandle(0), MAKEINTRESOURCE(ico))

bool IsFileOfType(LPCWSTR szFileName, const std::set<std::string>& extensionsSet);


inline bool IsVideoFile(LPCWSTR szFileName)
{
    return IsFileOfType(szFileName, VideoUtils::videoFilesExtensions);
}

inline bool IsAudioFile(LPCWSTR szFileName)
{
    return IsFileOfType(szFileName, VideoUtils::audioFilesExtensions);
}

CString PrepareVideoDialogFilters();
CString PrepareAudioDialogFilters();

#endif


