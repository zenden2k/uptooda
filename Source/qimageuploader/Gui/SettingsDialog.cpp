#include "SettingsDialog.h"

#include <QDialogButtonBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "Core/Settings/CommonGuiSettings.h"
#include "GeneralSettingsPage.h"
#include "SettingsPage.h"

SettingsDialog::SettingsDialog(CommonGuiSettings* settings, LogWindow* logWindow, QWidget* parent) :
    QDialog(parent), settings_(settings) {
    setObjectName(QStringLiteral("settingsDialog"));
    setWindowTitle(tr("Settings"));
    setWindowFlag(Qt::WindowContextHelpButtonHint, false);
    resize(900, 690);
    setMinimumSize(760, 580);

    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);
    rootLayout->setSpacing(0);

    auto* contentLayout = new QHBoxLayout;
    contentLayout->setContentsMargins(0, 0, 0, 0);
    contentLayout->setSpacing(0);
    pageList_ = new QListWidget(this);
    pageList_->setObjectName(QStringLiteral("settingsPageList"));
    pageList_->setFixedWidth(205);
    pageList_->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    pageList_->setSpacing(3);
    pageStack_ = new QStackedWidget(this);
    pageStack_->setObjectName(QStringLiteral("settingsPageStack"));
    contentLayout->addWidget(pageList_);
    contentLayout->addWidget(pageStack_, 1);
    rootLayout->addLayout(contentLayout, 1);

    auto* footer = new QFrame(this);
    footer->setObjectName(QStringLiteral("settingsFooter"));
    auto* footerLayout = new QHBoxLayout(footer);
    footerLayout->setContentsMargins(16, 12, 16, 12);
    savedLabel_ = new QLabel(tr("Settings have been saved."), footer);
    savedLabel_->setObjectName(QStringLiteral("settingsSavedLabel"));
    savedLabel_->hide();
    footerLayout->addWidget(savedLabel_);
    footerLayout->addStretch();
    auto* buttons
        = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel | QDialogButtonBox::Apply, footer);
    buttons->button(QDialogButtonBox::Ok)->setObjectName(QStringLiteral("settingsOkButton"));
    footerLayout->addWidget(buttons);
    rootLayout->addWidget(footer);

    addPage(tr("General"), [this, logWindow] { return new GeneralSettingsPage(settings_, logWindow, pageStack_); });

    connect(pageList_, &QListWidget::currentRowChanged, this, &SettingsDialog::showPage);
    connect(buttons, &QDialogButtonBox::accepted, this, &SettingsDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &SettingsDialog::reject);
    connect(buttons->button(QDialogButtonBox::Apply), &QPushButton::clicked, this, &SettingsDialog::applySettings);
    pageList_->setCurrentRow(0);
}

void SettingsDialog::addPage(const QString& title, std::function<SettingsPage*()> factory) {
    pages_.push_back({ title, std::move(factory), nullptr });
    pageList_->addItem(title);
}

SettingsPage* SettingsDialog::createPage(int index) {
    if (index < 0 || index >= static_cast<int>(pages_.size())) {
        return nullptr;
    }
    PageDescriptor& descriptor = pages_[index];
    if (!descriptor.page) {
        descriptor.page = descriptor.factory();
        pageStack_->addWidget(descriptor.page);
    }
    return descriptor.page;
}

void SettingsDialog::showPage(int index) {
    if (SettingsPage* page = createPage(index)) {
        pageStack_->setCurrentWidget(page);
    }
}

bool SettingsDialog::validateAndApply() {
    for (int index = 0; index < static_cast<int>(pages_.size()); ++index) {
        SettingsPage* page = createPage(index);
        QString error;
        if (!page->validate(error)) {
            pageList_->setCurrentRow(index);
            QMessageBox::warning(this, tr("Invalid settings"), error);
            return false;
        }
    }
    for (PageDescriptor& descriptor : pages_) {
        descriptor.page->apply();
    }
    if (!settings_->SaveSettings()) {
        QMessageBox::critical(this, tr("Settings"), tr("Unable to save the settings file."));
        return false;
    }
    savedLabel_->show();
    return true;
}

void SettingsDialog::applySettings() { validateAndApply(); }

void SettingsDialog::accept() {
    if (validateAndApply()) {
        QDialog::accept();
    }
}
