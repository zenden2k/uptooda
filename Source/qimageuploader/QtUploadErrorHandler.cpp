#include "QtUploadErrorHandler.h"

#include <QString>
#include <QDebug>
#include "Core/CommonDefs.h"
#include "Core/i18n/Translator.h"
#include "Core/Upload/UploadEngine.h"

QtUploadErrorHandler::QtUploadErrorHandler(ILogger* logger, CUploadEngineListBase *engineList): logger_(logger),
    engineList_(engineList) {
    responseFileIndex_ = 0;
}

void QtUploadErrorHandler::ErrorMessage(const ErrorInfo& errorInfo) {
    ILogger::LogMsgType type = errorInfo.messageType == (ErrorInfo::mtWarning) ? ILogger::logWarning : ILogger::logError;
    QString errorMsg;

    QString infoText;
    if (!errorInfo.FileName.empty())
        infoText += QString::fromUtf8(_("File: ").str().c_str()) + U2Q(errorInfo.FileName) + "\n";

    if (!errorInfo.ServerName.empty()) {
        std::string serverNameU8 = (errorInfo.uploadEngineData && engineList_)
        ? CUploadEngineListBase::getServerDisplayName(errorInfo.uploadEngineData) : errorInfo.ServerName;
        QString serverName = U2Q(serverNameU8);
        if (!errorInfo.sender.empty())
            serverName += "(" + U2Q(errorInfo.sender) + ")";
        infoText += QString::fromUtf8(_("Server: ").str().c_str()) + serverName + "\n";
    }

    if (!errorInfo.Url.empty())
        infoText += QString::fromUtf8(_("URL: ").str().c_str()) + U2Q(errorInfo.Url) + "\n";

    if (errorInfo.ActionIndex != -1)
        infoText += QString::fromUtf8(_("Action:").str().c_str()) + " #" + QString::number(errorInfo.ActionIndex);

    /*if (infoText.Right(1) == _T("\n"))
        infoText.Delete(infoText.GetLength() - 1);*/
    if (!errorInfo.error.empty()) {
        errorMsg += U2Q(errorInfo.error);
    }
    else {
        /*if (errorInfo.errorType == etRepeating) {
            errorMsg.Format(TR("Upload failed. Another retry (%d)..."), errorInfo.RetryIndex);
        } else if (errorInfo.errorType == etRetriesLimitReached) {
            errorMsg = TR("Upload failed! (retry limit reached)");
        }*/
    }

    QString sender = QString::fromUtf8(_("Uploading module").str().c_str());
    if (!errorMsg.isEmpty()) {
        logger_->write(type, Q2U(sender), Q2U(errorMsg), Q2U(infoText));
    }
}

void QtUploadErrorHandler::DebugMessage(const std::string& msg, bool isResponseBody) {

}
