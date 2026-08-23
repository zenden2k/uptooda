#ifndef QIMAGEUPLOADER_GUI_RESULTSWINDOW_H
#define QIMAGEUPLOADER_GUI_RESULTSWINDOW_H

#include <QDialog>
#include <vector>
#include <memory>
#include "Core/OutputGenerator/AbstractOutputGenerator.h"

namespace Ui {
class ResultsWindow;
}

class ResultsWindow : public QDialog
{
    Q_OBJECT
    
public:
    explicit ResultsWindow(std::vector<Uptooda::Core::OutputGenerator::UploadObject> uploadObjects, QWidget *parent = nullptr);
    ~ResultsWindow() override;
protected:
	
private slots:
	void generateCode();
	void currentTabChanged(int index);
	void onCopyToClipboard();
private:
    std::unique_ptr<Ui::ResultsWindow> ui;
    std::vector<Uptooda::Core::OutputGenerator::UploadObject> uploadObjects_;
};

#endif // QIMAGEUPLOADER_GUI_RESULTSWINDOW_H
