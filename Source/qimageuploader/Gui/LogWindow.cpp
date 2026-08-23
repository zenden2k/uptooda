#include "LogWindow.h"

#include "ui_LogWindow.h"

#include "Core/ServiceLocator.h"
#include "Core/Settings/BasicSettings.h"

LogWindow::LogWindow(QWidget* parent) :
    QDialog(parent),
    ui(new Ui::LogWindow) {
    separator.fill(  QLatin1Char('-'), 70);
    ui->setupUi(this);
    ui->plainTextEdit->setReadOnly(true);

    connect(ui->clearButton, &QPushButton::clicked, ui->plainTextEdit, &QPlainTextEdit::clear);
    connect(ui->hideButton, &QPushButton::clicked, this, &LogWindow::hide);

    setWindowFlags(windowFlags() | Qt::Tool | Qt::WindowCloseButtonHint);
    setAttribute(Qt::WA_QuitOnClose, false);
}

LogWindow::~LogWindow() = default;

void LogWindow::writeLog(ILogger::LogMsgType msgType, const QString &sender,
                          const QString &msg, const QString &info) {

    QString message = QStringLiteral("Sender:%1\r\n%2\r\n%3\r\n%4\r\n")
                           .arg(sender, info, msg, separator);

    QMetaObject::invokeMethod(this, "writeLogInMainThread", Q_ARG(int, msgType), Q_ARG(QString, message));
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
