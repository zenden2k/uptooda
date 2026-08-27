#include "ServersCheckerDlg.h"

#include "Gui/Dialogs/LogWindow.h"
#include "Core/Utils/CoreUtils.h"
#include "Gui/GuiTools.h"
#include "Func/WinUtils.h"
#include "Core/Utils/CryptoUtils.h"
#include "Core/Upload/UploadManager.h"
#include "ServersChecker.h"
#include "Core/ServiceLocator.h"
#include "Core/Settings/BasicSettings.h"
#include "Gui/Components/MyFileDialog.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "Func/WebUtils.h"

namespace ServersListTool
{

CServersCheckerDlg::CServersCheckerDlg(WtlGuiSettings* settings, UploadEngineManager* uploadEngineManager, UploadManager* uploadManager, CMyEngineList* engineList,
                    std::shared_ptr<INetworkClientFactory> factory) :
                    model_(engineList), listView_(&model_), networkClientFactory_(std::move(factory)), settings_(settings)
{
    uploadEngineManager_ = uploadEngineManager;
    uploadManager_ = uploadManager;
    engineList_ = engineList;
    contextMenuItemId_ = -1;
}

LRESULT CServersCheckerDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    CenterWindow(); // center the dialog on the screen
    DlgResize_Init(false, true, 0); // resizable dialog without "griper"
    DoDataExchange(FALSE);

    // set icons
    icon_.LoadIcon(MAKEINTRESOURCE(IDR_MAINFRAME), ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR);
    SetIcon(icon_, TRUE);
    iconSmall_ .LoadIcon(MAKEINTRESOURCE(IDR_MAINFRAME),::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    SetIcon(iconSmall_, FALSE);
    
    listView_.Init();

    withAccountsRadioButton_.SetCheck(BST_CHECKED);
    checkImageServersCheckBox_.SetCheck(BST_CHECKED);
    listView_.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

    SetDlgItemText(IDC_TOOLFILEEDIT, U2W(settings_->testFileName));
    SetDlgItemText(IDC_TESTURLEDIT, U2W(settings_->testUrl));

    serversChecker_ = std::make_unique<ServersChecker>(&model_, uploadManager_, networkClientFactory_);
    serversChecker_->setOnFinishedCallback([this] { processFinished(); });
    return TRUE;
}

LRESULT CServersCheckerDlg::OnContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
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
    LV_HITTESTINFO hti = {};
    hti.pt = ClientPoint;
    listView_.HitTest(&hti);

    if (hti.iItem >= 0) {
        ServerData* sd = model_.getDataByIndex(hti.iItem);

        if (sd) {
            CMenu menu;
            menu.CreatePopupMenu();
            menu.AppendMenu(MF_STRING, ID_COPYDIRECTURL, _T("Copy direct url"));
            menu.EnableMenuItem(ID_COPYDIRECTURL, sd->directUrl().empty() ? MF_DISABLED : MF_ENABLED);

            menu.AppendMenu(MF_STRING, ID_COPYTHUMBURL, _T("Copy thumb url"));
            menu.EnableMenuItem(ID_COPYTHUMBURL, sd->thumbUrl().empty() ? MF_DISABLED : MF_ENABLED);

            menu.AppendMenu(MF_STRING, ID_COPYVIEWURL, _T("Copy view url"));
            menu.EnableMenuItem(ID_COPYVIEWURL, sd->viewurl().empty() ? MF_DISABLED : MF_ENABLED);

            contextMenuItemId_ = hti.iItem;
            menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, ScreenPoint.x, ScreenPoint.y, m_hWnd);
        } 
    }

    return 0;
}

void CServersCheckerDlg::validateSettings() {
    CString fileName = GuiTools::GetWindowText(GetDlgItem(IDC_TOOLFILEEDIT));
    if (!WinUtils::FileExists(fileName)) {
        throw ValidationException(CString(_T("Test file not found.")) + _T("\r\n") + fileName);
    }

    bool checkUrlShorteners = checkUrlShortenersCheckBox_.GetCheck() == BST_CHECKED;
    CString url = GuiTools::GetWindowText(GetDlgItem(IDC_TESTURLEDIT));
    if (checkUrlShorteners){
        if (url.IsEmpty()) {
            throw ValidationException(_T("URL should not be empty!"));
        }
        if (!WebUtils::IsValidUrl(url)) {
            throw ValidationException(_T("Invalid URL"));
        }
    }
}

LRESULT CServersCheckerDlg::OnOK(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    try {
        validateSettings();
    } catch (const ValidationException& ex) {
        GuiTools::LocalizedMessageBox(m_hWnd, ex.errors_[0].Message, APP_NAME, MB_ICONERROR);
        return 0;
    }
    CString fileName = GuiTools::GetWindowText(GetDlgItem(IDC_TOOLFILEEDIT));
    std::string utf8FileName = W2U(fileName);
    sourceFileHash_ = U2W(IuCoreUtils::CryptoUtils::CalcMD5HashFromFile(W2U(fileName)));
    CString report = _T("Source file: ") + GetFileInfo(fileName, &sourceFileInfo_);
    SetDlgItemText(IDC_TOOLSOURCEFILE, report);
    ::EnableWindow(GetDlgItem(IDOK), false);
    ::EnableWindow(GetDlgItem(IDCANCEL), false);
    GuiTools::ShowDialogItem(m_hWnd, IDC_STOPBUTTON, true);

    bool useAccounts = withAccountsRadioButton_.GetCheck() == BST_CHECKED || alwaysWithAccountsRadioButton_.GetCheck() == BST_CHECKED;
    bool onlyAccs = alwaysWithAccountsRadioButton_.GetCheck() == BST_CHECKED;

    bool checkImageServers = checkImageServersCheckBox_.GetCheck() == BST_CHECKED;
    bool checkFileServers = checkFileServersCheckBox_.GetCheck() == BST_CHECKED;
    bool checkUrlShorteners = checkUrlShortenersCheckBox_.GetCheck() == BST_CHECKED;

    serversChecker_->setCheckFileServers(checkFileServers);
    serversChecker_->setCheckImageServers(checkImageServers);
    serversChecker_->setCheckUrlShorteners(checkUrlShorteners);
    serversChecker_->setOnlyAccs(onlyAccs);
    serversChecker_->setUseAccounts(useAccounts);
    model_.resetData();

    CString url = GuiTools::GetWindowText(GetDlgItem(IDC_TESTURLEDIT));
    std::string utf8Url = W2U(url);
    settings_->testFileName = utf8FileName;
    settings_->testUrl = utf8Url;
    loadingAnimation_.ShowWindow(SW_SHOW);
    serversChecker_->start(utf8FileName, utf8Url);
    return 0;
}

LRESULT CServersCheckerDlg::OnCancel(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    if (isRunning()) {
        stop();
    } else {
        EndDialog(wID);
    }
    return 0;
}

LRESULT CServersCheckerDlg::OnSkip(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    int nIndex = -1;
    do {
        nIndex = listView_.GetNextItem(nIndex, LVNI_SELECTED);
        if (nIndex == -1) break;
        ServerData* sd = model_.getDataByIndex(nIndex);
        if (sd) {
            sd->skip = !sd->skip;
            model_.notifyRowChanged(nIndex);
        }

    } while (true);

    return 0;
}

LRESULT CServersCheckerDlg::OnBrowseButton(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    IMyFileDialog::FileFilterArray filters = {
        {_T("All files"), _T("*.*")}
    };

    auto dlg = MyFileDialogFactory::createFileDialog(m_hWnd, _T(""), _T(""), filters, false);
    if (dlg->DoModal(m_hWnd) != IDOK) {
        return 0;
    }

    SetDlgItemText(IDC_TOOLFILEEDIT, dlg->getFile());
    return 0;
}

void CServersCheckerDlg::stop()
{
    serversChecker_->stop();
}

bool CServersCheckerDlg::isRunning() const
{
    return serversChecker_->isRunning();
}

void CServersCheckerDlg::processFinished() {
    ServiceLocator::instance()->taskRunner()->runInGuiThread([this] {
        ::EnableWindow(GetDlgItem(IDOK), true);
        ::EnableWindow(GetDlgItem(IDCANCEL), true);
        GuiTools::ShowDialogItem(m_hWnd, IDC_STOPBUTTON, false);
        loadingAnimation_.ShowWindow(SW_HIDE);
    });
}

LRESULT CServersCheckerDlg::OnErrorLogButtonClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CLogWindow* logWindow = ServiceLocator::instance()->logWindow();
    logWindow->Show();
    return 0;
}

LRESULT CServersCheckerDlg::OnSkipAll(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    for (int i = 0; i < engineList_->count(); i++) {
        ServerData* sd = model_.getDataByIndex(i);
        if (sd) {
            sd->skip = !sd->skip;
            model_.notifyRowChanged(i);
        }
    }

    listView_.Invalidate();
    return 0;
}

LRESULT CServersCheckerDlg::OnCopyDirectUrl(WORD, WORD, HWND, BOOL&) {
    ServerData* sd = model_.getDataByIndex(contextMenuItemId_);
    if (sd) {
        std::string directUrl = sd->directUrl();
        if (!directUrl.empty()) {
            WinUtils::CopyTextToClipboard(U2W(directUrl));
        }
    }

    return 0;
}

LRESULT CServersCheckerDlg::OnCopyThumbUrl(WORD, WORD, HWND, BOOL&) {
    ServerData* sd = model_.getDataByIndex(contextMenuItemId_);
    if (sd) {
        std::string thumbUrl = sd->thumbUrl();
        if (!thumbUrl.empty()) {
            WinUtils::CopyTextToClipboard(U2W(thumbUrl));
        }
    }

    return 0;
}

LRESULT CServersCheckerDlg::OnCopyViewUrl(WORD, WORD, HWND, BOOL&) {
    ServerData* sd = model_.getDataByIndex(contextMenuItemId_);
    if (sd) {
        std::string viewUrl = sd->viewurl();
        if (!viewUrl.empty()) {
            WinUtils::CopyTextToClipboard(U2W(viewUrl));
        }
    }

    return 0;
}

LRESULT CServersCheckerDlg::OnBnClickedStopButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    serversChecker_->stop();

    return 0;
}

}

