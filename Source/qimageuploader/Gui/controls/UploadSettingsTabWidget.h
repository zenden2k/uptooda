#ifndef UPTOODA_UPLOADSETTINGSTABWIDGET_H
#define UPTOODA_UPLOADSETTINGSTABWIDGET_H

#include <QWidget>

#include "Core/Upload/ServerProfileGroup.h"

class QLabel;
class QPushButton;
class MultiServerSelectorWidget;
class UploadEngineManager;

class UploadSettingsTabWidget : public QWidget {
    Q_OBJECT

public:
    explicit UploadSettingsTabWidget(QWidget* parent = nullptr);

    void configure(UploadEngineManager* uploadEngineManager, const ServerProfileGroup& imageProfiles,
                   const ServerProfileGroup& fileProfiles);
    void setFileCount(int count);
    ServerProfileGroup imageServerProfileGroup() const;
    ServerProfileGroup fileServerProfileGroup() const;
    void fillServerIcons();

signals:
    void backRequested();
    void uploadRequested();

private:
    bool validateServerGroups();

    QWidget* form_ = nullptr;
    QLabel* fileCountLabel_ = nullptr;
    MultiServerSelectorWidget* imageServerWidget_ = nullptr;
    MultiServerSelectorWidget* fileServerWidget_ = nullptr;
    QPushButton* uploadButton_ = nullptr;
};

#endif // UPTOODA_UPLOADSETTINGSTABWIDGET_H
