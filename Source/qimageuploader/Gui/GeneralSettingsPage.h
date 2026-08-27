#ifndef IU_QIMAGEUPLOADER_GUI_GENERALSETTINGSPAGE_H
#define IU_QIMAGEUPLOADER_GUI_GENERALSETTINGSPAGE_H

#pragma once

#include "SettingsPage.h"

#include <memory>

class CommonGuiSettings;
class LogWindow;

namespace Ui {
class GeneralSettingsPage;
}

class GeneralSettingsPage final : public SettingsPage {
    Q_OBJECT

public:
    GeneralSettingsPage(CommonGuiSettings* settings, LogWindow* logWindow, QWidget* parent = nullptr);
    ~GeneralSettingsPage() override;

    void load() override;
    bool validate(QString& error) const override;
    void apply() override;

private slots:
    void browseForImageEditor();
    void showLog();
    void clearServerSettings();

private:
    void fillLanguages();

    std::unique_ptr<Ui::GeneralSettingsPage> ui_;
    CommonGuiSettings* settings_;
    LogWindow* logWindow_;
};

#endif
