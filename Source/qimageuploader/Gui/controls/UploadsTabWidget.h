#ifndef UPTOODA_UPLOADSTABWIDGET_H
#define UPTOODA_UPLOADSTABWIDGET_H

#include <QWidget>

#include "Core/Upload/ServerProfile.h"

class QStackedWidget;
class UploadSessionListWidget;

class UploadsTabWidget : public QWidget {
    Q_OBJECT

public:
    explicit UploadsTabWidget(QWidget* parent = nullptr);

    UploadSessionListWidget* sessionList() const;

private:
    QStackedWidget* stack_ = nullptr;
    UploadSessionListWidget* sessionList_ = nullptr;
};


#endif //UPTOODA_UPLOADSTABWIDGET_H