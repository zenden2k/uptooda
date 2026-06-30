#include "ServerListModel.h"

#include "Func/MyEngineList.h"
#include "Core/i18n/Translator.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "Core/Utils/StringUtils.h"
#include "Gui/Interfaces/IFavoriteServers.h"

namespace {

size_t StringSearch(const std::string& str1, const std::string& str2) {
    auto loc = std::locale();
    auto it = std::search(str1.begin(), str1.end(),
        str2.begin(), str2.end(), [&loc](char ch1, char ch2) -> bool {
            return std::toupper(ch1, loc) == std::toupper(ch2, loc);
        });
    if (it != str1.end()) {
        return it - str1.begin();
    } else {
        return std::string::npos;
    }
}

}

ServerListModel::ServerListModel(CMyEngineList* engineList, IFavoriteServers* favoriteServers) : engineList_(engineList), favoriteServers_(favoriteServers) {
    updateEngineList();
}

ServerListModel::~ServerListModel() {
}

void ServerListModel::updateEngineList() {
    filteredItemsIndexes_.clear();
    items_.clear();

    for (int i = 0; i < engineList_->count(); i++) {
        CUploadEngineData* ued = engineList_->byIndex(i);

        ServerData sd;
        sd.ued = ued;
        sd.uedIndex = i;
        sd.engineList = engineList_;

        items_.push_back(std::move(sd));
    }
    if (iconsChangedCallback_) {
        iconsChangedCallback_();
    }
}

std::string ServerListModel::getItemText(int row, int column) const {
    const ServerData& serverData = getDataByIndex(row);
    if (column == tcServerName) {
        return serverData.getServerDisplayName();
    }
    if (column == tcMaxFileSize) {
        return serverData.getMaxFileSizeString();
    }
    if (column == tcStorageTime) {
        return serverData.getStorageTimeString();
    }
    if (column == tcAccount) {
        return serverData.getAcccountStr();
    }
    if (column == tcFileFormats) {
        return serverData.getFormats();
    } 
    return {};
}

uint32_t ServerListModel::getItemColor(int row) const {
    const ServerData& serverData = getDataByIndex(row);
    std::string name = serverData.ued->Name;
    if (favoriteServers_->isServerFavorite(name)) {
        return RGB(56,176, 0); // Green
    }

    return GetSysColor(COLOR_WINDOWTEXT);
}

size_t ServerListModel::getCount() const {
    if (filter_.empty()) {
        return items_.size();
    }
    return filteredItemsIndexes_.size();
}

void ServerListModel::notifyRowChanged(size_t row) {
    if (row < items_.size() && rowChangedCallback_) {
        rowChangedCallback_(row);
    }
}

const ServerData& ServerListModel::getDataByIndex(size_t row) const {
    if (!filter_.empty()) {
        row = filteredItemsIndexes_[row];
    }
    return items_[row];
}

void ServerListModel::setRowChangedCallback(std::function<void(size_t)> callback) {
    rowChangedCallback_ = std::move(callback);
}

void ServerListModel::setItemCountChangedCallback(std::function<void(size_t)> callback) {
    itemCountChangedCallback_ = std::move(callback);
}

void ServerListModel::setIconsChangedCallback(std::function<void()> callback) {
    iconsChangedCallback_ = callback;
}

void ServerListModel::resetData() {
    /* for (auto& it : items_) {
        it.clearInfo();
    }*/
}


void ServerListModel::applyFilter(const ServerFilter& filter) {
    filter_ = filter;
    filteredItemsIndexes_.clear();
    size_t i = 0;
    for (const auto& item : items_) {
        if (item.acceptFilter(filter_)) {
            filteredItemsIndexes_.push_back(i);
        }
        i++;
    }
    notifyCountChanged(getCount());
}

void ServerListModel::notifyCountChanged(size_t row) {
    if (itemCountChangedCallback_) {
        itemCountChangedCallback_(row);
    }
}

std::string ServerData::getFormats() const {
    if (!formats.has_value()) {
        std::string result;
        std::vector<std::set<std::string>> extensions;
        extensions.resize(ued->userTypes.size());

        for (const auto& formatGroup : ued->SupportedFormatGroups) {
            if (!formatGroup.Extensions.empty()) {
                const auto& userTypeIds = formatGroup.UserTypeIds.empty() ? ued->getUserTypesIds() : formatGroup.UserTypeIds;
                for (auto userTypeId : userTypeIds) {
                    if (userTypeId < extensions.size()) {
                        extensions[userTypeId].insert(formatGroup.Extensions.begin(), formatGroup.Extensions.end());
                    }
                }
            }
        }

        while (!extensions.empty() && extensions.back().empty()) {
            extensions.pop_back();
        }

        for (const auto& v : extensions) {
            if (!result.empty()) {
                result += "/ ";
            }
            if (!v.empty()) {
                result += IuStringUtils::Join(v, ",");
                result += " ";
            }
        }   
        
        formats = result;
    }

    return *formats;
}

int64_t ServerData::getMaxFileSize() const {
    return ued->SupportedFormatGroups.empty() ? ued->MaxFileSize : ued->SupportedFormatGroups[0].MaxFileSize;
}

std::string ServerData::getMaxFileSizeString() const {
    if (!maxFileSizeString.has_value()) {
        std::string result;
        std::vector<std::optional<int64_t>> fileSizes;
        fileSizes.resize(ued->userTypes.size());

        for (const auto& formatGroup : ued->SupportedFormatGroups) {
            if (formatGroup.MaxFileSize == 0) {
                continue;
            }
            const auto& userTypeIds = formatGroup.UserTypeIds.empty() ? ued->getUserTypesIds() : formatGroup.UserTypeIds;
            for (auto userTypeId : userTypeIds) {
                if (userTypeId >= fileSizes.size()) {
                    continue;
                }

                if (!fileSizes[userTypeId].has_value() || formatGroup.MaxFileSize == CUploadEngineData::MAX_FILE_SIZE_UNLIMITED || formatGroup.MaxFileSize > *fileSizes[userTypeId]) {
                    fileSizes[userTypeId] = formatGroup.MaxFileSize;
                }
            }    
        }

        while (!fileSizes.empty() && !fileSizes.back().has_value()) {
            fileSizes.pop_back();
        }

        int valueCount = 0;

        for (auto fileSize : fileSizes) {
            if (!result.empty()) {
                result += "/ ";
            }
            if (fileSize.has_value()) {
                result += fileSize == CUploadEngineData::MAX_FILE_SIZE_UNLIMITED ? u8"\u221E" : IuCoreUtils::FileSizeToString(*fileSize);
                valueCount++;
            } else {
                result += "-";
            }
            result += " ";
        }

        if (result.empty() && ued->MaxFileSize != 0) {
            result += ued->MaxFileSize == CUploadEngineData::MAX_FILE_SIZE_UNLIMITED ? u8"\u221E" : IuCoreUtils::FileSizeToString(ued->MaxFileSize);
            valueCount++;
        }
        maxFileSizeString = valueCount ? result : "";
    }
    return *maxFileSizeString;
}


std::string ServerData::getServerDisplayName() const {
    if (!serverDisplayName.has_value()) {
        serverDisplayName = engineList->getServerDisplayName(ued);
    }

    return *serverDisplayName;
}

std::string ServerData::getStorageTimeString() const {
    cacheStorageTime();
    return *storageTimeStr;
}


std::string ServerData::getAcccountStr() const {
    switch (ued->NeedAuthorization) {
        case CUploadEngineData::naNotAvailable:
            return "-";
        case CUploadEngineData::naAvailable:
            return "+";
        case CUploadEngineData::naObligatory:
            return _c("serverlist.account", "required");
    }
    return {};
}

int ServerData::getStorageTime() const {
    cacheStorageTime();
    return *storageTime;
}

bool ServerData::acceptFilter(const ServerFilter& filter) const {
    if (!filter.query.empty()) {
        if (StringSearch(getServerDisplayName(), filter.query) == std::string::npos) {
            return false;
        }
    }

    return (ued->TypeMask & filter.typeMask) != 0;
}

void ServerData::cacheStorageTime() const {
    if (!storageTimeStr.has_value()) {
        std::string daysStr;

        std::vector<std::optional<StorageTime>> storageTimes;
        storageTimes.resize(ued->userTypes.size());

        for (const auto& item : ued->StorageTimeInfo) {
            const auto& userTypeIds = item.UserTypeIds.empty() ? ued->getUserTypesIds() : item.UserTypeIds;
            for (auto userTypeId : userTypeIds) {
                if (userTypeId < storageTimes.size()) {
                    storageTimes[userTypeId] = item;
                }
            }
        }

        while (!storageTimes.empty() && !storageTimes.back().has_value()) {
            storageTimes.pop_back();
        }

        int i = 0;
        int valueCount = 0;
        for (const auto& item : storageTimes) {
            if (!daysStr.empty()) {
                daysStr += "/ ";
            }
            if (item.has_value()) {
                if (item->Time) {
                    if (!storageTime.has_value() && (i == 0 || i == 1)) {
                        storageTime = item->Time;
                    }
                    daysStr += item->Time == StorageTime::TIME_INFINITE ? u8"\u221E" : std::to_string(item->Time);
                    if (item->AfterLastDownload) {
                        daysStr += u8"\u2913";
                    }
                    valueCount++;
                } else {
                    daysStr += "?";
                }
            } else {
                daysStr += "-";
            }

            daysStr += " ";
            i++;
        }

        if (!storageTime.has_value()) {
            storageTime = 0;
        }
       
        storageTimeStr = valueCount ? daysStr : "";
    }
}
