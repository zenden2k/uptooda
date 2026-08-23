#ifndef QIMAGEUPLOADER_GUI_LOGWINDOW_H
#define QIMAGEUPLOADER_GUI_LOGWINDOW_H

#include <memory>
#include <QDialog>
#include "Core/Logging/Logger.h"

namespace Ui {
class LogWindow;
}

class LogWindow : public QDialog
{
    Q_OBJECT
 
public:
    explicit LogWindow(QWidget *parent = nullptr);
    ~LogWindow() override;
    void writeLog(ILogger::LogMsgType msgType, const QString &sender,
                          const QString &msg, const QString &info);
protected:
	std::unique_ptr<Ui::LogWindow> ui;
    Q_INVOKABLE void writeLogInMainThread(int msgType, QString msg);
    QString separator;
};

#endif // QIMAGEUPLOADER_GUI_LOGWINDOW_H
