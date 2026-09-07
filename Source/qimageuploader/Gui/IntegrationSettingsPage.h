#pragma once

#include <map>
#include "Core/Upload/ServerProfileGroup.h"
#include "SettingsPage.h"

class QtGuiSettings;
class UploadEngineManager;
class QCheckBox;
class QListWidget;
class QToolButton;

class IntegrationSettingsPage final : public SettingsPage {
    Q_OBJECT
public:
    IntegrationSettingsPage(QtGuiSettings* settings, UploadEngineManager* manager, QWidget* parent = nullptr);
    void load() override;
    bool validate(QString& error) const override;
    void apply() override;
    void afterSave() override;
    QString applyError() const override;

private:
    void editItem(bool create);
    void moveItem(int offset);
    void updateButtons();
    QtGuiSettings* settings_;
    UploadEngineManager* manager_;
    QCheckBox* enabled_;
    QCheckBox* videos_;
    QCheckBox* cascaded_;
    QCheckBox* sendTo_;
    QCheckBox* quick_;
    QListWidget* items_;
    QToolButton* edit_;
    QToolButton* remove_;
    QToolButton* up_;
    QToolButton* down_;
    std::map<QString, ServerProfileGroup> groups_;
    QString error_;
    bool changed_ = false;
};
