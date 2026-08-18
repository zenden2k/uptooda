#include "LogWindow.h"

#include "ui_LogWindow.h"

#include "Core/ServiceLocator.h"
#include "Core/Settings/BasicSettings.h"

LogWindow::LogWindow(QWidget* parent) : QDialog(parent), ui(new Ui::LogWindow) {
    ui->setupUi(this);
    ui->plainTextEdit->setReadOnly(true);

    connect(ui->clearButton, &QPushButton::clicked, ui->plainTextEdit, &QPlainTextEdit::clear);
    connect(ui->hideButton, &QPushButton::clicked, this, &LogWindow::hide);

    setWindowFlags(windowFlags() | Qt::Tool | Qt::WindowCloseButtonHint);
    setAttribute(Qt::WA_QuitOnClose, false);
}


LogWindow::~LogWindow() { }

void LogWindow::writeLog(ILogger::LogMsgType MsgType, QString Sender, QString Msg, QString Info) {
    QString line;
    line.fill('-', 70);
    QString msg = tr("Sender:") + Sender + "\r\n" + Info + "\r\n" + Msg + "\r\n" + line + "\r\n";
    QMetaObject::invokeMethod(this, "writeLogInMainThread", Q_ARG(int, MsgType), Q_ARG(QString, msg));
}

void LogWindow::writeLogInMainThread(int msgType, QString msg) {
    ui->plainTextEdit->appendPlainText(msg);
    if (msgType == ILogger::logError) {
        auto settings = ServiceLocator::instance()->basicSettings();
        if (settings->AutoShowLog) {
            show();
            raise();
            activateWindow();
        }
    }
}
