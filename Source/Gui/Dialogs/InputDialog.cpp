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

#include "InputDialog.h"

#include "3rdpart/GdiPlusH.h"
#include "Gui/GuiTools.h"

namespace {

void OffsetControl(HWND control, int offset) {
    RECT rc;

    GetWindowRect(control, &rc);
    MapWindowPoints(0, GetParent(control), reinterpret_cast<LPPOINT>(&rc), 2);
    SetWindowPos(control, 0, rc.left, rc.top + offset, 0, 0, SWP_NOSIZE);
}

}

// CInputDialog
CInputDialog::CInputDialog(const CString& title, const CString& descr, const CString& defaultValue,
                           const CString& image)
{
    title_ = title;
    description_ = descr;
    value_ = defaultValue;
    image_ = image;
}

LRESULT CInputDialog::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    DlgResize_Init(false, true, 0); // resizable dialog without "griper"
    SetWindowText(title_);
    SetDlgItemText(IDC_DESCRIPTIONLABEL, description_);
    SetDlgItemText(IDC_VALUEEDIT, value_);
    TRC(IDCANCEL, "Cancel");
    TRC(IDOK, "OK");

    imgControl.SubclassWindow(GetDlgItem(IDC_IMAGE));

    if (!image_.IsEmpty())
    {
        Gdiplus::Image img(image_);
        int imgHeight = img.GetHeight() + 5;
        RECT rc;
        GetWindowRect(&rc);
        imgControl.SetWindowPos(0, 0, 0, img.GetWidth() + 2, imgHeight + 2, SWP_NOMOVE);
        imgControl.loadImage(image_);
        /*SetWindowPos(0, 0, 0, rc.right - rc.left, rc.bottom - rc.top + imgHeight, SWP_NOMOVE);
        OffsetControl(GetDlgItem(IDC_VALUEEDIT), imgHeight );
        OffsetControl(GetDlgItem(IDCANCEL), imgHeight );
        OffsetControl(GetDlgItem(IDOK), imgHeight );*/
    } else {
        imgControl.ShowWindow(SW_HIDE);
        CRect temp { 0, 0, 100, 100 };
        MapDialogRect(&temp);

        CRect rc;
        GetClientRect(&rc);

        m_ptMinTrackSize.x = rc.Width();
        m_ptMinTrackSize.y = temp.Height();
        GuiTools::SetClientRect(m_hWnd, rc.Width(), temp.Height());
    }
    CenterWindow(GetParent());
    return 0;  // Let the system set the focus
}

LRESULT CInputDialog::OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    CString val = GuiTools::GetWindowText(GetDlgItem(IDC_VALUEEDIT));
    if (val.FindOneOf(forbiddenCharacters_) >= 0) {
        LocalizedMessageBox(TR("Text contains forbidden characters!"), APP_NAME, MB_ICONERROR);
        return 0;
    }
    value_ = val;
    EndDialog(wID);
    return 0;
}

LRESULT CInputDialog::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(wID);
    return 0;
}

CString CInputDialog::getValue() const
{
    return value_;
}

void CInputDialog::setForbiddenCharacters(CString chars) {
    forbiddenCharacters_ = chars;
}
