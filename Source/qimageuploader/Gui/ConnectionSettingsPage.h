#ifndef IU_QIMAGEUPLOADER_GUI_CONNECTIONSETTINGSPAGE_H
#define IU_QIMAGEUPLOADER_GUI_CONNECTIONSETTINGSPAGE_H

#pragma once

#include "SettingsPage.h"

#include <memory>

class CommonGuiSettings;

namespace Ui {
class ConnectionSettingsPage;
}

class ConnectionSettingsPage final : public SettingsPage {
    Q_OBJECT

public:
    explicit ConnectionSettingsPage(CommonGuiSettings* settings, QWidget* parent = nullptr);
    ~ConnectionSettingsPage() override;

    void load() override;
    bool validate(QString& error) const override;
    void apply() override;

private slots:
    void updateProxyControls();
    void openSystemProxySettings();

private:
    std::unique_ptr<Ui::ConnectionSettingsPage> ui_;
    CommonGuiSettings* settings_;
};

#endif
