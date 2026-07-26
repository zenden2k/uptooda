#ifndef IU_GUI_MYFILEDIALOG_H
#define IU_GUI_MYFILEDIALOG_H

#include <vector>
#include <memory>
#include "atlheaders.h"


class IMyFileDialog {
public:
    using FileFilter = std::pair<CString, CString>;
    using FileFilterArray = std::vector<FileFilter>;
    virtual ~IMyFileDialog() = default;
    virtual INT_PTR DoModal(HWND hWndParent)=0;
    virtual CString getFolderPath();
    virtual void setTitle(LPCWSTR title);
    virtual void setFileName(LPCWSTR fileName);
    virtual void setDefaultExtension(LPCTSTR extension);
    virtual void getFiles(std::vector<CString>& arr)=0; 
    virtual void setFileTypeIndex(UINT iFileType);
    CString getFile();
};

class MyFileDialogFactory {
public:
    static std::unique_ptr<IMyFileDialog> createFileDialog(HWND parent, const CString& initialFolder, const CString& title, const IMyFileDialog::FileFilterArray& filters, bool multiselect = false, bool openDialog = true);
};
#endif
