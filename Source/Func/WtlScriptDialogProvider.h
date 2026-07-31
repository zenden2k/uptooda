#ifndef IU_FUNC_WTLSCRIPTDIALOGPROVIDER_H
#define IU_FUNC_WTLSCRIPTDIALOGPROVIDER_H

#include "Core/Scripting/DialogProvider.h"

class WtlScriptDialogProvider : public IDialogProvider {
public:
    std::string askUserCaptcha(INetworkClient *nm, const std::string& url) override;
    std::string inputDialog(const std::string& text, const std::string& defaultValue) override;
    std::string messageBox(const std::string& message, const std::string& title, const std::string& buttons, const std::string& type) override;
};
#endif
