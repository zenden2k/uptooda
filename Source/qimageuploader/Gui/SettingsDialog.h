#ifndef IU_QIMAGEUPLOADER_GUI_SETTINGSDIALOG_H
#define IU_QIMAGEUPLOADER_GUI_SETTINGSDIALOG_H

#pragma once

#include <QDialog>

#include <functional>
#include <vector>

class CommonGuiSettings;
class LogWindow;
class QListWidget;
class QLabel;
class QStackedWidget;
class SettingsPage;
class UploadEngineManager;

class SettingsDialog final : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(CommonGuiSettings* settings, UploadEngineManager* uploadEngineManager, LogWindow* logWindow,
                            QWidget* parent = nullptr);

private slots:
    void showPage(int index);
    void accept() override;
    void applySettings();

private:
    struct PageDescriptor {
        QString title;
        std::function<SettingsPage*()> factory;
        SettingsPage* page = nullptr;
    };

    void addPage(const QString& title, std::function<SettingsPage*()> factory);
    SettingsPage* createPage(int index);
    bool validateAndApply();

    CommonGuiSettings* settings_;
    QListWidget* pageList_;
    QStackedWidget* pageStack_;
    QLabel* savedLabel_;
    std::vector<PageDescriptor> pages_;
};

#endif
