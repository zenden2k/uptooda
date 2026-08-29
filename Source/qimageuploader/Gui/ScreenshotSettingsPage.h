#ifndef IU_QIMAGEUPLOADER_GUI_SCREENSHOTSETTINGSPAGE_H
#define IU_QIMAGEUPLOADER_GUI_SCREENSHOTSETTINGSPAGE_H

#pragma once

#include "SettingsPage.h"

#include <memory>

class CommonGuiSettings;

namespace Ui {
class ScreenshotSettingsPage;
}

class ScreenshotSettingsPage final : public SettingsPage {
    Q_OBJECT

public:
    explicit ScreenshotSettingsPage(CommonGuiSettings* settings, QWidget* parent = nullptr);
    ~ScreenshotSettingsPage() override;

    void load() override;
    bool validate(QString& error) const override;
    void apply() override;

private slots:
    void browseForFolder();
    void showMacrosMenu();

private:
    std::unique_ptr<Ui::ScreenshotSettingsPage> ui_;
    CommonGuiSettings* settings_;
};

#endif
