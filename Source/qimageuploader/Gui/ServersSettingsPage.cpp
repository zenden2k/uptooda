#include "ServersSettingsPage.h"

#include <QFrame>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QScrollArea>
#include <QVBoxLayout>

#include "Core/Settings/CommonGuiSettings.h"
#include "Core/Upload/UploadEngine.h"
#include "Gui/controls/MultiServerSelectorWidget.h"
#include "Gui/controls/ServerSelectorWidget.h"

ServersSettingsPage::ServersSettingsPage(CommonGuiSettings* settings, UploadEngineManager* uploadEngineManager,
                                         QWidget* parent) : SettingsPage(parent), settings_(settings) {
    auto* rootLayout = new QVBoxLayout(this);
    rootLayout->setContentsMargins(0, 0, 0, 0);

    auto* scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    auto* scrollContents = new QWidget(scrollArea);
    auto* outerLayout = new QHBoxLayout(scrollContents);
    outerLayout->setContentsMargins(20, 24, 20, 24);

    auto* form = new QWidget(scrollContents);
    form->setMaximumWidth(960);
    auto* formLayout = new QVBoxLayout(form);
    formLayout->setContentsMargins(0, 0, 0, 0);
    formLayout->setSpacing(14);

    auto* titleLabel = new QLabel(tr("Servers"), form);
    titleLabel->setObjectName(QStringLiteral("settingsPageTitle"));
    formLayout->addWidget(titleLabel);

    imageServerSelector_ = new MultiServerSelectorWidget(uploadEngineManager, form);
    imageServerSelector_->setTitle(tr("Default servers for uploading images"));
    imageServerSelector_->setServersMask(ServerSelectorWidget::smImageServers | ServerSelectorWidget::smFileServers);
    formLayout->addWidget(imageServerSelector_);

    fileServerSelector_ = new MultiServerSelectorWidget(uploadEngineManager, form);
    fileServerSelector_->setTitle(tr("Default servers for other file types"));
    fileServerSelector_->setServersMask(ServerSelectorWidget::smFileServers);
    formLayout->addWidget(fileServerSelector_);

    quickScreenshotServerSelector_ = new MultiServerSelectorWidget(uploadEngineManager, form);
    quickScreenshotServerSelector_->setTitle(tr("Servers for quick screenshot uploading"));
    quickScreenshotServerSelector_->setServersMask(ServerSelectorWidget::smImageServers
                                                   | ServerSelectorWidget::smFileServers);
    formLayout->addWidget(quickScreenshotServerSelector_);

    auto* temporaryServerGroup = new QGroupBox(tr("Server for temporary images"), form);
    auto* temporaryServerLayout = new QVBoxLayout(temporaryServerGroup);
    temporaryServerLayout->setContentsMargins(6, 8, 6, 6);
    temporaryServerSelector_ = new ServerSelectorWidget(uploadEngineManager, false, temporaryServerGroup);
    temporaryServerSelector_->setServersMask(ServerSelectorWidget::smImageServers
                                             | ServerSelectorWidget::smFileServers);
    temporaryServerLayout->addWidget(temporaryServerSelector_);
    formLayout->addWidget(temporaryServerGroup);

    formLayout->addStretch(1);

    outerLayout->addStretch(1);
    outerLayout->addWidget(form, 12);
    outerLayout->addStretch(1);
    scrollArea->setWidget(scrollContents);
    rootLayout->addWidget(scrollArea);

    load();
    imageServerSelector_->fillServerIcons();
    fileServerSelector_->fillServerIcons();
    quickScreenshotServerSelector_->fillServerIcons();
    temporaryServerSelector_->fillServerIcons();
}

void ServersSettingsPage::load() {
    imageServerSelector_->setServerProfileGroup(settings_->imageServer);
    fileServerSelector_->setServerProfileGroup(settings_->fileServer);
    quickScreenshotServerSelector_->setServerProfileGroup(settings_->quickScreenshotServer);
    temporaryServerSelector_->setServerProfile(settings_->temporaryServer);
}

bool ServersSettingsPage::validateGroup(MultiServerSelectorWidget* selector, QString& error,
                                        bool focusFirstInvalid) const {
    QString serverName;
    if (selector->validate(&serverName, focusFirstInvalid)) {
        return true;
    }
    if (serverName.isEmpty()) {
        error = tr("You have not selected a server for \"%1\"").arg(selector->baseTitle());
    } else {
        error = tr("You have not selected account for server \"%1\"").arg(serverName);
    }
    return false;
}

bool ServersSettingsPage::validate(QString& error) const {
    MultiServerSelectorWidget* selectors[]
        = { imageServerSelector_, fileServerSelector_, quickScreenshotServerSelector_ };
    for (MultiServerSelectorWidget* selector : selectors) {
        if (!validateGroup(selector, error, true)) {
            return false;
        }
    }

    const ServerProfile& temporaryServer = temporaryServerSelector_->serverProfile();
    if (temporaryServer.serverName().empty()) {
        error = tr("You have not selected a server for \"Server for temporary images\"");
        temporaryServerSelector_->focusServerSelection();
        return false;
    }
    const CUploadEngineData* server = temporaryServer.uploadEngineData();
    if (server && server->NeedAuthorization == CUploadEngineData::naObligatory
        && temporaryServer.profileName().empty()) {
        error = tr("You have not selected account for server \"%1\"")
                    .arg(QString::fromUtf8(temporaryServer.serverName()));
        temporaryServerSelector_->focusServerSelection();
        return false;
    }
    return true;
}

void ServersSettingsPage::apply() {
    settings_->imageServer = imageServerSelector_->serverProfileGroup();
    settings_->fileServer = fileServerSelector_->serverProfileGroup();
    settings_->quickScreenshotServer = quickScreenshotServerSelector_->serverProfileGroup();
    settings_->temporaryServer = temporaryServerSelector_->serverProfile();
}
