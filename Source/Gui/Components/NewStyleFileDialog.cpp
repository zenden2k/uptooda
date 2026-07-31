#include "NewStyleFileDialog.h"

#include <vector>

CNewStyleFileDialog::CNewStyleFileDialog(HWND parent, const CString& initialFolder, const CString& title, const FileFilterArray& filters, bool multiselect, bool openDialog){
    std::vector<COMDLG_FILTERSPEC> fileTypes;
    
    if (!filters.empty()) {
        fileTypes.resize(filters.size());
        for (size_t i = 0; i < filters.size(); i++) {
            fileTypes[i].pszName = filters[i].first;
            fileTypes[i].pszSpec = filters[i].second;
        }
    }
    
    DWORD dwFlags = 0;

    // Create the file-save dialog COM object.
    HRESULT hr = newStyleDialog_.CoCreateInstance(CLSID_FileOpenDialog);

    if (FAILED(hr)) {
        return;
    }

    if (!filters.empty()) {
        newStyleDialog_->SetFileTypes(filters.size(), fileTypes.data());
    }

    if (!title.IsEmpty()) {
        newStyleDialog_->SetTitle(title);
    }

    CComPtr<IShellItem> psiFolder;


    hr = SHCreateItemFromParsingName(initialFolder, nullptr, IID_PPV_ARGS(&psiFolder));

    if (SUCCEEDED(hr)) {
        newStyleDialog_->SetDefaultFolder(psiFolder);
    }

    newStyleDialog_->GetOptions(&dwFlags);

    if (multiselect) {
        dwFlags |= FOS_ALLOWMULTISELECT;
    }
    dwFlags |= FOS_FILEMUSTEXIST | FOS_FORCEFILESYSTEM;
    newStyleDialog_->SetOptions(dwFlags);
}

INT_PTR CNewStyleFileDialog::DoModal(HWND hWndParent) {
    HRESULT hr = newStyleDialog_->Show(hWndParent);

    // If the user chose any files, loop thru the array of files.
    if (SUCCEEDED(hr)) {
        return IDOK;
    }
    return IDCANCEL;
}

CString CNewStyleFileDialog::getFolderPath() {
    CString result;

    CComPtr<IShellItem> pFolderItem;
    HRESULT hr = newStyleDialog_->GetFolder(&pFolderItem);
    if (SUCCEEDED(hr)) {
        LPOLESTR pwsz = nullptr;

        // Get its file system path.
        hr = pFolderItem->GetDisplayName(SIGDN_FILESYSPATH, &pwsz);

        if (SUCCEEDED(hr)) {
            result = pwsz;
            CoTaskMemFree(pwsz);
        }
    }
    
    return result;
}


void CNewStyleFileDialog::setTitle(LPCWSTR title) {
    newStyleDialog_->SetTitle(title);
}

void CNewStyleFileDialog::setDefaultExtension(LPCTSTR extension) {
    newStyleDialog_->SetDefaultExtension(extension);
}

void CNewStyleFileDialog::setFileName(LPCWSTR fileName) {
    newStyleDialog_->SetFileName(fileName);
}

void CNewStyleFileDialog::getFiles(std::vector<CString>& arr) {
    HRESULT hr;
    // If the user chose any files, loop thru the array of files.

    CComPtr<IShellItemArray> pItemArray;

    hr = newStyleDialog_->GetResults(&pItemArray);

    if (SUCCEEDED(hr)) {
        DWORD cSelItems;

        // Get the number of selected files.
        hr = pItemArray->GetCount(&cSelItems);

        if (SUCCEEDED(hr)) {
            if (!cSelItems) {
                return;
            }
            for (DWORD j = 0; j < cSelItems; j++) {
                CComPtr<IShellItem> pItem;

                // Get an IShellItem interface on the next file.
                hr = pItemArray->GetItemAt(j, &pItem);

                if (SUCCEEDED(hr)) {
                    LPOLESTR pwsz = nullptr;

                    // Get its file system path.
                    hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &pwsz);

                    if (SUCCEEDED(hr)) {
                        arr.emplace_back(pwsz);
                        CoTaskMemFree(pwsz);
                    }
                }
            }
        }
    }
}

void CNewStyleFileDialog::setFileTypeIndex(UINT iFileType) {
    newStyleDialog_->SetFileTypeIndex(iFileType);
}