#include "ConnectionSettingsPage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDesktopServices>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QRadioButton>
#include <QSpinBox>
#include <QUrl>

#include "Core/Settings/CommonGuiSettings.h"
#include "ui_ConnectionSettingsPage.h"

ConnectionSettingsPage::ConnectionSettingsPage(CommonGuiSettings* settings, QWidget* parent) :
    SettingsPage(parent), ui_(std::make_unique<Ui::ConnectionSettingsPage>()), settings_(settings) {
    ui_->setupUi(this);

    ui_->proxyTypeCombo->addItem(QStringLiteral("HTTP"), 0);
    ui_->proxyTypeCombo->addItem(QStringLiteral("HTTPS"), 5);
    ui_->proxyTypeCombo->addItem(QStringLiteral("SOCKS4"), 1);
    ui_->proxyTypeCombo->addItem(QStringLiteral("SOCKS4A"), 2);
    ui_->proxyTypeCombo->addItem(QStringLiteral("SOCKS5"), 3);
    ui_->proxyTypeCombo->addItem(QStringLiteral("SOCKS5 (DNS)"), 4);

    connect(ui_->noProxyRadio, &QRadioButton::toggled, this, &ConnectionSettingsPage::updateProxyControls);
    connect(ui_->systemProxyRadio, &QRadioButton::toggled, this, &ConnectionSettingsPage::updateProxyControls);
    connect(ui_->customProxyRadio, &QRadioButton::toggled, this, &ConnectionSettingsPage::updateProxyControls);
    connect(ui_->authenticationCheckBox, &QCheckBox::toggled, this, &ConnectionSettingsPage::updateProxyControls);
    connect(ui_->openSystemSettingsButton, &QPushButton::clicked, this,
            &ConnectionSettingsPage::openSystemProxySettings);

#ifndef Q_OS_WIN
    ui_->openSystemSettingsButton->hide();
#endif

    load();
}

ConnectionSettingsPage::~ConnectionSettingsPage() = default;

void ConnectionSettingsPage::load() {
    const ConnectionSettingsStruct& connection = settings_->ConnectionSettings;
    switch (connection.UseProxy) {
    case ConnectionSettingsStruct::kUserProxy:
        ui_->customProxyRadio->setChecked(true);
        break;
    case ConnectionSettingsStruct::kSystemProxy:
        ui_->systemProxyRadio->setChecked(true);
        break;
    default:
        ui_->noProxyRadio->setChecked(true);
        break;
    }

    ui_->addressEdit->setText(QString::fromUtf8(connection.ServerAddress.c_str()));
    ui_->portSpin->setValue(connection.ProxyPort);
    const int proxyTypeIndex = ui_->proxyTypeCombo->findData(connection.ProxyType);
    ui_->proxyTypeCombo->setCurrentIndex(proxyTypeIndex >= 0 ? proxyTypeIndex : 0);
    ui_->authenticationCheckBox->setChecked(connection.NeedsAuth);
    ui_->loginEdit->setText(QString::fromUtf8(connection.ProxyUser.c_str()));
    ui_->passwordEdit->setText(
        QString::fromUtf8(static_cast<std::string&>(settings_->ConnectionSettings.ProxyPassword)));
    ui_->uploadSpeedLimitSpin->setValue(static_cast<int>(settings_->MaxUploadSpeed));
    updateProxyControls();
}

bool ConnectionSettingsPage::validate(QString& /*error*/) const { return true; }

void ConnectionSettingsPage::apply() {
    ConnectionSettingsStruct& connection = settings_->ConnectionSettings;
    if (ui_->customProxyRadio->isChecked()) {
        connection.UseProxy = ConnectionSettingsStruct::kUserProxy;
    } else if (ui_->systemProxyRadio->isChecked()) {
        connection.UseProxy = ConnectionSettingsStruct::kSystemProxy;
    } else {
        connection.UseProxy = ConnectionSettingsStruct::kNoProxy;
    }

    connection.ServerAddress = ui_->addressEdit->text().trimmed().toUtf8().toStdString();
    if (connection.UseProxy == ConnectionSettingsStruct::kUserProxy && connection.ServerAddress.empty()) {
        connection.UseProxy = ConnectionSettingsStruct::kNoProxy;
    }
    connection.ProxyPort = ui_->portSpin->value();
    connection.ProxyType = ui_->proxyTypeCombo->currentData().toInt();
    connection.NeedsAuth = ui_->authenticationCheckBox->isChecked();
    connection.ProxyUser = ui_->loginEdit->text().toUtf8().toStdString();
    connection.ProxyPassword.fromPlainText(ui_->passwordEdit->text().toUtf8().toStdString());
    settings_->MaxUploadSpeed = static_cast<unsigned int>(ui_->uploadSpeedLimitSpin->value());
}

void ConnectionSettingsPage::updateProxyControls() {
    const bool useCustomProxy = ui_->customProxyRadio->isChecked();
    const bool useAuthentication = useCustomProxy && ui_->authenticationCheckBox->isChecked();

    ui_->addressLabel->setEnabled(useCustomProxy);
    ui_->addressEdit->setEnabled(useCustomProxy);
    ui_->portLabel->setEnabled(useCustomProxy);
    ui_->portSpin->setEnabled(useCustomProxy);
    ui_->proxyTypeLabel->setEnabled(useCustomProxy);
    ui_->proxyTypeCombo->setEnabled(useCustomProxy);
    ui_->authenticationCheckBox->setEnabled(useCustomProxy);
    ui_->loginLabel->setEnabled(useAuthentication);
    ui_->loginEdit->setEnabled(useAuthentication);
    ui_->passwordLabel->setEnabled(useAuthentication);
    ui_->passwordEdit->setEnabled(useAuthentication);
    ui_->openSystemSettingsButton->setEnabled(ui_->systemProxyRadio->isChecked());
}

void ConnectionSettingsPage::openSystemProxySettings() {
#ifdef Q_OS_WIN
    QDesktopServices::openUrl(QUrl(QStringLiteral("ms-settings:network-proxy")));
#endif
}
