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

#include "FolderList.h"

#include <string>

void CFolderList::AddFolder(const std::string& title, const std::string& summary, const std::string& id,
                            const std::string& parentId,
                            int accessType)
{
    CFolderItem ai;
    ai.title = title;
    ai.summary = summary;
    ai.id = id;
    ai.parentId =  parentId;
    ai.accessType = accessType;
    m_folderItems.push_back(ai);
}

void CFolderList::AddFolderItem(const CFolderItem& item)
{
    m_folderItems.push_back(item);
}
