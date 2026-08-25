#ifndef QIMAGEUPLOADER_GUI_CONTROLS_SERVERSELECTORWIDGET_H
#define QIMAGEUPLOADER_GUI_CONTROLS_SERVERSELECTORWIDGET_H

#include <QGroupBox>
#include <QHBoxLayout>
#include <QMenu>
#include <QToolButton>

#include "Core/Upload/ServerProfile.h"


class UploadEngineManager;

class ServerSelectorWidget : public QGroupBox {
    Q_OBJECT
private:
public:
    ServerSelectorWidget(UploadEngineManager* uploadEngineManager, bool defaultServer = false, QWidget* parent = 0);
    void setServerProfile(const ServerProfile& serverProfile);
    void setShowDefaultServerItem(bool show);
    void setServersMask(int mask);
    void setShowFilesizeLimits(bool show);
    void updateServerList();
    const ServerProfile& serverProfile() const;
    void focusServerSelection();
    enum ServerMaskEnum { smAll = 0xffff, smImageServers = 0x1, smFileServers = 0x2, smUrlShorteners = 0x4 };

public slots:
    void serverButtonClicked();
    void accountButtonClicked(bool checked);
    void noAccountSelected();
    void addAccountClicked();
    void fillServerIcons();

signals:
    void serverProfileChanged();

protected:
    QToolButton* accountButton;
    QToolButton* serverButton_;
    QHBoxLayout* accountLayout;
    UploadEngineManager* uploadEngineManager;
    ServerProfile serverProfile_;
    std::unique_ptr<QMenu> accountButtonMenu_;
    bool showDefaultServerItem;
    int serversMask;
    bool showFileSizeLimits;
    bool iconsLoaded_ = false;
    void serverChanged(const std::string& serverName);
    void updateAccountButtonMenu();
    void updateAccountButton();
    void updateServerButton();
};

#endif
