#include "ScreenshotSettingsPage.h"

#include <QAction>
#include <QComboBox>
#include <QDir>
#include <QFileDialog>
#include <QLineEdit>
#include <QMenu>
#include <QPushButton>
#include <QRegularExpression>
#include <QSpinBox>

#include <utility>

#include "Core/Settings/CommonGuiSettings.h"
#include "ui_ScreenshotSettingsPage.h"

ScreenshotSettingsPage::ScreenshotSettingsPage(CommonGuiSettings* settings, QWidget* parent) :
    SettingsPage(parent), ui_(std::make_unique<Ui::ScreenshotSettingsPage>()), settings_(settings) {
    ui_->setupUi(this);

    ui_->formatCombo->addItems({ QStringLiteral("JPEG"), QStringLiteral("PNG"), QStringLiteral("GIF"),
                                 QStringLiteral("WebP"), QStringLiteral("WebP (lossless)") });

#ifndef _WIN32
    ui_->windowCaptureGroup->setEnabled(false);
    ui_->windowCaptureHint->setText(tr("These options are available on Windows only."));
#endif

    connect(ui_->folderBrowseButton, &QPushButton::clicked, this, &ScreenshotSettingsPage::browseForFolder);
    connect(ui_->macrosButton, &QPushButton::clicked, this, &ScreenshotSettingsPage::showMacrosMenu);

    load();
}

ScreenshotSettingsPage::~ScreenshotSettingsPage() = default;

void ScreenshotSettingsPage::load() {
    const ScreenshotSettingsStruct& screenshot = settings_->ScreenshotSettings;
    ui_->folderEdit->setText(QString::fromUtf8(screenshot.Folder.c_str()));
    ui_->filenameTemplateEdit->setText(QString::fromUtf8(screenshot.FilenameTemplate.c_str()));
    ui_->formatCombo->setCurrentIndex(qBound(0, screenshot.Format, ui_->formatCombo->count() - 1));
    ui_->qualitySpin->setValue(qBound(ui_->qualitySpin->minimum(), screenshot.Quality, ui_->qualitySpin->maximum()));
    ui_->timeoutSpin->setValue(qBound(ui_->timeoutSpin->minimum(), screenshot.Delay, ui_->timeoutSpin->maximum()));
    ui_->windowHidingDelaySpin->setValue(qBound(ui_->windowHidingDelaySpin->minimum(), screenshot.WindowHidingDelay,
                                                ui_->windowHidingDelaySpin->maximum()));
    ui_->foregroundCheckBox->setChecked(screenshot.ShowForeground);
    ui_->copyToClipboardCheckBox->setChecked(screenshot.CopyToClipboard);
    ui_->captureCursorCheckBox->setChecked(screenshot.CaptureCursor);
    ui_->removeCornersCheckBox->setChecked(screenshot.RemoveCorners);
    ui_->addShadowCheckBox->setChecked(screenshot.AddShadow);
    ui_->removeBackgroundCheckBox->setChecked(screenshot.RemoveBackground);
}

bool ScreenshotSettingsPage::validate(QString& error) const {
    const QString filenameTemplate = ui_->filenameTemplateEdit->text().trimmed();
    if (filenameTemplate.isEmpty()) {
        error = tr("The filename template cannot be empty!");
        ui_->filenameTemplateEdit->setFocus();
        return false;
    }
    if (filenameTemplate.contains(QRegularExpression(QStringLiteral("[:*?\"<>|]")))) {
        error = tr("The filename template contains forbidden characters!");
        ui_->filenameTemplateEdit->setFocus();
        ui_->filenameTemplateEdit->selectAll();
        return false;
    }
    return true;
}

void ScreenshotSettingsPage::apply() {
    ScreenshotSettingsStruct& screenshot = settings_->ScreenshotSettings;
    screenshot.Folder = ui_->folderEdit->text().trimmed().toUtf8().toStdString();
    screenshot.FilenameTemplate = ui_->filenameTemplateEdit->text().trimmed().toUtf8().toStdString();
    screenshot.Format = ui_->formatCombo->currentIndex();
    screenshot.Quality = ui_->qualitySpin->value();
    screenshot.Delay = ui_->timeoutSpin->value();
    screenshot.WindowHidingDelay = ui_->windowHidingDelaySpin->value();
    screenshot.ShowForeground = ui_->foregroundCheckBox->isChecked();
    screenshot.CopyToClipboard = ui_->copyToClipboardCheckBox->isChecked();
    screenshot.CaptureCursor = ui_->captureCursorCheckBox->isChecked();
    screenshot.RemoveCorners = ui_->removeCornersCheckBox->isChecked();
    screenshot.AddShadow = ui_->addShadowCheckBox->isChecked();
    screenshot.RemoveBackground = ui_->removeBackgroundCheckBox->isChecked();
}

void ScreenshotSettingsPage::browseForFolder() {
    const QString folder = QFileDialog::getExistingDirectory(this, tr("Select folder"), ui_->folderEdit->text());
    if (!folder.isEmpty()) {
        ui_->folderEdit->setText(QDir::toNativeSeparators(folder));
    }
}

void ScreenshotSettingsPage::showMacrosMenu() {
    const std::pair<const char*, const char*> macros[] = {
        { "%y%", QT_TR_NOOP("year") },
        { "%m%", QT_TR_NOOP("month") },
        { "%d%", QT_TR_NOOP("day") },
        { "%h%", QT_TR_NOOP("hour") },
        { "%n%", QT_TR_NOOP("minute") },
        { "%s%", QT_TR_NOOP("second") },
        { "%i%", QT_TR_NOOP("index") },
        { "%width%", QT_TR_NOOP("image width") },
        { "%height%", QT_TR_NOOP("image height") },
        { "%type%", QT_TR_NOOP("object type") },
    };

    QMenu menu(this);
    for (const auto& [macro, description] : macros) {
        QAction* action = menu.addAction(QString::fromLatin1(macro) + QStringLiteral(" — ") + tr(description));
        action->setData(QString::fromLatin1(macro));
    }
    if (QAction* selected = menu.exec(ui_->macrosButton->mapToGlobal(QPoint(0, ui_->macrosButton->height())))) {
        ui_->filenameTemplateEdit->insert(selected->data().toString());
    }
}
