#pragma once

#include "Core/BackgroundTask.h"
#include "Core/Upload/ServerProfileGroup.h"
#include "IFileList.h"

struct BadFileFormat {
    size_t index = 0;
    std::string fileName;
    std::string message;
    std::string mimeType;
    int64_t fileSize = -1;
    const ServerProfile* uploadProfile = nullptr;
};

class FileTypeCheckTask: public BackgroundTask {
public:
    explicit FileTypeCheckTask(IFileList* fileList, ServerProfileGroup sessionImageServer, ServerProfileGroup sessionFileServer, ImageUploadParams defaultImageUploadParams);
	
	BackgroundTaskResult doJob() override;

	std::string message() const;

	std::vector<BadFileFormat>& errors();
	DISALLOW_COPY_AND_ASSIGN(FileTypeCheckTask);

    private:
        IFileList* fileList_;
        ServerProfileGroup sessionImageServer_;
        ServerProfileGroup sessionFileServer_;
        std::string message_;
        std::vector<BadFileFormat> errors_;
        ImageUploadParams defaultImageUploadParams_;
};
