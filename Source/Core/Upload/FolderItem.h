#pragma once

#include <string>
#include <vector>

/**
CFolderItem class
*/
class CFolderItem {
public:
    enum ItemCount {
        icUnknown = -1,
        icNoChildren = 0
    };

    CFolderItem() {
        accessType = 0;
        itemCount = icUnknown;
    }

    /*! @cond PRIVATE */
    inline static const std::string NewFolderMark = "{B94534ED-CE67-478B-A757-8D452CB70F0E}";

    std::string title;
    std::string summary;
    std::string id;
    std::string parentId;
    std::string viewUrl;
    std::vector<std::string> parentIds;

    int accessType;
    int itemCount;
    /*! @endcond */
    std::string getTitle() const { return title; }
    std::string getSummary() const { return summary; }
    std::string getId() const { return id; }
    std::string getParentId() const { return parentId; }
    int getItemCount() const { return itemCount; }
    int getAccessType() const { return accessType; }
    std::string getViewUrl() const { return viewUrl; }
    void setTitle(const std::string& str) { title = str; }
    void setViewUrl(const std::string& str) { viewUrl = str; }
    void setSummary(const std::string& str) { summary = str; }
    void setId(const std::string& str) { id = str; }
    void setParentId(const std::string& str) { parentId = str; }
    void setAccessType(const int type) { accessType = type; }
    void setItemCount(const int count) { itemCount = count; }
};
