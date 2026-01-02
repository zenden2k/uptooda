#ifndef IU_FUNC_IUCOMMONFUNCTIONS_H
#define IU_FUNC_IUCOMMONFUNCTIONS_H

#include <unordered_set>
#include <string>


#include "atlheaders.h"

namespace IuCommonFunctions {
    extern int screenshotIndex;
    CString GetDataFolder();

    BOOL CreateTempFolder(CString& IUCommonTempFolder, CString& IUTempFolder);
    void ClearTempFolder(CString folder);

    int GetNextImgFile(LPCTSTR folder, CString& szBuffer);
    CString GenerateFileName(const CString &templateStr, int index, const CPoint& size, const CString& originalName = _T(""));
    bool IsImage(LPCTSTR szFileName);
    bool IsImage(const std::string fileName);

    CString FindDataFolder();

    const std::unordered_set<std::string>& GetSupportedImageExtensions();

    CString PrepareFileDialogImageFilter();
};
#endif
