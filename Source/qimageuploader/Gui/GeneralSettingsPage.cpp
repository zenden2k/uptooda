#include "GeneralSettingsPage.h"

#include <QComboBox>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QLineEdit>
#include <QLocale>
#include <QMessageBox>
#include <QPushButton>
#include <QSet>

#include "Core/AppRuntimeInfo.h"
#include "Core/Settings/CommonGuiSettings.h"
#include "Gui/LogWindow.h"
#include "ui_GeneralSettingsPage.h"

GeneralSettingsPage::GeneralSettingsPage(CommonGuiSettings* settings, LogWindow* logWindow, QWidget* parent) :
    SettingsPage(parent), ui_(std::make_unique<Ui::GeneralSettingsPage>()), settings_(settings), logWindow_(logWindow) {
    ui_->setupUi(this);
    ui_->clearServersButton->setObjectName(QStringLiteral("destructiveSettingsButton"));
    ui_->videoPreviewCheckBox->setEnabled(CommonGuiSettings::IsFFmpegAvailable());

    connect(ui_->browseButton, &QPushButton::clicked, this, &GeneralSettingsPage::browseForImageEditor);
    connect(ui_->showLogButton, &QPushButton::clicked, this, &GeneralSettingsPage::showLog);
    connect(ui_->clearServersButton, &QPushButton::clicked, this, &GeneralSettingsPage::clearServerSettings);

    fillLanguages();
    load();
}

GeneralSettingsPage::~GeneralSettingsPage() = default;

void GeneralSettingsPage::fillLanguages() {
    ui_->languageCombo->clear();
    ui_->languageCombo->addItem(QStringLiteral("English"), QStringLiteral("en"));

    const QString appDirectory = QCoreApplication::applicationDirPath();
    const QString dataDirectory = QString::fromUtf8(AppRuntimeInfo::instance()->dataDirectory().c_str());
    const QStringList candidates
        = { appDirectory + QStringLiteral("/Lang/locale"), appDirectory + QStringLiteral("/Data/Lang/locale"),
            dataDirectory + QStringLiteral("Lang/locale") };
    QSet<QString> localeNames { QStringLiteral("en") };
    for (const QString& candidate : candidates) {
        const QDir directory(candidate);
        for (const QString& localeName : directory.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
            localeNames.insert(localeName);
        }
    }

    QStringList sortedLocales = localeNames.values();
    sortedLocales.sort(Qt::CaseInsensitive);
    for (const QString& localeName : sortedLocales) {
        if (localeName == QStringLiteral("en")) {
            continue;
        }
        const QLocale locale(localeName);
        QString displayName = locale.nativeLanguageName();
        if (displayName.isEmpty()) {
            displayName = localeName;
        }
        ui_->languageCombo->addItem(displayName + QStringLiteral("  (") + localeName + QLatin1Char(')'), localeName);
    }
}

void GeneralSettingsPage::load() {
    const QString language = QString::fromUtf8(settings_->Language.c_str());
    int languageIndex = ui_->languageCombo->findData(language.isEmpty() ? QStringLiteral("en") : language);
    if (languageIndex < 0 && !language.isEmpty()) {
        ui_->languageCombo->addItem(language, language);
        languageIndex = ui_->languageCombo->count() - 1;
    }
    ui_->languageCombo->setCurrentIndex(qMax(0, languageIndex));
    ui_->imageEditorEdit->setText(QString::fromUtf8(settings_->ImageEditorPath.c_str()));
    ui_->autoShowLogCheckBox->setChecked(settings_->AutoShowLog);
    ui_->confirmOnExitCheckBox->setChecked(settings_->ConfirmOnExit);
    ui_->dropVideoFilesCheckBox->setChecked(settings_->DropVideoFilesToTheList);
    ui_->developerModeCheckBox->setChecked(settings_->DeveloperMode);
    ui_->checkUpdatesCheckBox->setChecked(settings_->AutomaticallyCheckUpdates);
    ui_->toastNotificationsCheckBox->setChecked(settings_->EnableToastNotifications);
    ui_->videoPreviewCheckBox->setChecked(settings_->ShowPreviewForVideoFiles);
}

bool GeneralSettingsPage::validate(QString& error) const {
    const QString editorCommand = ui_->imageEditorEdit->text().trimmed();
    if (!editorCommand.isEmpty() && !editorCommand.contains(QStringLiteral("%1"))) {
        error = tr("The external image editor command must contain %1, which is replaced with the image file name.");
        ui_->imageEditorEdit->setFocus();
        ui_->imageEditorEdit->selectAll();
        return false;
    }
    return true;
}

void GeneralSettingsPage::apply() {
    settings_->Language = ui_->languageCombo->currentData().toString().toUtf8().toStdString();
    settings_->ImageEditorPath = ui_->imageEditorEdit->text().trimmed().toUtf8().toStdString();
    settings_->AutoShowLog = ui_->autoShowLogCheckBox->isChecked();
    settings_->ConfirmOnExit = ui_->confirmOnExitCheckBox->isChecked();
    settings_->DropVideoFilesToTheList = ui_->dropVideoFilesCheckBox->isChecked();
    settings_->DeveloperMode = ui_->developerModeCheckBox->isChecked();
    settings_->AutomaticallyCheckUpdates = ui_->checkUpdatesCheckBox->isChecked();
    settings_->EnableToastNotifications = ui_->toastNotificationsCheckBox->isChecked();
    settings_->ShowPreviewForVideoFiles = ui_->videoPreviewCheckBox->isChecked();
}

void GeneralSettingsPage::browseForImageEditor() {
    const QString fileName = QFileDialog::getOpenFileName(this, tr("Choose an image editor"));
    if (!fileName.isEmpty()) {
        ui_->imageEditorEdit->setText(QLatin1Char('"') + QDir::toNativeSeparators(fileName)
                                      + QStringLiteral("\" \"%1\""));
    }
}

void GeneralSettingsPage::showLog() {
    if (logWindow_) {
        logWindow_->show();
        logWindow_->raise();
        logWindow_->activateWindow();
    }
}

void GeneralSettingsPage::clearServerSettings() {
    const auto answer = QMessageBox::warning(
        this, tr("Clear server settings"),
        tr("All account data will be deleted, including logins, passwords, and other server settings."),
        QMessageBox::Ok | QMessageBox::Cancel, QMessageBox::Cancel);
    if (answer != QMessageBox::Ok) {
        return;
    }
    settings_->clearServerSettings();
    if (!settings_->SaveSettings()) {
        QMessageBox::critical(this, tr("Settings"), tr("Unable to save the settings file."));
    }
}
