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

#ifndef IU_FUNC_MY_ENGINE_LIST_H
#define IU_FUNC_MY_ENGINE_LIST_H

#include <future>
#include <mutex>

#include "atlheaders.h"
#include "Library.h"

#include "Core/UploadEngineList.h"

class CMyEngineList: public CUploadEngineList
{
    public:
        struct ServerIconCacheItem {
            HICON icon{};
            HBITMAP bm{};

            ServerIconCacheItem() = default;

            ServerIconCacheItem(HICON i, HBITMAP b): icon(i), bm(b) {

            }
        };
        CMyEngineList() = default;

        CString errorStr() const;

        using CUploadEngineListBase::byName;
        const CUploadEngineData* byName(const CString &name) const;

        int getUploadEngineIndex(const CString& Name) const;
        bool loadFromFile(const CString& filename);

        static char DefaultServer[];
        static char RandomServer[];
    private:
        CString m_ErrorStr;
        DISALLOW_COPY_AND_ASSIGN(CMyEngineList);
};
#endif
