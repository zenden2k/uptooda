#ifndef QIMAGEUPLOADER_GUI_IIMAGEVIEWERSOURCE_H
#define QIMAGEUPLOADER_GUI_IIMAGEVIEWERSOURCE_H

#include <QString>

class IImageViewerSource {
public:
    virtual ~IImageViewerSource() = default;

    virtual QString currentFile() const = 0;
    virtual QString nextFile() = 0;
    virtual QString previousFile() = 0;
    virtual bool hasNext() const = 0;
    virtual bool hasPrevious() const = 0;
};

#endif
