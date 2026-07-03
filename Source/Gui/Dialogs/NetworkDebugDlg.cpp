#include "NetworkDebugDlg.h"

#include "Gui/Dialogs/LogWindow.h"
#include "Core/Utils/CoreUtils.h"
#include "Gui/GuiTools.h"
#include "Func/WinUtils.h"
#include "Core/ServiceLocator.h"
#include "Core/Settings/BasicSettings.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "Func/WebUtils.h"
#include "Core/3rdpart/Hexdump.hpp"
#include "Gui/Components/MyFileDialog.h"
#include "Core/Utils/IOException.h"

CNetworkDebugDlg::CNetworkDebugDlg()
    : listView_(&model_) {
    contextMenuItemId = -1;
}

LRESULT CNetworkDebugDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    SetWindowText(TR("Network Debugger"));
    CenterWindow(); // center the dialog on the screen

    DlgResize_Init(true, true, 0); // resizable dialog without "griper"
    DoDataExchange(FALSE);
    TRC(IDC_CLEARLOGBUTTON, "Clear log");
    TRC(IDC_SAVETOFILEBTN, "Save to file");
    TRC(IDC_DECODERESPONSEBUTTON, "Decode response");

    splitterCtrl_.SubclassWindow(GetDlgItem(IDC_SPLITTER));
    splitterCtrl_.SetSplitterPanes(listView_.m_hWnd, detailsEdit_.m_hWnd);
    
    // set icons
    icon_ = static_cast<HICON>(::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME),
        IMAGE_ICON, ::GetSystemMetrics(SM_CXICON), ::GetSystemMetrics(SM_CYICON), LR_DEFAULTCOLOR));
    SetIcon(icon_, TRUE);
    iconSmall_ = static_cast<HICON>(::LoadImage(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_MAINFRAME),
        IMAGE_ICON, ::GetSystemMetrics(SM_CXSMICON), ::GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR));
    SetIcon(iconSmall_, FALSE);

    LOGFONT logFont;
    WinUtils::StringToFont(_T("Courier New,9,,204"), &logFont);

    detailsEditFont_.CreateFontIndirect(&logFont);
    detailsEdit_.SetFont(detailsEditFont_);

   // listView_.SubclassWindow(GetDlgItem(IDC_DEBUGLIST));
    listView_.Init();

    listView_.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER, LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);

    CMessageLoop* pLoop = _Module.GetMessageLoop();
    ATLASSERT(pLoop != NULL);
    pLoop->AddMessageFilter(this);

    return TRUE;
}

LRESULT CNetworkDebugDlg::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    // unregister message filtering
    CMessageLoop* pLoop = _Module.GetMessageLoop();
    ATLASSERT(pLoop != NULL);
    pLoop->RemoveMessageFilter(this);
    return 0;
}

LRESULT CNetworkDebugDlg::OnClose(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    ShowWindow(SW_HIDE);
    return 0;
}

LRESULT CNetworkDebugDlg::OnContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
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
        /* FileFormatModelData* sd = model_.getDataByIndex(hti.iItem);

        if (sd) {
            CMenu menu;
            menu.CreatePopupMenu();
            /* menu.AppendMenu(MF_STRING, ID_COPYDIRECTURL, _T("Copy direct url"));
            menu.EnableMenuItem(ID_COPYDIRECTURL, sd->directUrl().empty() ? MF_DISABLED : MF_ENABLED);

            menu.AppendMenu(MF_STRING, ID_COPYTHUMBURL, _T("Copy thumb url"));
            menu.EnableMenuItem(ID_COPYTHUMBURL, sd->thumbUrl().empty() ? MF_DISABLED : MF_ENABLED);

            menu.AppendMenu(MF_STRING, ID_COPYVIEWURL, _T("Copy view url"));
            menu.EnableMenuItem(ID_COPYVIEWURL, sd->viewurl().empty() ? MF_DISABLED : MF_ENABLED);
            
            contextMenuItemId = hti.iItem;
            menu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, ScreenPoint.x, ScreenPoint.y, m_hWnd);
        } */
    }

    return 0;
}


LRESULT CNetworkDebugDlg::OnErrorLogButtonClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    CLogWindow* logWindow = ServiceLocator::instance()->logWindow();
    logWindow->Show();
    return 0;
}

LRESULT CNetworkDebugDlg::OnClearLogButtonClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    model_.clear();
    detailsEdit_.SetWindowText(_T(""));
    return 0;
}

LRESULT CNetworkDebugDlg::OnListViewItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& /*bHandled*/) {
    if (idCtrl == IDC_DEBUGLIST) {
        auto* p = reinterpret_cast<LPNMLISTVIEW>(pnmh);
        if (p->iItem >= 0) {

            auto* item = const_cast<NetworkDebugModelData*>(model_.getDataByIndex(p->iItem));
            bool hexDump = false;
            if (item->type == CURLINFO_SSL_DATA_OUT || item->type == CURLINFO_SSL_DATA_IN) {
                hexDump = true;
            } else {
                
                showText(item->hasDecoded() ? item->getDecoded() : item->data);
                return 0;
            }
            if (hexDump) {
                std::stringstream ss;
                ss << CustomHexdump<8, true>(item->data.data(), item->data.length());
                detailsEdit_.SetWindowText(IuCoreUtils::Utf8ToWstring(ss.str()).c_str());
            }
        }
    }
    return 0;
}

LRESULT CNetworkDebugDlg::OnDecodeResponseButtonClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    int nCurItem;

    if ((nCurItem = listView_.GetNextItem(-1, LVNI_ALL | LVNI_SELECTED)) < 0)
        return {};

    auto* item = model_.getDataByIndex(nCurItem);
    if (!item) {
        return 0;
    }
    std::string s = const_cast<NetworkDebugModelData*>(item)->getDecoded();
    showText(s);
    return 0;
}

LRESULT CNetworkDebugDlg::OnSaveToFileButtonClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    IMyFileDialog::FileFilterArray filters {
        { TR("Text files"), _T("*.txt;*.log") },
        { TR("All files"), _T("*.*") }
    };

    auto dlg = MyFileDialogFactory::createFileDialog(m_hWnd, {}, TR("Save log file"), filters, false, false);
    dlg->setDefaultExtension(_T("txt"));

    if (dlg->DoModal(m_hWnd) != IDOK) {
        return 0;
    }

    try {
        model_.saveToFile(W2U(dlg->getFile()));
    } catch (const IOException& ex) {
        GuiTools::LocalizedMessageBox(m_hWnd, U2WC(ex.what()), TR("Error"), MB_ICONERROR);
    }

    return 0;
}

BOOL CNetworkDebugDlg::PreTranslateMessage(MSG* pMsg) {
    return CWindow::IsDialogMessage(pMsg);
}

bool CNetworkDebugDlg::showText(const std::string& src)
{
    std::wstring s = IuCoreUtils::Utf8ToWstring(src);
    bool hexDump = false;
    for (int i = 0; i < 10 && i < s.length(); i++) {
        if (!iswprint(s[i]) && !iswspace(s[i])) {
            hexDump = true;
            break;
        }
    }

    if (!hexDump) {
        detailsEdit_.SetWindowText(s.c_str());
    } else {
        std::stringstream ss;
        ss << CustomHexdump<8, true>(src.data(), src.length());
        detailsEdit_.SetWindowText(IuCoreUtils::Utf8ToWstring(ss.str()).c_str());
    }
    return true;
}
