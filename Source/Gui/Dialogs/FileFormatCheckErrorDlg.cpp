#include "FileFormatCheckErrorDlg.h"

#include "Gui/Dialogs/LogWindow.h"
#include "Core/Utils/CoreUtils.h"
#include "Gui/GuiTools.h"
#include "Func/WinUtils.h"
#include "Core/ServiceLocator.h"
#include "Core/Settings/BasicSettings.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "Func/WebUtils.h"

CFileFormatCheckErrorDlg::CFileFormatCheckErrorDlg(IFileList* items, const std::vector<BadFileFormat>& errors)
    : model_(items, errors)
    , listView_(&model_)
{
    contextMenuItemId = -1;
}

LRESULT CFileFormatCheckErrorDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    SetWindowText(TR("Error while file format checking"));
    TRC(IDC_FILESCANNOTBEUPLOADED, "These files cannot be uploaded to chosen servers:");
    TRC(IDC_BUTTONSKIP, "Skip selected");
    TRC(IDC_BUTTONSKIPALL, "Skip all");
    TRC(IDCANCEL, "Cancel");
    TRC(IDOK, "Continue");
    TRC(IDC_IGNORE, "Ignore");
    TRC(IDC_IGNOREALL, "Ignore all");
    CenterWindow(); // center the dialog on the screen
    DlgResize_Init(false, true, 0); // resizable dialog without "griper"
    DoDataExchange(FALSE);

    // set icons
    icon_.LoadIcon(MAKEINTRESOURCE(IDR_MAINFRAME), ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
    SetIcon(icon_, TRUE);
    iconSmall_.LoadIcon(MAKEINTRESOURCE(IDR_MAINFRAME),::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    SetIcon(iconSmall_, FALSE);

    labelBoldFont_ = GuiTools::MakeLabelBold(GetDlgItem(IDC_FILESCANNOTBEUPLOADED));
    
    listView_.Init();

    listView_.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
    enableButtons();
    return TRUE;
}

LRESULT CFileFormatCheckErrorDlg::OnContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
    HWND hwnd = reinterpret_cast<HWND>(wParam);
    POINT ClientPoint, ScreenPoint;
    if (hwnd != GetDlgItem(IDC_TOOLSERVERLIST)) return 0;

    if (lParam == -1) {
        ClientPoint.x = 0;
        ClientPoint.y = 0;
        int nCurItem = listView_.GetNextItem(-1, LVNI_ALL | LVNI_SELECTED);
        if (nCurItem >= 0) {
            CRect rc;
            if (listView_.GetItemRect(nCurItem, &rc, LVIR_BOUNDS)) {
                ClientPoint = rc.CenterPoint();
            }
        }
        
        ScreenPoint = ClientPoint;
        ::ClientToScreen(hwnd, &ScreenPoint);
    } else {
        ScreenPoint.x = GET_X_LPARAM(lParam);
        ScreenPoint.y = GET_Y_LPARAM(lParam);
        ClientPoint = ScreenPoint;
        ::ScreenToClient(hwnd, &ClientPoint);
    }
    LV_HITTESTINFO hti;
    memset(&hti, 0, sizeof(hti));
    hti.pt = ClientPoint;
    listView_.HitTest(&hti);

    if (hti.iItem >= 0) {
        FileFormatModelData* sd = model_.getDataByIndex(hti.iItem);

        if (sd) {
            CMenu menu;
            menu.CreatePopupMenu();
            /* menu.AppendMenu(MF_STRING, ID_COPYDIRECTURL, _T("Copy direct url"));
            menu.EnableMenuItem(ID_COPYDIRECTURL, sd->directUrl().empty() ? MF_DISABLED : MF_ENABLED);

            menu.AppendMenu(MF_STRING, ID_COPYTHUMBURL, _T("Copy thumb url"));
            menu.EnableMenuItem(ID_COPYTHUMBURL, sd->thumbUrl().empty() ? MF_DISABLED : MF_ENABLED);

            menu.AppendMenu(MF_STRING, ID_COPYVIEWURL, _T("Copy view url"));
            menu.EnableMenuItem(ID_COPYVIEWURL, sd->viewurl().empty() ? MF_DISABLED : MF_ENABLED);
            */
            contextMenuItemId = hti.iItem;
            menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, ScreenPoint.x, ScreenPoint.y, m_hWnd);
        } 
    }

    return 0;
}

void CFileFormatCheckErrorDlg::validateSettings() {
    //CString fileName = GuiTools::GetWindowText(GetDlgItem(IDC_TOOLFILEEDIT));
    /* if (!WinUtils::FileExists(fileName)) {
        throw ValidationException(CString(_T("Test file not found.")) + _T("\r\n") + fileName);
    }*/
}

void CFileFormatCheckErrorDlg::enableButtons(){
    bool enable = listView_.GetSelectedCount() != 0;
    GuiTools::EnableDialogItem(m_hWnd, IDC_BUTTONSKIP, enable);
    GuiTools::EnableDialogItem(m_hWnd, IDC_IGNORE, enable);
}

LRESULT CFileFormatCheckErrorDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/){
    size_t errorCount = model_.hasItemsWithStatus(FileFormatModelData::RowStatus::Error);
    if (errorCount) {
        std::string msg = str(IuStringUtils::FormatNoExcept(boost::locale::ngettext("%u error has not been fixed.", "%u errors have not been fixed.", errorCount)) % errorCount);
        GuiTools::LocalizedMessageBox(m_hWnd, IuCoreUtils::Utf8ToWstring(msg).c_str(), APP_NAME, MB_ICONERROR);
        return 0;
    }

    size_t skippedCount = 0;
    for (size_t i = 0; i < model_.getCount(); i++) {
        auto* row = model_.getDataByIndex(i);
        bool isSkipped = row->status() == FileFormatModelData::RowStatus::Skipped;

        row->file->setSkipped(isSkipped);
        if (isSkipped) {
            skippedCount++;
            result_.skippedFileIndexes.push_back(i);
        }
    }

    // EndDialog((model_.getCount() - skippedCount) ? IDOK : IDCANCEL);
    EndDialog(wID);
    return 0;
}

LRESULT CFileFormatCheckErrorDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    EndDialog(wID);
    
    return 0;
}

LRESULT CFileFormatCheckErrorDlg::OnSkip(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    int nIndex = -1;
    do {
        nIndex = listView_.GetNextItem(nIndex, LVNI_SELECTED);
        if (nIndex == -1) break;
        auto* sd = model_.getDataByIndex(nIndex);
        if (sd) {
            sd->setStatus(FileFormatModelData::RowStatus::Skipped);
            model_.notifyRowChanged(nIndex);
        }

    } while (true);

    return 0;
}

LRESULT CFileFormatCheckErrorDlg::OnErrorLogButtonClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CLogWindow* logWindow = ServiceLocator::instance()->logWindow();
    logWindow->Show();
    return 0;
}

LRESULT CFileFormatCheckErrorDlg::OnSkipAll(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    for (int i = 0; i < model_.getCount(); i++) {
        auto* sd = model_.getDataByIndex(i);
        if (sd) {
            sd->setStatus(FileFormatModelData::RowStatus::Skipped);
            model_.notifyRowChanged(i);
        }
    }

    listView_.Invalidate();
    return 0;
}

LRESULT CFileFormatCheckErrorDlg::OnIgnore(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    int nIndex = -1;
    do {
        nIndex = listView_.GetNextItem(nIndex, LVNI_SELECTED);
        if (nIndex == -1)
            break;
        auto* sd = model_.getDataByIndex(nIndex);
        if (sd) {
            sd->setStatus(FileFormatModelData::RowStatus::Ignore);
            model_.notifyRowChanged(nIndex);
        }

    } while (true);
    return 0;
}

LRESULT CFileFormatCheckErrorDlg::OnIgnoreAll(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    for (int i = 0; i < model_.getCount(); i++) {
        auto* sd = model_.getDataByIndex(i);
        if (sd) {
            sd->setStatus(FileFormatModelData::RowStatus::Ignore);
            model_.notifyRowChanged(i);
        }
    }

    listView_.Invalidate();
    return 0;
}

LRESULT CFileFormatCheckErrorDlg::OnListViewItemChanged(int idCtrl, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/) {
    if (idCtrl == IDC_FILELIST) {
        enableButtons();
    }
    return 0;
}

const FileFormatCheckResult& CFileFormatCheckErrorDlg::result() const {
    return result_;
}