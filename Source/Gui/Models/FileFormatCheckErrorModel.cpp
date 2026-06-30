#include "FileFormatCheckErrorModel.h"

#include "Func/MyEngineList.h"

FileFormatCheckErrorModel::FileFormatCheckErrorModel(IFileList* fileList, const std::vector<BadFileFormat>& errors)
    : fileList_(fileList)
{
    for (const auto& error: errors) {
        IFileListItem* item = fileList->getFile(error.index);
        if (!item) {
            continue;
        }
        
        auto sd = std::make_unique<FileFormatModelData>();
        sd->file = item;
        sd->justFileName = IuCoreUtils::ExtractFileName(item->getFileName());
        sd->badFileFormat = error;
        sd->extension = IuCoreUtils::ExtractFileExt(error.fileName);
        items_.push_back(std::move(sd));
    }
}

std::string FileFormatCheckErrorModel::getItemText(int row, int column) const {
    if (row < 0 || row >= items_.size()) {
        return {};
    }
    const auto& modelData = items_[row];
    if (column == 0) {
        return std::to_string(row + 1);
    } else if (column == 1) {
        return modelData->justFileName;
    } else if (column == 2) {
        return modelData->badFileFormat.mimeType;
    } else if (column == 3) {
        return IuCoreUtils::FileSizeToString(modelData->badFileFormat.fileSize);
    } else if (column == 4) {
        return modelData->extension;
    } else if (column == 5) {
        return modelData->badFileFormat.uploadProfile->serverName();
    } 
    return {};
}

uint32_t FileFormatCheckErrorModel::getItemColor(int row) const {
    const FileFormatModelData& modelData = *items_[row];
    switch (modelData.status()){
        case FileFormatModelData::RowStatus::Skipped: 
            return RGB(160, 160, 160);
        case FileFormatModelData::RowStatus::Ignore:
            return RGB(29, 194, 74);
        case FileFormatModelData::RowStatus::Error:
            return RGB(230, 0, 0);

        case FileFormatModelData::RowStatus::Normal:
        default:
            return GetSysColor(COLOR_WINDOWTEXT);
    } 
    return modelData.color;
}

size_t FileFormatCheckErrorModel::getCount() const {
    return items_.size();
}

void FileFormatCheckErrorModel::notifyRowChanged(size_t row) const {
    if (row < items_.size() && rowChangedCallback_) {
        rowChangedCallback_(row);
    }
}

FileFormatModelData* FileFormatCheckErrorModel::getDataByIndex(size_t row) const {
    if (row >= items_.size()) {
        return nullptr;
    }
    return items_[row].get();
}

void FileFormatCheckErrorModel::setOnRowChangedCallback(std::function<void(size_t)> callback) {
    rowChangedCallback_ = std::move(callback);
}

void FileFormatCheckErrorModel::resetData() {
    for (auto& it : items_) {
        it->clearInfo();
    }
}

size_t FileFormatCheckErrorModel::hasItemsWithStatus(FileFormatModelData::RowStatus status) const {
    size_t count = 0;
    for (auto& it : items_) {
        if (it->status() == status) {
            ++count;
        }
    }
    return count;
}
