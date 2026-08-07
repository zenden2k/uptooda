#pragma once

#include <map>

#include "atlheaders.h"
#include "resource.h"
#include "Func/MyEngineList.h"
#include "Core/TaskDispatcher.h"
#include "Gui/Controls/ProgressRingControl.h"
#include "Gui/Models/FileFormatCheckErrorModel.h"
#include "Gui/Controls/FileFormatErrorView.h"
#include "Core/Upload/UploadEngine.h"
//#include "Gui/Dialogs/WizardDlg.h"

class WtlGuiSettings;

struct FileFormatCheckResult {
    std::vector<size_t> skippedFileIndexes;
};

class CFileFormatCheckErrorDlg : public CDialogImpl<CFileFormatCheckErrorDlg>, public CDialogResize<CFileFormatCheckErrorDlg>, public CWinDataExchange<CFileFormatCheckErrorDlg> {
public:
    enum { IDD = IDD_FILEFORMATERRORDLG };
     enum {
        ID_COPYDIRECTURL = 13000, ID_COPYTHUMBURL, ID_COPYVIEWURL
    };
    CFileFormatCheckErrorDlg(IFileList* items, const std::vector<BadFileFormat>& errors);

    BEGIN_MSG_MAP(CFileFormatCheckErrorDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MESSAGE_HANDLER(WM_CONTEXTMENU, OnContextMenu)
        COMMAND_ID_HANDLER(IDOK, OnOK)
        COMMAND_ID_HANDLER(IDC_BUTTONSKIP, OnSkip)
        COMMAND_ID_HANDLER(IDC_BUTTONSKIPALL, OnSkipAll)
        COMMAND_ID_HANDLER(IDC_IGNORE, OnIgnore)
        COMMAND_ID_HANDLER(IDC_IGNOREALL, OnIgnoreAll)
        COMMAND_ID_HANDLER(IDCANCEL, OnCancel)
        COMMAND_ID_HANDLER(IDC_ERRORLOGBUTTON, OnErrorLogButtonClicked)
        NOTIFY_CODE_HANDLER(LVN_ITEMCHANGED, OnListViewItemChanged)
        CHAIN_MSG_MAP(CDialogResize<CFileFormatCheckErrorDlg>)
        REFLECT_NOTIFICATIONS()   
    END_MSG_MAP()

    BEGIN_DLGRESIZE_MAP(CFileFormatCheckErrorDlg)
        DLGRESIZE_CONTROL(IDC_FILELIST, DLSZ_SIZE_X | DLSZ_SIZE_Y)
        DLGRESIZE_CONTROL(IDOK, DLSZ_MOVE_X | DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_BUTTONSKIP, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_BUTTONSKIPALL, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_IGNORE, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDC_IGNOREALL, DLSZ_MOVE_Y)
        DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
    END_DLGRESIZE_MAP()

    BEGIN_DDX_MAP(CFileFormatCheckErrorDlg)
        DDX_CONTROL(IDC_FILELIST, listView_)
    END_DDX_MAP()

    // Handler prototypes (uncomment arguments if needed):
    //    LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    //    LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    //    LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)

    LRESULT OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnSkip(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    
    LRESULT OnErrorLogButtonClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnSkipAll(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnIgnore(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnIgnoreAll(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnListViewItemChanged(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/);

    const FileFormatCheckResult& result() const;

private:
    int contextMenuItemId;
    CString sourceFileHash_;

    FileFormatCheckErrorModel model_;
    CFileFormatErrorView listView_;

    CIcon icon_, iconSmall_;

    FileFormatCheckResult result_;
    CFont labelBoldFont_;

    /**
     * @throws ValidationException
     */
    void validateSettings();

    void enableButtons();
};

