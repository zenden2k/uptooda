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

#include "ClearHistoryDlg.h"

#include "Gui/GuiTools.h"

CClearHistoryDlg::CClearHistoryDlg() : value_(HistoryClearPeriod::OlderThan30Days)
{
}

LRESULT CClearHistoryDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    SetWindowText(TR("Clear History"));
    TRC(IDCANCEL, "Cancel");
    TRC(IDOK, "OK");
    TRC(IDC_ALLTIMERADIO, "All the time");
    TRC(IDC_CURRENTMONTHRADIO, "Older than 30 days");
    TRC(IDC_DESCRIPTIONLABEL, "Please choose period:");
    GuiTools::SetCheck(m_hWnd, IDC_ALLTIMERADIO, true);
    CenterWindow(GetParent());
    return 0;  // Let the system set the focus
}

LRESULT CClearHistoryDlg::OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    bool allTime = GuiTools::GetCheck(m_hWnd, IDC_ALLTIMERADIO);
    value_ = allTime ? HistoryClearPeriod::ClearAll : HistoryClearPeriod::OlderThan30Days;
    EndDialog(wID);
    return 0;
}

LRESULT CClearHistoryDlg::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(wID);
    return 0;
}

HistoryClearPeriod CClearHistoryDlg::GetValue() const
{
    return value_;
}
