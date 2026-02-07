
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <optional>

#include "Core/Upload/UploadEngine.h"
#include "Core/Utils/CoreTypes.h"

class CUploadEngineData;
class CMyEngineList;
class UploadEngineManager;

struct ServerFilter {
    std::string query;
    int64_t fileSize = 0;
    int typeMask = CUploadEngineListBase::ALL_SERVERS;

    bool empty() const {
        return false;
    }
};

class ServerData {
public:
    uint32_t color;
    std::string data;
    CUploadEngineData* ued {};
    CMyEngineList* engineList {};
    int uedIndex = -1;

    std::string getFormats() const;
    int64_t getMaxFileSize() const;
    std::string getMaxFileSizeString() const;
    std::string getServerDisplayName() const;
    std::string getStorageTimeString() const;
    std::string getAcccountStr() const;
    int getStorageTime() const;

    bool acceptFilter(const ServerFilter& filter) const;

private:
    mutable std::optional<std::string> formats;
    mutable std::optional<std::string> maxFileSizeString;
    mutable std::optional<std::string> serverDisplayName;
    mutable std::optional<std::string> storageTimeStr;
    mutable std::optional<int> storageTime;

    void cacheStorageTime() const;
};


class ServerListModel {
public:
    enum TableColumn { tcServerName,
        tcMaxFileSize,
        tcStorageTime,
        tcAccount,
        tcFileFormats };
    ServerListModel(CMyEngineList* engineList);
    ~ServerListModel();
    void updateEngineList();
    std::string getItemText(int row, int column) const;
    uint32_t getItemColor(int row) const;
    size_t getCount() const;
    void notifyRowChanged(size_t row);
    void notifyCountChanged(size_t row);
    const ServerData& getDataByIndex(size_t row) const;
    void setRowChangedCallback(std::function<void(size_t)> callback);
    void setItemCountChangedCallback(std::function<void(size_t)> callback);
    void setIconsChangedCallback(std::function<void()> callback);
    void resetData();
    void applyFilter(const ServerFilter& filter);

protected:
    CMyEngineList* engineList_;
    std::vector<ServerData> items_;
    std::vector<size_t> filteredItemsIndexes_;
    std::function<void(size_t)> rowChangedCallback_;
    std::function<void(size_t)> itemCountChangedCallback_;
    std::function<void()> iconsChangedCallback_;
    ServerFilter filter_;
    DISALLOW_COPY_AND_ASSIGN(ServerListModel);
};

