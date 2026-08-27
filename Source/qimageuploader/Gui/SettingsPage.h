#ifndef IU_QIMAGEUPLOADER_GUI_SETTINGSPAGE_H
#define IU_QIMAGEUPLOADER_GUI_SETTINGSPAGE_H

#pragma once

#include <QWidget>

class QString;

class SettingsPage : public QWidget {
    Q_OBJECT

public:
    using QWidget::QWidget;
    ~SettingsPage() override = default;

    virtual void load() = 0;
    virtual bool validate(QString& error) const = 0;
    virtual void apply() = 0;
};

#endif
