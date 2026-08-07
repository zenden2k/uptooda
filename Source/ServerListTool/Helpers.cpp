#include "Helpers.h"

#include "3rdpart/GdiplusH.h"
#include "Core/Utils/CoreUtils.h"
#include "Core/Utils/StringUtils.h"

namespace ServersListTool::Helpers {

CString GetFileInfo(CString fileName, MyFileInfo* mfi) {
    std::string utf8FileName = W2U(fileName);
    int64_t fileSize = IuCoreUtils::GetFileSize(utf8FileName);
    std::string mimeType = IuCoreUtils::GetFileMimeTypeByContents(utf8FileName);
    std::string result = str(IuStringUtils::FormatNoExcept("%1% (%2% bytes); %3%;") % IuCoreUtils::FileSizeToString(fileSize)
        % fileSize % mimeType);

    CString wideMimeType = U2W(mimeType);

    if (mfi) {
        mfi->mimeType = wideMimeType;
    }

    if (wideMimeType.Find(_T("image/")) >= 0) {
        Gdiplus::Image pic(fileName);
        if (pic.GetLastStatus() == Gdiplus::Ok) {
            int width = static_cast<int>(pic.GetWidth());
            int height = static_cast<int>(pic.GetHeight());
            if (mfi) {
                mfi->width = width;
                mfi->height = height;
            }
            result += str(IuStringUtils::FormatNoExcept("%d x %d") % width % height);
        }
    }
    return U2W(result);
}

}
