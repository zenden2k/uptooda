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

#include "Func/MyEngineList.h"

#include "Core/Settings/BasicSettings.h"
#include "Func/IuCommonFunctions.h"
#include "Func/WinUtils.h"
#include "Core/ServiceLocator.h"

char CMyEngineList::DefaultServer[] = "default";

char CMyEngineList::RandomServer[]  = "random";

const CUploadEngineData* CMyEngineList::byName(const CString& name) const
{
    return CUploadEngineListBase::byName(WCstringToUtf8(name));
}

int CMyEngineList::getUploadEngineIndex(const CString& Name) const
{
    return CUploadEngineListBase::getUploadEngineIndex(WCstringToUtf8(Name));
}

CString CMyEngineList::errorStr() const
{
    return m_ErrorStr;
}

bool CMyEngineList::loadFromFile(const CString& filename)
{
    std::string fileNameU8 = W2U(filename);
    if (!IuCoreUtils::FileExists(fileNameU8)) {
        m_ErrorStr = "File not found.";
        return false;
    }
    BasicSettings* Settings = ServiceLocator::instance()->basicSettings();
    return CUploadEngineList::loadFromFile(fileNameU8, Settings->ServersSettings);
}
