#ifndef UPTOODA_UPLOADSETTINGSTABWIDGET_H
#define UPTOODA_UPLOADSETTINGSTABWIDGET_H

#include <QWidget>

#include "Core/Upload/ServerProfile.h"

class QLabel;
class QPushButton;
class ServerSelectorWidget;
class UploadEngineManager;

class UploadSettingsTabWidget : public QWidget {
    Q_OBJECT

public:
    explicit UploadSettingsTabWidget(QWidget* parent = nullptr);

    void configure(UploadEngineManager* uploadEngineManager, const ServerProfile& imageProfile,
                   const ServerProfile& fileProfile);
    void setFileCount(int count);
    ServerProfile imageServerProfile() const;
    ServerProfile fileServerProfile() const;
    void fillServerIcons();

    signals:
        void backRequested();
    void uploadRequested();

private:
    QWidget* form_ = nullptr;
    QLabel* fileCountLabel_ = nullptr;
    ServerSelectorWidget* imageServerWidget_ = nullptr;
    ServerSelectorWidget* fileServerWidget_ = nullptr;
    QPushButton* uploadButton_ = nullptr;
};

#endif //UPTOODA_UPLOADSETTINGSTABWIDGET_H