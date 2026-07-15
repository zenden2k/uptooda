#include "NewStyleFileSaveDialog.h"

CNewStyleFileSaveDialog::CNewStyleFileSaveDialog(HWND parent, const CString& initialFolder, const CString& title, const FileFilterArray& filters){
    std::unique_ptr<COMDLG_FILTERSPEC[]> fileTypes;
    
    if (!filters.empty()) {
        fileTypes = std::make_unique<COMDLG_FILTERSPEC[]>(filters.size());
        for (size_t i = 0; i < filters.size(); i++) {
            fileTypes[i].pszName = filters[i].first;
            fileTypes[i].pszSpec = filters[i].second;
        }
    }
    
    DWORD dwFlags = 0;

    // Create the file-save dialog COM object.
    HRESULT hr = newStyleDialog_.CoCreateInstance(CLSID_FileSaveDialog);

    if (FAILED(hr)) {
        return;
    }

    if (fileTypes) {
        newStyleDialog_->SetFileTypes(filters.size(), fileTypes.get());
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

    dwFlags |=  FOS_FORCEFILESYSTEM | FOS_OVERWRITEPROMPT;
    newStyleDialog_->SetOptions(dwFlags);
}

INT_PTR CNewStyleFileSaveDialog::DoModal(HWND hWndParent) {
    HRESULT hr = newStyleDialog_->Show(hWndParent);

    // If the user chose any files, loop thru the array of files.
    if (SUCCEEDED(hr)) {
        return IDOK;
    }
    return IDCANCEL;
}

CString CNewStyleFileSaveDialog::getFolderPath() {
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

void CNewStyleFileSaveDialog::setTitle(LPCWSTR title) {
    newStyleDialog_->SetTitle(title);
}

void CNewStyleFileSaveDialog::setDefaultExtension(LPCTSTR extension) {
    newStyleDialog_->SetDefaultExtension(extension);
}

void CNewStyleFileSaveDialog::setFileName(LPCWSTR fileName) {
    newStyleDialog_->SetFileName(fileName);
}

void CNewStyleFileSaveDialog::getFiles(std::vector<CString>& arr) {
    // If the user chose any files, loop thru the array of files.

    CComPtr<IShellItem> pItem;

    HRESULT hr = newStyleDialog_->GetResult(&pItem);

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

void CNewStyleFileSaveDialog::setFileTypeIndex(UINT iFileType) {
    newStyleDialog_->SetFileTypeIndex(iFileType);
}
