#ifndef CORE_UPLOAD_FILEUPLOADTASK_H
#define CORE_UPLOAD_FILEUPLOADTASK_H

#include "UploadTask.h"
#include <string>

class FileUploadTask: public UploadTask {
    public:
        FileUploadTask(const std::string& fileName, const std::string& displayName, UploadTask* parentTask = nullptr);
        Type type() const override;
        std::string getMimeType() const override;
        int64_t getDataLength() const override;
        std::string getFileName() const;
        int64_t getFileSize() const;
        void setFileName(const std::string& fileName);
        std::string getDisplayName() const;
        void setDisplayName(const std::string& name);
        std::string originalFileName() const;
        void finishTask(Status status) override;
        std::string toString() override;
        std::string title() const override;
        bool isImage() const;
        void setIsImage(bool image);
        bool isVideo() const;
        void setIsVideo(bool isVid);
        void setFileIndex(size_t index);
        size_t fileIndex() const;
protected:
        std::string fileName_;
        std::string originalFileName_;
        std::string displayName_;
        mutable int64_t cachedFileSize_;
        bool isImage_, isVideo_;
        size_t fileIndex_;
};    

#endif
