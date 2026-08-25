#ifndef QIMAGEUPLOADER_GUI_LOGINDIALOG_H
#define QIMAGEUPLOADER_GUI_LOGINDIALOG_H

#include <QDialog>
#include <unordered_map>

#include "Core/Upload/Parameters/AbstractParameter.h"
#include "Core/Upload/UploadEngine.h"

class AbstractParameter;
class UploadEngineManager;
class QWidget;

namespace Ui {
class LoginDialog;
}

class LoginDialog : public QDialog {
    Q_OBJECT

public:
    explicit LoginDialog(ServerProfile& serverProfile, UploadEngineManager* uploadEngineManager, bool createNew,
                         QWidget* parent = nullptr);
    ~LoginDialog();
    QString accountName() const;

private:
    std::unique_ptr<Ui::LoginDialog> ui;
    ServerProfile& serverProfile_;
    UploadEngineManager* uploadEngineManager_;
    QString accountName_;
    bool createNew_;
    ServerProfile originalServerProfile_;
    std::string provisionalProfileName_;
    ParameterList parameterList_;
    std::unordered_map<AbstractParameter*, QWidget*> parameterWidgets_;
    void onAccept();
    void reject() override;
    void browseServerFolders();
    void loadServerParameters();
    void saveServerParameters(ServerSettingsStruct* serverSettings);
    void updateFolderLabel();
    void removeProvisionalProfile();
};

#endif
