#include "QtScriptDialogProvider.h"

#include <QInputDialog>
#include <QMessageBox>
#include <QMetaObject>
#include <QDesktopServices>
#include <qapplication.h>

#include "Core/Network/NetworkClient.h"
#include "Core/ProgramWindow.h"
#include "Core/CommonDefs.h"
#include "Core/ServiceLocator.h"

QtScriptDialogProvider::QtScriptDialogProvider() {
    dialogResult_ = 0;
}

std::string QtScriptDialogProvider::askUserCaptcha(INetworkClient* nm, const std::string& url) {
    std::lock_guard<std::mutex> lk(mutex_);
    // TODO: Implement this
    QDesktopServices::openUrl(QString::fromStdString(url));
    return showDialog("Enter text from the image:", "");
}

std::string QtScriptDialogProvider::inputDialog(const std::string& text, const std::string& defaultValue) {
    std::lock_guard<std::mutex> lk(mutex_);
    return showDialog(text, defaultValue);
}

std::string QtScriptDialogProvider::messageBox(const std::string& message, const std::string& title,
                                               const std::string& buttons, const std::string& type) {
    std::string result;
    QMetaObject::invokeMethod(qApp, [&] {
        result = messageBoxInMainThread(U2Q(message), U2Q(title), U2Q(buttons), U2Q(type));
    }, Qt::BlockingQueuedConnection);
    return result;
}

std::string QtScriptDialogProvider::showDialog(const std::string& text, const std::string& defaultValue) {
    // GUI objects must be created only in main thread
    QString defaultValueStr = U2Q(defaultValue);
    QString textStr = U2Q(text);
    QMetaObject::invokeMethod(this, "showDialogInMainThread", Qt::BlockingQueuedConnection, Q_ARG(QString, textStr),
                              Q_ARG(QString, defaultValueStr));
    return value_;
}

void QtScriptDialogProvider::showDialogInMainThread(QString text, QString defaultValue) {
    QInputDialog dlg;
    dlg.setTextValue(defaultValue);
    dlg.setLabelText(text);
    if (dlg.exec() == QDialog::Accepted) {
        this->value_ = Q2U(dlg.textValue());
    } else {
        this->value_.clear();
    }
}

std::string QtScriptDialogProvider::messageBoxInMainThread(QString message, QString title, QString buttons,
                                                           QString type) {
    QMessageBox::StandardButtons uButtons = QMessageBox::NoButton;
    if (buttons == "ABORT_RETRY_IGNORE") {
        uButtons = QMessageBox::Abort | QMessageBox::Retry | QMessageBox::Ignore;
    } else if (buttons == "CANCEL_TRY_CONTINUE") {
        uButtons = QMessageBox::Cancel | QMessageBox::Retry | QMessageBox::Ignore;
    } else if (buttons == "OK_CANCEL") {
        uButtons = QMessageBox::Ok | QMessageBox::Cancel;
    } else if (buttons == "RETRY_CANCEL") {
        uButtons = QMessageBox::Retry | QMessageBox::Cancel;
    } else if (buttons == "YES_NO") {
        uButtons = QMessageBox::Yes | QMessageBox::No;
    } else if (buttons == "YES_NO_CANCEL") {
        uButtons = QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel;
    }
    QMessageBox::Icon icon = QMessageBox::NoIcon;
    if (type == "EXCLAMATION") {
        icon = QMessageBox::Warning;
    } else if (type == "WARNING") {
        icon = QMessageBox::Warning;
    } else if (type == "INFORMATION") {
        icon = QMessageBox::Information;
    } else if (type == "QUESTION") {
        icon = QMessageBox::Question;
    } else if (type == "ERROR") {
        icon = QMessageBox::Critical;
    }
    IProgramWindow* window = ServiceLocator::instance()->programWindow();

    QMessageBox msgBox(icon, title, message, uButtons, window->getHandle());
    int res = msgBox.exec();
    if (res == QMessageBox::Abort) {
        return "ABORT";
    } else if (res == QMessageBox::Cancel) {
        return "CANCEL";
    } else if (res == QMessageBox::Ignore) {
        return "IGNORE";
    } else if (res == QMessageBox::No) {
        return "NO";
    } else if (res == QMessageBox::Ok) {
        return "OK";
    } else if (res == QMessageBox::Yes) {
        return "YES";
    } else if (res == QMessageBox::Retry) {
        return "TRY";
    }
    return {};
}
