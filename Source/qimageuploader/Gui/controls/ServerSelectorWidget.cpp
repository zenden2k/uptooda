#include "ServerSelectorWidget.h"

#include <QGridLayout>
#include <QDir>
#include <QMenu>
#include <QDebug>

#include "Core/Upload/UploadEngineManager.h"
#include "Core/Upload/ServerProfile.h"
#include "Core/ServiceLocator.h"
#include "Core/CommonDefs.h"
#include "Core/AppParams.h"
#include "Core/Settings/BasicSettings.h"
#include "Gui/LoginDialog.h"
#include "Core/AbstractServerIconCache.h"

ServerSelectorWidget::ServerSelectorWidget(UploadEngineManager* uploadEngineManager, bool defaultServer,
                                           QWidget* parent) : QGroupBox(parent) {
    this->uploadEngineManager = uploadEngineManager;
    showDefaultServerItem = false;
    showFileSizeLimits = false;
    serversMask = smImageServers | smFileServers;

    setStyleSheet("QGroupBox {font-weight: bold;}");
    QGridLayout* grid = new QGridLayout(this);
    serverListComboBox = new QComboBox(this);

    accountButton = new QToolButton(this);
    accountButton->setIcon(QIcon(":/res/icon-user.png"));
    accountButton->setText(tr("<without account>"));
    accountButton->setToolButtonStyle(Qt::ToolButtonTextBesideIcon);
    accountButton->setPopupMode(QToolButton::MenuButtonPopup);
    //accountButton->setFlat(true);
    accountButton->setCursor(Qt::PointingHandCursor);
    accountButton->setStyleSheet("QPushButton { color : blue; }");
    /*accountIcon = new QLabel(this);
    accountIcon->setPixmap(QIcon(":/res/icon-user.ico").pixmap(16,16));*/
    grid->addWidget(serverListComboBox, 0, 0);
    accountLayout = new QHBoxLayout();
    //accountLayout->addWidget(accountIcon);
    accountLayout->addWidget(accountButton);
    QSpacerItem* horizontalSpacer = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

    accountLayout->addItem(horizontalSpacer);

    grid->addLayout(accountLayout, 0, 1);
    connect(serverListComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(comboBoxIndexChanged(int)));
    connect(accountButton, &QPushButton::clicked, this, &ServerSelectorWidget::accountButtonClicked);
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
    int itemIndex = serverListComboBox->findData(U2Q(serverProfile.serverName()));
    if (itemIndex != -1) {
        serverListComboBox->setCurrentIndex(itemIndex);
        updateAccountButton();
    }
}

void ServerSelectorWidget::setShowDefaultServerItem(bool show) {
    showDefaultServerItem = show;
}

void ServerSelectorWidget::setServersMask(int mask) {
    serversMask = mask;
}

void ServerSelectorWidget::setShowFilesizeLimits(bool show) {
    showFileSizeLimits = show;
}

void ServerSelectorWidget::updateServerList() {
    auto serviceLocator = ServiceLocator::instance();
    serverListComboBox->clear();
    //auto* serverIconCache = serviceLocator->serverIconCache();
    auto* myEngineList = serviceLocator->engineList();

    int addedItems = 0;
    std::string selectedServerName = serverProfile_.serverName();
    QString line;
    line.fill('-', 40);

    AppParams* params = AppParams::instance();
    QString dataDir = U2Q(params->dataDirectory());

    for (int mask = 1; mask <= 4; mask *= 2) {
        int currentLoopMask = mask & serversMask;
        if (!currentLoopMask) {
            continue;
        }
        if (addedItems) {
            serverListComboBox->insertSeparator(addedItems);
        }
        for (int i = 0; i < myEngineList->count(); i++) {
            CUploadEngineData* ue = myEngineList->byIndex(i);

            if (serversMask != smUrlShorteners && !ue->hasType(CUploadEngineData::TypeFileServer) && !ue->hasType(
                CUploadEngineData::TypeImageServer)) {
                continue;
            }
            if (!ue->hasType(CUploadEngineData::TypeImageServer) && ((currentLoopMask & smImageServers) ==
                smImageServers)) {
                continue;
            }
            if (!ue->hasType(CUploadEngineData::TypeFileServer) && ((currentLoopMask & smFileServers) == smFileServers)
            ) {
                continue;
            }

            if (!ue->hasType(CUploadEngineData::TypeUrlShorteningServer) && (currentLoopMask & smUrlShorteners)) {
                continue;
            }

            QString iconPath = QDir(dataDir).filePath("Favicons/" + U2Q(ue->Name).toLower() + ".ico");

            //QIcon ico = serverIconCache->getIconForServer(ue->Name);
            /*if (QFile::exists(iconPath)) {
                ico = QIcon(iconPath);

            }
            if (ico.isNull()) {
                ico = QIcon(":/res/server.png");
            }*/
            /*HICON hImageIcon = myEngineList->getIconForServer(ue->Name);
            int nImageIndex = -1;
            if (hImageIcon) {
                nImageIndex = comboBoxImageList_.AddIcon(hImageIcon);
            }*/

            std::string displayName = ue->Name;
            if (showFileSizeLimits && ue->MaxFileSize > 0) {
                displayName += " (" + IuCoreUtils::FileSizeToString(ue->MaxFileSize) + ")";
            }
            serverListComboBox->addItem(QIcon(), U2Q(displayName), U2Q(ue->Name));
            /*if (ue->Name == selectedServerName) {
                selectedIndex = itemIndex;
            }*/
            addedItems++;
        }
    }
    /*if (serversMask != smUrlShorteners) {
        serverListComboBox->addItem(line);
        //serverListComboBox->addItem(tr("Add FTP server..."), reinterpret_cast<LPARAM>(kAddFtpServer));
        //serverListComboBox->addItem(tr("Add local folder..."), reinterpret_cast<LPARAM>(kAddDirectoryAsServer));
    }*/
}

void ServerSelectorWidget::comboBoxIndexChanged(int index) {
    if (serverListComboBox->itemData(index).isNull()) {
        serverListComboBox->setCurrentIndex(0);
    }
    else {
        serverChanged();
    }
}

const ServerProfile& ServerSelectorWidget::serverProfile() const {
    return serverProfile_;
}

void ServerSelectorWidget::serverChanged() {
    QVariant data = serverListComboBox->currentData();
    QString serverName = data.toString();
    serverProfile_.setServerName(Q2U(serverName));
    serverProfile_.setProfileName({});
    auto ued = serverProfile_.uploadEngineData();
    if (ued) {
        accountButton->setVisible(ued->NeedAuthorization != CUploadEngineData::naNotAvailable);
    }
    updateAccountButton();
    updateAccountButtonMenu();
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
    }
    else {
        LoginDialog dlg(serverProfile_, false, this);
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
        //userAction->setData(U2Q(user.first));

        connect(userAction, &QAction::triggered, [accountName, this]
        {
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
    //accountButton->setText(tr("<without account>"));
    updateAccountButton();
}

void ServerSelectorWidget::addAccountClicked() {
    ServerProfile serverProfileCopy = serverProfile_;
    serverProfileCopy.setProfileName(std::string());

    LoginDialog dlg(serverProfileCopy, true, this);
    if (dlg.exec() == QDialog::Accepted) {
        std::string accountNameUtf8 = Q2U(dlg.accountName());
        serverProfileCopy.setProfileName(accountNameUtf8);
        serverProfileCopy.setFolderId(std::string());
        serverProfileCopy.setFolderTitle(std::string());
        serverProfileCopy.setFolderUrl(std::string());

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
    auto* serverIconCache = ServiceLocator::instance()->serverIconCache();
    int count = serverListComboBox->count();
    for (int i = 0; i < count; i++) {
        QString s = serverListComboBox->itemData(i).toString();
        std::string serverName = s.toStdString();

        QIcon ico = serverIconCache->getIconForServer(serverName, 96, true);
        serverListComboBox->setItemIcon(i, ico);
    }
}

void ServerSelectorWidget::updateAccountButton() {
    QString buttonText;
    if (!serverProfile_.profileName().empty()){
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
