#ifndef IU_QIMAGEUPLOADER_GUI_VIDEOGRABBERSETTINGSPAGE_H
#define IU_QIMAGEUPLOADER_GUI_VIDEOGRABBERSETTINGSPAGE_H

#pragma once

#include "SettingsPage.h"

#include <QColor>
#include <QFont>

#include <memory>

class CommonGuiSettings;

namespace Ui {
class VideoGrabberSettingsPage;
}

class VideoGrabberSettingsPage final : public SettingsPage {
    Q_OBJECT

public:
    explicit VideoGrabberSettingsPage(CommonGuiSettings* settings, QWidget* parent = nullptr);
    ~VideoGrabberSettingsPage() override;

    void load() override;
    bool validate(QString& error) const override;
    void apply() override;

private slots:
    void browseForFolder();
    void chooseFont();
    void chooseTextColor();
    void showMacrosMenu();
    void updateMediaInfoControls(bool enabled);

private:
    void updateColorButton();
    void updateFontButton();

    std::unique_ptr<Ui::VideoGrabberSettingsPage> ui_;
    CommonGuiSettings* settings_;
    QFont mediaInfoFont_;
    QColor textColor_;
    int fontCharset_ = 204;
    int fontWritingSystem_ = 0;
};

#endif
