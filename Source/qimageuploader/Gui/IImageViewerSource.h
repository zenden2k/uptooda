#ifndef QIMAGEUPLOADER_GUI_IIMAGEVIEWERSOURCE_H
#define QIMAGEUPLOADER_GUI_IIMAGEVIEWERSOURCE_H

#include <QString>
#include <QStringList>

#include <utility>

class IImageViewerSource {
public:
    virtual ~IImageViewerSource() = default;
    virtual QString currentFile() const = 0;
    virtual QString nextFile() = 0;
    virtual QString previousFile() = 0;
    virtual bool hasNext() const = 0;
    virtual bool hasPrevious() const = 0;
};

class FileListImageViewerSource final : public IImageViewerSource {
public:
    explicit FileListImageViewerSource(QStringList fileNames, int currentIndex = 0) :
        fileNames_(std::move(fileNames)), currentIndex_(currentIndex) { }

    QString currentFile() const override {
        return currentIndex_ >= 0 && currentIndex_ < fileNames_.size() ? fileNames_[currentIndex_] : QString { };
    }

    QString nextFile() override {
        if (hasNext()) {
            currentIndex_ = (currentIndex_ + 1) % fileNames_.size();
        }
        return currentFile();
    }

    QString previousFile() override {
        if (hasPrevious()) {
            currentIndex_ = (currentIndex_ - 1 + fileNames_.size()) % fileNames_.size();
        }
        return currentFile();
    }

    bool hasNext() const override {
        return currentIndex_ >= 0 && currentIndex_ < fileNames_.size() && fileNames_.size() > 1;
    }

    bool hasPrevious() const override {
        return hasNext();
    }

private:
    QStringList fileNames_;
    int currentIndex_ = 0;
};

#endif
