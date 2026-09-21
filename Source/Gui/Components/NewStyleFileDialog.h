#ifndef IU_GUI_NEWSTYLEFILEDIALOG_H
#define IU_GUI_NEWSTYLEFILEDIALOG_H

#include <vector>
#include "atlheaders.h"

#include "MyFileDialog.h"
#include "Func/Library.h"

class CNewStyleFileDialog: public IMyFileDialog {
public:
    CNewStyleFileDialog(HWND parent, const CString& initialFolder, const CString& title, const FileFilterArray& filters, bool multiselect = false);

    INT_PTR DoModal(HWND hWndParent) override;

    CString getFolderPath() override;
    void setTitle(LPCWSTR title) override;
    void setDefaultExtension(LPCTSTR extension) override;
    void setFileName(LPCWSTR fileName) override;
    void getFiles(std::vector<CString>& arr) override; 
    void setFileTypeIndex(UINT iFileType) override;

protected:
    Library shellDll_{ L"shell32.dll" };
    CComPtr<IFileOpenDialog> newStyleDialog_;
private:
    DISALLOW_COPY_AND_ASSIGN(CNewStyleFileDialog);
};
#endif