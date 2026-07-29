
#pragma once

#include <string>
#include <vector>
#include <functional>
#include <memory>
#include <optional>

#include "Core/Upload/UploadEngine.h"
#include "Core/Utils/CoreTypes.h"

class IFavoriteServers;
class CUploadEngineData;
class CMyEngineList;
class UploadEngineManager;

struct ServerFilter {
    std::string query;
    std::optional<int64_t> fileSize;
    std::optional<int> typeMask = CUploadEngineListBase::ALL_SERVERS;
    bool showFavoritesOnly = false;
    bool hideBlacklisted = true;

    bool empty() const {
        return query.empty() && !fileSize.has_value() && !typeMask.has_value() && !showFavoritesOnly && !hideBlacklisted;
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
    std::string getAccountStr() const;
    int getStorageTime() const;

    bool acceptFilter(const ServerFilter& filter, IFavoriteServers* favoriteServers) const;

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
    ServerListModel(CMyEngineList* engineList, IFavoriteServers* favoriteServers);
    void updateEngineList();
    std::string getItemText(int row, int column) const;
    uint32_t getItemColor(int row) const;
    size_t getCount() const;
    void notifyRowChanged(size_t row);
    void notifyCountChanged(size_t row);
    std::shared_ptr<ServerData> getDataByIndex(size_t row) const;
    std::optional<size_t> getIndexByServerName(const std::string& serverName) const;
    void setRowChangedCallback(std::function<void(size_t)> callback);
    void setItemCountChangedCallback(std::function<void(size_t)> callback);
    void setIconsChangedCallback(std::function<void()> callback);
    void resetData();
    void applyFilter(const ServerFilter& filter);

protected:
    CMyEngineList* engineList_;
    std::vector<std::shared_ptr<ServerData>> items_;
    std::optional<std::vector<size_t>> filteredItemsIndexes_;
    std::function<void(size_t)> rowChangedCallback_;
    std::function<void(size_t)> itemCountChangedCallback_;
    std::function<void()> iconsChangedCallback_;
    ServerFilter filter_;
    IFavoriteServers* favoriteServers_;
    DISALLOW_COPY_AND_ASSIGN(ServerListModel);
};

