#ifndef IU_FUNC_IUCOMMONFUNCTIONS_H
#define IU_FUNC_IUCOMMONFUNCTIONS_H

#include <unordered_set>
#include <string>


#include "atlheaders.h"

namespace IuCommonFunctions {

struct ScreenshotData {
    int index;
    time_t time;
};

extern int screenshotIndex;
CString GetDataFolder();

BOOL CreateTempFolder(CString& IUCommonTempFolder, CString& IUTempFolder);
void ClearTempFolder(CString folder);

int GetNextImgFile(LPCTSTR folder, CString& szBuffer);
CString GenerateFileName(const CString& templateStr, int index, const CPoint& size, time_t t, const CString& objectType,
                         const CString& originalName = _T(""));
CString ExpandRandomMacros(const CString& value);
CString MakeScreenshotFileName(const ScreenshotData& screenshotData, SIZE size);

bool IsImage(LPCTSTR szFileName);
bool IsImage(const std::string& fileName);

CString FindDataFolder();

const std::unordered_set<std::string>& GetSupportedImageExtensions();

CString PrepareFileDialogImageFilter();
};
#endif
