#ifndef CORE_UPLOAD_FOLDERLIST_H
#define CORE_UPLOAD_FOLDERLIST_H
#pragma once

#include <string>
#include <vector>

#include "UploadEngine.h"


/**
CFolderList class
*/
class CFolderList
{
    public:
        /*! @cond PRIVATE */
        std::vector<CFolderItem> m_folderItems;
        CFolderItem parent_;
        /*! @endcond */

        /**
        @since 1.3.2
        */
        int GetCount() { return m_folderItems.size();}
        /*! @cond PRIVATE */
        CFolderItem& operator [] (size_t index) {  return m_folderItems[index]; }
        void Clear() { m_folderItems.clear(); }
        /*! @endcond */
        void AddFolder(const std::string& title, const std::string& summary, const std::string& id, const std::string& parentId, int accessType);
        void AddFolderItem(const CFolderItem& item);

        void setParentFolder(const CFolderItem& parent) {
            parent_ = parent;
        }

        const CFolderItem& parentFolder() const {
            return parent_;
        }
};

#endif
