#ifndef IU_SERVERLISTTOOL_FileFormatCheckErrorModel_H
#define IU_SERVERLISTTOOL_FileFormatCheckErrorModel_H

#pragma once

#include <atomic>
#include <chrono>
#include <string>
#include <vector>
#include <functional>
#include <mutex>
#include <memory>

#include "Core/Utils/CoreTypes.h"
#include "Core/IFileList.h"
#include "Core/FileTypeCheckTask.h"

class IFileListItem;

class FileFormatModelData {
public:
    enum class RowStatus { Normal, Error, Skipped, Ignore};

    uint32_t color;
    BadFileFormat badFileFormat;
    IFileListItem* file = nullptr;
    std::string justFileName;
    std::string extension;
    RowStatus status_ = RowStatus::Error;

protected:

public:
    FileFormatModelData() {
        color = 0;
    }

    void clearInfo() {
        color = 0;
        // TODO:
    }

    RowStatus status() const
    {
        return status_;
    }

    void setStatus(RowStatus status)
    {
        status_ = status;
    }
};


class FileFormatCheckErrorModel {
public:
    FileFormatCheckErrorModel(IFileList* fileList, const std::vector<BadFileFormat>&);
    std::string getItemText(int row, int column) const;
    uint32_t getItemColor(int row) const;
    size_t getCount() const;
    void notifyRowChanged(size_t row) const;
    FileFormatModelData* getDataByIndex(size_t row) const;
    void setOnRowChangedCallback(std::function<void(size_t)> callback);
    void resetData();
    size_t hasItemsWithStatus(FileFormatModelData::RowStatus status) const;

protected:
    IFileList* fileList_;
    std::vector<std::unique_ptr<FileFormatModelData>> items_;
    std::function<void(size_t)> rowChangedCallback_;
    DISALLOW_COPY_AND_ASSIGN(FileFormatCheckErrorModel);
};
#endif
