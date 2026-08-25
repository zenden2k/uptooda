#include "LoginDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QMessageBox>
#include <QPushButton>
#include <algorithm>
#include "Core/CommonDefs.h"
#include "Core/ServiceLocator.h"
#include "Core/Settings/BasicSettings.h"
#include "Core/Upload/AdvancedUploadEngine.h"
#include "Core/Upload/Parameters/BooleanParameter.h"
#include "Core/Upload/Parameters/ChoiceParameter.h"
#include "Core/Upload/Parameters/FileNameParameter.h"
#include "Core/Upload/UploadEngine.h"
#include "Core/Upload/UploadEngineManager.h"
#include "ServerFolderSelectDialog.h"
#include "ui_LoginDialog.h"

LoginDialog::LoginDialog(ServerProfile& serverProfile, UploadEngineManager* uploadEngineManager, bool createNew,
                         QWidget* parent) :
    QDialog(parent), ui(new Ui::LoginDialog), serverProfile_(serverProfile), uploadEngineManager_(uploadEngineManager),
    createNew_(createNew), originalServerProfile_(serverProfile) {

    ui->setupUi(this);

    BasicSettings* Settings = ServiceLocator::instance()->basicSettings();
    if (!Settings) {
        return;
    }
    ServerSettingsStruct* serverSettings = Settings->getServerSettings(serverProfile_);

    LoginInfo li = serverSettings ? serverSettings->authData : LoginInfo();
    auto ued = serverProfile_.uploadEngineData();
    if (ued) {
        setWindowTitle(tr("%1 server settings").arg(U2Q(serverProfile_.serverName())));
        QString loginLabelText = ued->LoginLabel.empty() ? tr("Login:") : U2Q(ued->LoginLabel) + ":";
        ui->loginLabel->setText(loginLabelText);
        QString passwordLabelText = ued->PasswordLabel.empty() ? tr("Password:") : U2Q(ued->PasswordLabel) + ":";
        ui->passwordLabel->setText(passwordLabelText);
        ui->passwordLabel->setEnabled(ued->NeedPassword);
        ui->passwordEdit->setEnabled(ued->NeedPassword);
        ui->folderWidget->setVisible(ued->SupportsFolders);
    }

    accountName_ = U2Q(li.Login);

    ui->loginEdit->setText(accountName_);
    ui->passwordEdit->setText(U2Q(li.Password));
    updateFolderLabel();
    loadServerParameters();

    connect(ui->buttonBox, &QDialogButtonBox::accepted, this, &LoginDialog::onAccept);
    connect(ui->selectFolderButton, &QPushButton::clicked, this, &LoginDialog::browseServerFolders);
    connect(ui->loginEdit, &QLineEdit::textChanged, this, [this](const QString& login) {
        ui->selectFolderButton->setEnabled(!login.isEmpty());
        if (login != accountName_) {
            ui->folderNameLabel->setText(tr("<not selected>"));
        } else {
            updateFolderLabel();
        }
    });
    ui->selectFolderButton->setEnabled(!ui->loginEdit->text().isEmpty());
}


LoginDialog::~LoginDialog() { removeProvisionalProfile(); }

QString LoginDialog::accountName() const { return accountName_; }

void LoginDialog::onAccept() {
    LoginInfo li;
    QString buffer = ui->loginEdit->text();

    li.Login = Q2U(buffer);

    if (li.Login.empty()) {
        QMessageBox::critical(this, tr("Error"), tr("Login cannot be empty"));
        return;
    }
    std::string serverNameU8 = serverProfile_.serverName();
    BasicSettings* settings = ServiceLocator::instance()->basicSettings();
    // /* !ignoreExistingAccount_ &&  */
    if (createNew_ && li.Login != provisionalProfileName_
        && settings->ServersSettings[serverNameU8].find(li.Login) != settings->ServersSettings[serverNameU8].end()) {
        QMessageBox::critical(this, tr("Error"), tr("Account with such name already exists."));
        return;
    }

    if (li.Login != serverProfile_.profileName()) {
        serverProfile_.clearFolderInfo();
    }

    accountName_ = buffer;
    if (!provisionalProfileName_.empty() && provisionalProfileName_ != li.Login) {
        removeProvisionalProfile();
    }
    serverProfile_.setProfileName(Q2U(buffer));
    li.Login = Q2U(buffer);
    li.Password = Q2U(ui->passwordEdit->text());
    li.DoAuth = true;
    // uploadEngineManager_->resetAuthorization(serverProfile_);

    ServerSettingsStruct* serverSettings = settings->getServerSettings(serverProfile_, true);
    if (serverSettings) {
        serverSettings->authData = li;
        saveServerParameters(serverSettings);
    }
    provisionalProfileName_.clear();
    accept();
}

void LoginDialog::reject() {
    removeProvisionalProfile();
    serverProfile_ = originalServerProfile_;
    QDialog::reject();
}

void LoginDialog::browseServerFolders() {
    const QString login = ui->loginEdit->text();
    if (login.isEmpty()) {
        return;
    }

    const std::string loginUtf8 = Q2U(login);
    BasicSettings* settings = ServiceLocator::instance()->basicSettings();
    if (createNew_ && loginUtf8 != provisionalProfileName_) {
        const auto serverIt = settings->ServersSettings.find(serverProfile_.serverName());
        if (serverIt != settings->ServersSettings.end() && serverIt->second.find(loginUtf8) != serverIt->second.end()) {
            QMessageBox::critical(this, tr("Error"), tr("Account with such name already exists."));
            return;
        }
        removeProvisionalProfile();
    }

    if (loginUtf8 != serverProfile_.profileName()) {
        serverProfile_.clearFolderInfo();
    }
    serverProfile_.setProfileName(loginUtf8);

    ServerSettingsStruct* serverSettings = settings ? settings->getServerSettings(serverProfile_, true) : nullptr;
    if (createNew_) {
        provisionalProfileName_ = loginUtf8;
    }
    if (serverSettings) {
        serverSettings->authData.Login = Q2U(login);
        serverSettings->authData.Password = Q2U(ui->passwordEdit->text());
        serverSettings->authData.DoAuth = true;
        saveServerParameters(serverSettings);
    }

    ServerFolderSelectDialog dialog(serverProfile_, uploadEngineManager_, this);
    if (dialog.exec() == QDialog::Accepted) {
        const CFolderItem folder = dialog.selectedFolder();
        if (folder.id.empty()) {
            serverProfile_.clearFolderInfo();
        } else {
            serverProfile_.setFolder(folder);
        }
        updateFolderLabel();
        loadServerParameters();
    }
}

void LoginDialog::loadServerParameters() {
    while (ui->parametersFormLayout->rowCount() > 0) {
        ui->parametersFormLayout->removeRow(0);
    }
    parameterWidgets_.clear();
    parameterList_.clear();

    auto uploadEngine
        = std::dynamic_pointer_cast<CAdvancedUploadEngine>(uploadEngineManager_->getUploadEngine(serverProfile_));
    if (!uploadEngine || uploadEngine->getServerParamList(parameterList_) <= 0) {
        ui->parametersGroup->setVisible(false);
        return;
    }

    std::sort(parameterList_.begin(), parameterList_.end(),
              [](const auto& left, const auto& right) { return left->getName() < right->getName(); });
    BasicSettings* settings = ServiceLocator::instance()->basicSettings();
    ServerSettingsStruct* serverSettings = settings ? settings->getServerSettings(serverProfile_) : nullptr;

    for (const auto& parameter : parameterList_) {
        std::string value;
        if (serverSettings) {
            const auto valueIt = serverSettings->params.find(parameter->getName());
            if (valueIt != serverSettings->params.end()) {
                value = valueIt->second;
            }
        }
        parameter->setValue(value);
        QWidget* editor = nullptr;
        if (auto* choice = dynamic_cast<ChoiceParameter*>(parameter.get())) {
            auto* combo = new QComboBox(ui->parametersGroup);
            for (const auto& item : choice->getItems()) {
                combo->addItem(U2Q(item.second), U2Q(item.first));
            }
            combo->setCurrentIndex(choice->selectedIndex());
            editor = combo;
        } else if (auto* booleanParameter = dynamic_cast<BooleanParameter*>(parameter.get())) {
            auto* checkBox = new QCheckBox(ui->parametersGroup);
            checkBox->setChecked(booleanParameter->getValue());
            editor = checkBox;
        } else if (auto* fileName = dynamic_cast<FileNameParameter*>(parameter.get())) {
            auto* container = new QWidget(ui->parametersGroup);
            auto* layout = new QHBoxLayout(container);
            layout->setContentsMargins(0, 0, 0, 0);
            auto* edit = new QLineEdit(U2Q(fileName->getValueAsString()), container);
            auto* browseButton = new QPushButton(tr("Browse..."), container);
            layout->addWidget(edit, 1);
            layout->addWidget(browseButton);
            connect(browseButton, &QPushButton::clicked, this, [this, edit, fileName] {
                const QString path = fileName->directory()
                    ? QFileDialog::getExistingDirectory(this, tr("Choose folder"), edit->text())
                    : QFileDialog::getOpenFileName(this, tr("Choose file"), edit->text());
                if (!path.isEmpty()) {
                    edit->setText(path);
                }
            });
            editor = edit;
            ui->parametersFormLayout->addRow(U2Q(parameter->getTitle()) + ':', container);
        } else {
            editor = new QLineEdit(U2Q(parameter->getValueAsString()), ui->parametersGroup);
        }

        if (!parameter->getDescription().empty()) {
            editor->setToolTip(U2Q(parameter->getDescription()));
        }
        parameterWidgets_[parameter.get()] = editor;
        if (!dynamic_cast<FileNameParameter*>(parameter.get())) {
            ui->parametersFormLayout->addRow(U2Q(parameter->getTitle()) + ':', editor);
        }
    }
    ui->parametersGroup->setVisible(true);
}

void LoginDialog::saveServerParameters(ServerSettingsStruct* serverSettings) {
    if (!serverSettings) {
        return;
    }
    for (const auto& parameter : parameterList_) {
        QWidget* editor = parameterWidgets_[parameter.get()];
        if (auto* choice = dynamic_cast<ChoiceParameter*>(parameter.get())) {
            choice->setSelectedIndex(qobject_cast<QComboBox*>(editor)->currentIndex());
        } else if (auto* booleanParameter = dynamic_cast<BooleanParameter*>(parameter.get())) {
            booleanParameter->setValue(qobject_cast<QCheckBox*>(editor)->isChecked());
        } else if (auto* edit = qobject_cast<QLineEdit*>(editor)) {
            parameter->setValue(Q2U(edit->text()));
        }
        serverSettings->params[parameter->getName()] = parameter->getValueAsString();
    }
}

void LoginDialog::updateFolderLabel() {
    const QString title = U2Q(serverProfile_.folderTitle());
    ui->folderNameLabel->setText(title.isEmpty() ? tr("<not selected>") : title);
}

void LoginDialog::removeProvisionalProfile() {
    if (provisionalProfileName_.empty()) {
        return;
    }
    if (BasicSettings* settings = ServiceLocator::instance()->basicSettings()) {
        settings->deleteProfile(serverProfile_.serverName(), provisionalProfileName_);
    }
    provisionalProfileName_.clear();
}
