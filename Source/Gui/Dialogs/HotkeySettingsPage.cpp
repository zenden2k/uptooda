/*

    Uptooda - free application for uploading images/files to the Internet

    Copyright 2007-2025 Sergey Svistunov (zenden2k@gmail.com)

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

*/

#include "HotkeySettingsPage.h"

#include "TraySettings.h"
#include "HotkeyEditor.h"
#include "Gui/GuiTools.h"
#include "Gui/Dialogs/ScreenRecorderWindow.h"
#include "Gui/Helpers/DPIHelper.h"

LRESULT CHotkeySettingsPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    m_HotkeyList.m_hWnd = GetDlgItem(IDC_HOTKEYLIST);
    TRC(IDC_EDITHOTKEY, "Edit hotkey...");
    TRC(IDC_ATTENTION, "Attention! Global hotkeys are active only when tray icon is shown.");
    attentionLabelFont_ = GuiTools::MakeLabelBold(GetDlgItem(IDC_ATTENTION));
    m_HotkeyList.AddColumn(TR("Action"),0);
    m_HotkeyList.AddColumn(TR("Local"),1);
    m_HotkeyList.AddColumn(TR("Global"),2);
    m_HotkeyList.SetExtendedListViewStyle(LVS_EX_FULLROWSELECT);
    initListView();
    hotkeyList = settings->Hotkeys;

    for(int i=0; i < int(hotkeyList.size())-1; i++)
    {
        m_HotkeyList.AddItem(i, 0, hotkeyList[i+1].GetDisplayName());
        m_HotkeyList.AddItem(i, 1, hotkeyList[i+1].localKey.toString());
        m_HotkeyList.AddItem(i, 2, hotkeyList[i+1].globalKey.toString());
    }
    updateEditButtonState();
    return 1;  // Let the system set the focus
}

LRESULT CHotkeySettingsPage::OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    initListView();
    return 0;
}

LRESULT CHotkeySettingsPage::OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(wID);
    return 0;
}

LRESULT CHotkeySettingsPage::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(wID);
    return 0;
}
bool CHotkeySettingsPage::apply()
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    if (settings->Hotkeys != hotkeyList) {
        settings->Hotkeys_changed = true;
    }
    settings->Hotkeys = hotkeyList;
    return true;
}

LRESULT CHotkeySettingsPage::OnHotkeylistNmDblclk(LPNMHDR pnmh)
{
    auto* ia = reinterpret_cast<LPNMITEMACTIVATE>(pnmh);
    if (ia) {
        EditHotkey(ia->iItem);
    }

    return 0;
}

LRESULT CHotkeySettingsPage::OnListViewItemChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
    auto* lpStateChange = reinterpret_cast<NMLISTVIEW*>(pnmh);
    if ((lpStateChange->uNewState ^  lpStateChange->uOldState) & LVIS_SELECTED) {
        updateEditButtonState();
    }
    return 0;
}

void CHotkeySettingsPage::EditHotkey(int index)
{
    if(index < 0) return ;

    CHotkeyEditor hotKeyDlg;
    hotKeyDlg.m_data = hotkeyList[index+1];
    if(hotKeyDlg.DoModal(m_hWnd)==IDOK)
    {
        hotkeyList[index+1] = hotKeyDlg.m_data ;
        m_HotkeyList.AddItem(index,1,hotKeyDlg.m_data.localKey.toString());
        m_HotkeyList.AddItem(index,2,hotKeyDlg.m_data.globalKey.toString());
    }
}

void CHotkeySettingsPage::initListView() {
    const int dpi = DPIHelper::GetDpiForDialog(m_hWnd);
    m_HotkeyList.SetColumnWidth(0, MulDiv(175, dpi, USER_DEFAULT_SCREEN_DPI));
    m_HotkeyList.SetColumnWidth(1, MulDiv(82, dpi, USER_DEFAULT_SCREEN_DPI));
    m_HotkeyList.SetColumnWidth(2, MulDiv(82, dpi, USER_DEFAULT_SCREEN_DPI));
}

void CHotkeySettingsPage::updateEditButtonState() {
    const int index = m_HotkeyList.GetSelectedIndex();
    GuiTools::EnableDialogItem(m_hWnd, IDC_EDITHOTKEY, index >= 0);
}

LRESULT CHotkeySettingsPage::OnEditHotkeyBnClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    int index = m_HotkeyList.GetSelectedIndex();
    if (index >= 0) {
        EditHotkey(index);
    }

    return 0;
}

LRESULT CHotkeySettingsPage::OnContextMenu(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HWND hwnd = reinterpret_cast<HWND>(wParam);
    POINT ClientPoint, ScreenPoint;

    if(lParam == -1)
    {
        ClientPoint.x = 0;
        ClientPoint.y = 0;

        int nCurItem = m_HotkeyList.GetNextItem(-1, LVNI_ALL | LVNI_SELECTED);
        if (nCurItem >= 0) {
            CRect rc;
            if (m_HotkeyList.GetItemRect(nCurItem, &rc, LVIR_BOUNDS)) {
                ClientPoint = rc.CenterPoint();
            }
        }
        ScreenPoint = ClientPoint;
        ::ClientToScreen(hwnd, &ScreenPoint);
    }
    else
    {
        ScreenPoint.x = GET_X_LPARAM(lParam);
        ScreenPoint.y = GET_Y_LPARAM(lParam);
        ClientPoint = ScreenPoint;
        ::ScreenToClient(hwnd, &ClientPoint);
    }
    CMenu contextMenu;
    contextMenu.CreatePopupMenu();
    GuiTools::InsertMenu(contextMenu, 0, IDC_EDITHOTKEY, TR("Change"));
    contextMenu.SetMenuDefaultItem(IDC_EDITHOTKEY, FALSE);
    GuiTools::InsertMenu(contextMenu, 1, IDM_CLEARHOTKEY, TR("Clear"));
    GuiTools::InsertMenu(contextMenu, 2, IDM_CLEARALLHOTKEYS, TR("Clear all"));
    contextMenu.TrackPopupMenu( TPM_LEFTALIGN | TPM_LEFTBUTTON, ScreenPoint.x, ScreenPoint.y, m_hWnd);
    return 0;
}


LRESULT CHotkeySettingsPage::OnClearHotkey(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    int index = m_HotkeyList.GetSelectedIndex();
    if(index)
    {
        hotkeyList[index+1].Clear();
        m_HotkeyList.SetItem(index, 1, LVIF_TEXT,_T(""),0,0,0,0);
        m_HotkeyList.SetItem(index, 2, LVIF_TEXT,_T(""),0,0,0,0);
    }
    return 0;
}

LRESULT CHotkeySettingsPage::OnClearAllHotkeys(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    if (hotkeyList.empty()) {
        return 0;
    }

    for (size_t i = 0; i < hotkeyList.size() - 1; i++) {
        hotkeyList[i + 1].Clear();
        m_HotkeyList.SetItem(i, 1, LVIF_TEXT,_T(""), 0, 0, 0, 0);
        m_HotkeyList.SetItem(i, 2, LVIF_TEXT,_T(""), 0, 0, 0, 0);
    }
    return 0;
}
