#ifndef QIMAGEUPLOADER_GUI_ABOUTDIALOG_H
#define QIMAGEUPLOADER_GUI_ABOUTDIALOG_H

#include <memory>
#include <QDialog>

namespace Ui {
class AboutDialog;
}

class AboutDialog : public QDialog
{
    Q_OBJECT
    
public:
    AboutDialog(QWidget *parent = nullptr);
    ~AboutDialog() override;
private:
	std::unique_ptr<Ui::AboutDialog> ui;
};

#endif