#include "ServerSelectorWidget.h"

#include <QGridLayout>
#include <QMenu>

#include "Core/AbstractServerIconCache.h"
#include "Core/CommonDefs.h"
#include "Core/ServiceLocator.h"
#include "Core/Settings/BasicSettings.h"
#include "Core/Upload/ServerProfile.h"
#include "Core/Upload/UploadEngineManager.h"
#include "Gui/LoginDialog.h"
#include "ServerListPopup.h"

ServerSelectorWidget::ServerSelectorWidget(UploadEngineManager* uploadEngineManager, bool defaultServer,
                                           QWidget* parent) : QGroupBox(parent) {
    this->uploadEngineManager = uploadEngineManager;
    showDefaultServerItem = false;
    showFileSizeLimits = false;
    serversMask = smImageServers | smFileServers;

    setStyleSheet("QGroupBox {font-weight: bold;}");
    QGridLayout* grid = new QGridLayout(this);
    serverButton_ = new QToolButton(this);
    serverButton_->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    serverButton_->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    serverButton_->setCursor(Qt::PointingHandCursor);

    accountButton = new QToolButton(this);
    accountButton->setObjectName(QStringLiteral("serverAccountButton"));
    accountButton->setIcon(QIcon(":/res/icon-user.png"));
    accountButton->setText(tr("<without account>"));
    accountButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    accountButton->setPopupMode(QToolButton::MenuButtonPopup);
    accountButton->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    accountButton->setCursor(Qt::PointingHandCursor);

    grid->setHorizontalSpacing(10);
    grid->setColumnStretch(0, 1);
    grid->setColumnStretch(1, 1);
    grid->addWidget(serverButton_, 0, 0);
    accountLayout = new QHBoxLayout();
    accountLayout->setContentsMargins(0, 0, 0, 0);
    accountLayout->addWidget(accountButton);
    grid->addLayout(accountLayout, 0, 1);
    connect(serverButton_, &QToolButton::clicked, this, &ServerSelectorWidget::serverButtonClicked);
    connect(accountButton, &QToolButton::clicked, this, &ServerSelectorWidget::accountButtonClicked);
    updateServerList();
    updateAccountButton();
    updateAccountButtonMenu();
}

/*void ServerSelectorWidget::setTitle(QString title)
{
    titleLabel->setText(title);
}*/

void ServerSelectorWidget::setServerProfile(const ServerProfile& serverProfile) {
    serverProfile_ = serverProfile;
    updateServerButton();
    updateAccountButton();
    updateAccountButtonMenu();
}

void ServerSelectorWidget::setShowDefaultServerItem(bool show) { showDefaultServerItem = show; }

void ServerSelectorWidget::setServersMask(int mask) { serversMask = mask; }

void ServerSelectorWidget::setShowFilesizeLimits(bool show) { showFileSizeLimits = show; }

void ServerSelectorWidget::updateServerList() { updateServerButton(); }

void ServerSelectorWidget::serverButtonClicked() {
    auto* popup = new ServerListPopup(serversMask, serverProfile_.serverName(), this);
    popup->setAttribute(Qt::WA_DeleteOnClose);
    connect(popup, &QDialog::accepted, this, [this, popup] { serverChanged(popup->selectedServer()); });
    const QRect anchorRect(serverButton_->mapToGlobal(QPoint(0, 0)), serverButton_->size());
    popup->showPopup(anchorRect);
}

const ServerProfile& ServerSelectorWidget::serverProfile() const { return serverProfile_; }

void ServerSelectorWidget::focusServerSelection() { serverButton_->setFocus(Qt::OtherFocusReason); }

void ServerSelectorWidget::serverChanged(const std::string& serverName) {
    serverProfile_.setServerName(serverName);
    serverProfile_.setProfileName({ });
    auto ued = serverProfile_.uploadEngineData();
    if (ued) {
        accountButton->setVisible(ued->NeedAuthorization != CUploadEngineData::naNotAvailable);
    }
    updateServerButton();
    updateAccountButton();
    updateAccountButtonMenu();
    emit serverProfileChanged();
}

void ServerSelectorWidget::accountButtonClicked(bool /*checked*/) {
    /*QRect widgetRect = accountButton->geometry();
    QMenu* contextMenu = new QMenu(accountButton);
    QAction* viewCodeAction = new QAction(tr("<without account>"), contextMenu);

    contextMenu->addAction(viewCodeAction);
    contextMenu->setDefaultAction(viewCodeAction);
    contextMenu->exec(accountButton->parentWidget()->mapToGlobal(widgetRect.bottomLeft()));*/
    if (serverProfile_.profileName().empty()) {
        addAccountClicked();
    } else {
        LoginDialog dlg(serverProfile_, uploadEngineManager, false, this);
        if (dlg.exec() == QDialog::Accepted) {
            updateAccountButton();
            updateAccountButtonMenu();
        }
    }
}

void ServerSelectorWidget::updateAccountButtonMenu() {
    if (!accountButtonMenu_) {
        accountButtonMenu_.reset(new QMenu(accountButton));
    }
    accountButtonMenu_->clear();
    BasicSettings& Settings = *ServiceLocator::instance()->basicSettings();
    auto& serverUsers = Settings.ServersSettings[serverProfile_.serverName()];
    for (const auto& user : serverUsers) {
        std::string accountName = user.first;
        if (accountName.empty()) {
            continue;
        }
        QAction* userAction = new QAction(U2Q(accountName), accountButtonMenu_.get());
        // userAction->setData(U2Q(user.first));

        connect(userAction, &QAction::triggered, [accountName, this] {
            serverProfile_.setProfileName(accountName);
            updateAccountButton();
        });
        accountButtonMenu_->addAction(userAction);
    }

    auto ued = serverProfile_.uploadEngineData();
    if (ued && ued->NeedAuthorization != CUploadEngineData::naObligatory) {
        QAction* withoutAccountAction = new QAction(tr("<without account>"), accountButtonMenu_.get());
        connect(withoutAccountAction, &QAction::triggered, this, &ServerSelectorWidget::noAccountSelected);
        accountButtonMenu_->addAction(withoutAccountAction);
    }

    accountButtonMenu_->addSeparator();
    QAction* addAccountAction = new QAction(tr("Add account..."), accountButtonMenu_.get());
    connect(addAccountAction, &QAction::triggered, this, &ServerSelectorWidget::addAccountClicked);
    accountButtonMenu_->addAction(addAccountAction);
    accountButton->setMenu(accountButtonMenu_.get());
}

void ServerSelectorWidget::noAccountSelected() {
    serverProfile_.setProfileName(std::string());
    // accountButton->setText(tr("<without account>"));
    updateAccountButton();
}

void ServerSelectorWidget::addAccountClicked() {
    ServerProfile serverProfileCopy = serverProfile_;
    serverProfileCopy.setProfileName(std::string());

    LoginDialog dlg(serverProfileCopy, uploadEngineManager, true, this);
    if (dlg.exec() == QDialog::Accepted) {
        std::string accountNameUtf8 = Q2U(dlg.accountName());
        serverProfileCopy.setProfileName(accountNameUtf8);

        serverProfile_ = serverProfileCopy;
        /*auto settings = ServiceLocator::instance()->basicSettings();
        ServerSettingsStruct& sss = settings->ServersSettings[serverProfile_.serverName()][accountNameUtf8];
        sss.authData.DoAuth = true;
        sss.authData.Login = accountNameUtf8;
        sss.authData.Password = dlg.*/
        updateAccountButton();
        updateAccountButtonMenu();
    }
}

void ServerSelectorWidget::fillServerIcons() {
    iconsLoaded_ = true;
    updateServerButton();
}

void ServerSelectorWidget::updateServerButton() {
    const auto* server = serverProfile_.uploadEngineData();
    if (!server) {
        serverButton_->setText(tr("Choose server..."));
        serverButton_->setIcon(QIcon(":/res/server.png"));
        return;
    }

    std::string displayName = CUploadEngineListBase::getServerDisplayName(server);
    if (showFileSizeLimits && server->MaxFileSize > 0) {
        displayName += " (" + IuCoreUtils::FileSizeToString(server->MaxFileSize) + ")";
    }
    serverButton_->setText(U2Q(displayName));
    if (iconsLoaded_) {
        auto* iconCache = ServiceLocator::instance()->serverIconCache();
        serverButton_->setIcon(iconCache->getIconForServer(server->Name, 96, true));
    } else {
        serverButton_->setIcon(QIcon(":/res/server.png"));
    }
}

void ServerSelectorWidget::updateAccountButton() {
    QString buttonText;
    if (!serverProfile_.profileName().empty()) {
        buttonText = U2Q(serverProfile_.profileName());
    } else {
        auto ued = serverProfile_.uploadEngineData();

        if (ued && ued->NeedAuthorization != CUploadEngineData::naObligatory) {
            buttonText = tr("<without account>");
        } else {
            buttonText = tr("choose account...");
        }
    }

    accountButton->setText(buttonText);
}
