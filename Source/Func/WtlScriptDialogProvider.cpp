#include "WtlScriptDialogProvider.h"

#include "Gui/Dialogs/InputDialog.h"
#include "Core/CommonDefs.h"
#include "WinUtils.h"
#include "Core/Network/NetworkClient.h"
#include "Core/AppRuntimeInfo.h"

std::string WtlScriptDialogProvider::askUserCaptcha(INetworkClient* nm, const std::string& url) {   
    std::lock_guard<std::mutex> guard(dialogMutex_);
    const CString wFileName = WinUtils::GetUniqFileName(AppRuntimeInfo::instance()->tempDirectoryW() + Utf8ToWstring("captcha").c_str());

    nm->setOutputFile(W2U(wFileName));
    if (!nm->doGet(url))
        return {};
    CInputDialog dlg(APP_NAME, TR("Please enter the text you see in the image:"), {}, wFileName);
    nm->setOutputFile({});
    IProgramWindow* window = ServiceLocator::instance()->programWindow();
    if (dlg.DoModal(window ? window->getHandle() : GetActiveWindow()) == IDOK)
        return W2U(dlg.getValue());
    return {};
}

std::string WtlScriptDialogProvider::inputDialog(const std::string& text, const std::string& defaultValue) {
    std::lock_guard<std::mutex> guard(dialogMutex_);
    CInputDialog dlg(APP_NAME, Utf8ToWCstring(text), Utf8ToWCstring(defaultValue));
    IProgramWindow* window = ServiceLocator::instance()->programWindow();
	
    if (dlg.DoModal(window? window->getHandle(): GetActiveWindow()) == IDOK) {
        return W2U(dlg.getValue());
    }
    return {};
}

std::string WtlScriptDialogProvider::messageBox(const std::string& message, const std::string& title, const std::string& buttons, const std::string& type) {
    UINT uButtons = MB_OK;
    if (buttons == "ABORT_RETRY_IGNORE") {
        uButtons = MB_ABORTRETRYIGNORE;
    } else if (buttons == "CANCEL_TRY_CONTINUE") {
        uButtons = MB_CANCELTRYCONTINUE;
    } else if (buttons == "OK_CANCEL") {
        uButtons = MB_OKCANCEL;
    } else if (buttons == "RETRY_CANCEL") {
        uButtons = MB_RETRYCANCEL;
    } else if (buttons == "YES_NO") {
        uButtons = MB_YESNO;
    } else if (buttons == "YES_NO_CANCEL") {
        uButtons = MB_YESNOCANCEL;
    }
    UINT icon = 0;
    if (type == "EXCLAMATION") {
        icon = MB_ICONEXCLAMATION;
    } else if (type == "WARNING") {
        icon = MB_ICONWARNING;
    } else if (type == "INFORMATION") {
        icon = MB_ICONINFORMATION;
    } else if (type == "QUESTION") {
        icon = MB_ICONQUESTION;
    } else if (type == "ERROR") {
        icon = MB_ICONERROR;
    }
    IProgramWindow* window = ServiceLocator::instance()->programWindow();

    int res = ::MessageBox(window ? window->getHandle() : ::GetActiveWindow(), IuCoreUtils::Utf8ToWstring(message).c_str(), IuCoreUtils::Utf8ToWstring(title).c_str(), uButtons | icon);
    if (res == IDABORT) {
        return "ABORT";
    } else if (res == IDCANCEL) {
        return "CANCEL";
    } else if (res == IDCONTINUE) {
        return "CONTINUE";
    } else if (res == IDIGNORE) {
        return "IGNORE";
    } else if (res == IDNO) {
        return "NO";
    } else if (res == IDOK) {
        return "OK";
    } else if (res == IDYES) {
        return "YES";
    } else if (res == IDRETRY) {
        return "TRY";
    } else if (res == IDTRYAGAIN) {
        return "TRY";
    }
    return {};
}
