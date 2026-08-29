#ifndef IU_QIMAGEUPLOADER_GUI_SERVERSSETTINGSPAGE_H
#define IU_QIMAGEUPLOADER_GUI_SERVERSSETTINGSPAGE_H

#pragma once

#include "SettingsPage.h"

class CommonGuiSettings;
class MultiServerSelectorWidget;
class ServerSelectorWidget;
class UploadEngineManager;

class ServersSettingsPage final : public SettingsPage {
    Q_OBJECT

public:
    ServersSettingsPage(CommonGuiSettings* settings, UploadEngineManager* uploadEngineManager,
                        QWidget* parent = nullptr);

    void load() override;
    bool validate(QString& error) const override;
    void apply() override;

private:
    bool validateGroup(MultiServerSelectorWidget* selector, QString& error, bool focusFirstInvalid) const;

    CommonGuiSettings* settings_ = nullptr;
    MultiServerSelectorWidget* imageServerSelector_ = nullptr;
    MultiServerSelectorWidget* fileServerSelector_ = nullptr;
    MultiServerSelectorWidget* quickScreenshotServerSelector_ = nullptr;
    ServerSelectorWidget* temporaryServerSelector_ = nullptr;
};

#endif
