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

#include "ThumbEditor.h"

#include "Core/Settings/WtlGuiSettings.h"
#include "Core/Images/Thumbnail.h"
#include "Gui/GuiTools.h"

CThumbEditor::CThumbEditor(Thumbnail *thumb)
{
    thumb_ = thumb;
    memset(&ThumbFont, 0, sizeof(ThumbFont));
}

LRESULT CThumbEditor::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    TRC(IDCANCEL, "Cancel");
    TRC(IDOK, "OK");
    TRC(IDC_THUMBSTROKECOLOR, "Stroke color:");
    TRC(IDC_COLORSGROUP, "Colors");
    TRC(IDC_FRAMECOLORLABEL, "Frame color:");
    TRC(IDC_GRADIENTCOLOR1LABEL, "Gradient color 1:");
    TRC(IDC_GRADIENTCOLOR2LABEL, "Gradient color 2:");
    TRC(IDC_DRAWFRAME, "Draw frame");
    TRC(IDC_FRAMEWIDTHLABEL, "Width:");
    TRC(IDC_THUMBPARAMS, "Text Settings:");
    TRC(IDC_THUMBTEXTCOLORLABEL, "Text color:");
    TRC(IDC_THUMBSTROKECOLORLABEL, "Stroke color:");
    TRC(IDC_THUMBTEXTLABEL, "Text:");
    TRC(IDC_THUMBFONT, "Font...");
    TRC(IDC_ADDFILESIZE, "Add text");

    SetWindowText(TR("Thumbnail Preset Editor"));
    FrameColor.SubclassWindow(GetDlgItem(IDC_FRAMECOLOR));
    Color1.SubclassWindow(GetDlgItem(IDC_COLOR1));
    Color2.SubclassWindow(GetDlgItem(IDC_COLOR2));
    StrokeColor.SubclassWindow(GetDlgItem(IDC_THUMBTEXTCOLOR2));
    ThumbTextColor.SubclassWindow(GetDlgItem(IDC_THUMBTEXTCOLOR));
    CenterWindow(GetParent());

    LoadParams();
    ShowTextCheckboxChanged();
    DrawFrameCheckboxChanged();
    return 0;  // Let the system set the focus
}

LRESULT CThumbEditor::OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    /*Settings.ThumbSettings.FrameColor = FrameColor.GetColor();
    Settings.ThumbSettings.ThumbColor1 = Color1.GetColor();
    Settings.ThumbSettings.ThumbColor2 = Color2.GetColor();
    Settings.ThumbSettings.ThumbTextColor=ThumbTextColor.GetColor();
    Settings.ImageSettings.TextColor=TextColor.GetColor();
    Settings.ImageSettings.StrokeColor=StrokeColor.GetColor();*/

    SaveParams();
    //Settings.ThumbSettings.TextOverThumb = SendDlgItemMessage(IDC_TEXTOVERTHUMB2, BM_GETCHECK);
    EndDialog(wID);
    return 0;
}

LRESULT CThumbEditor::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(wID);
    return 0;
}

void CThumbEditor::LoadParams()
{
    WinUtils::StringToFont(_T("Tahoma,7,b,204"), &ThumbFont);
    if(thumb_->existsParam("Font"))
    {
        std::string font = thumb_->getParamString("Font");
        CString wideText = Utf8ToWCstring(font);
        WinUtils::StringToFont(wideText, &ThumbFont);
    }

    if(thumb_->existsParam("FrameWidth"))
        SetDlgItemInt(IDC_FRAMEWIDTH, thumb_->getParam("FrameWidth"));
    else
        ::EnableWindow(GetDlgItem(IDC_FRAMEWIDTH), false);

    if(thumb_->existsParam("DrawFrame"))
    {
        bool DrawFrame = thumb_->getParam("DrawFrame")!=0;
        SendDlgItemMessage(IDC_DRAWFRAME, BM_SETCHECK, DrawFrame);
    }
    else
    {
        GuiTools::EnableNextN(GetDlgItem(IDC_DRAWFRAME), 3, false);
        ::EnableWindow(GetDlgItem(IDC_DRAWFRAME), false);
    }

    bool DrawText = thumb_->getParam("DrawText")!=0;
    SendDlgItemMessage(IDC_ADDFILESIZE, BM_SETCHECK, DrawText);

    if(thumb_->existsParam("FrameColor"))
        FrameColor.SetColor(WinUtils::RGB2COLORREF(thumb_->getColor("FrameColor")));
    else
    {
        FrameColor.EnableWindow(false);
        ::EnableWindow(GetDlgItem(IDC_FRAMECOLORLABEL), false);
    }

    if(thumb_->existsParam("GradientColor1"))
        Color1.SetColor(WinUtils::RGB2COLORREF(thumb_->getColor("GradientColor1")));
    else
    {
        Color1.EnableWindow(false);
        ::EnableWindow(GetDlgItem(IDC_GRADIENTCOLOR1LABEL), false);
    }

    if(thumb_->existsParam("TextColor"))
    {
        ThumbTextColor.SetColor(WinUtils::RGB2COLORREF(thumb_->getColor("TextColor")));
    }
    else
    {
        ThumbTextColor.EnableWindow(false);
        ::EnableWindow(GetDlgItem(IDC_THUMBTEXTCOLORLABEL), false);
    }

    if(thumb_->existsParam("StrokeColor"))
    {
        StrokeColor.SetColor(WinUtils::RGB2COLORREF(thumb_->getColor("StrokeColor")));
    }
    else
    {
        StrokeColor.EnableWindow(false);
        ::EnableWindow(GetDlgItem(IDC_THUMBSTROKECOLORLABEL), false);
    }


    if(thumb_->existsParam("GradientColor2"))
    Color2.SetColor(WinUtils::RGB2COLORREF(thumb_->getColor("GradientColor2")));
    else
    {
        Color2.EnableWindow(false);
        ::EnableWindow(GetDlgItem(IDC_GRADIENTCOLOR2LABEL), false);
    }
    SetDlgItemText(IDC_THUMBTEXT, Utf8ToWCstring(thumb_->getParamString("Text")));
}

void CThumbEditor::SaveParams()
{
    if(FrameColor.IsWindowEnabled())
        thumb_->setColor("FrameColor", WinUtils::COLORREF2RGB(FrameColor.GetColor()));
    if(Color1.IsWindowEnabled())
        thumb_->setColor("GradientColor1", WinUtils::COLORREF2RGB(Color1.GetColor()));
    if(ThumbTextColor.IsWindowEnabled())
        thumb_->setColor("TextColor", WinUtils::COLORREF2RGB(ThumbTextColor.GetColor()));

    if(StrokeColor.IsWindowEnabled())
        thumb_->setColor("StrokeColor", WinUtils::COLORREF2RGB(StrokeColor.GetColor()));

    if(Color2.IsWindowEnabled())
    thumb_->setColor("GradientColor2", WinUtils::COLORREF2RGB(Color2.GetColor()));

    if(thumb_->existsParam("DrawFrame"))
    {
        bool DrawFrame = SendDlgItemMessage(IDC_DRAWFRAME, BM_GETCHECK)!=0;
        thumb_->setParam("DrawFrame", DrawFrame);
    }
    if(thumb_->existsParam("FrameWidth"))
        thumb_->setParam("FrameWidth", GetDlgItemInt(IDC_FRAMEWIDTH));

    CString text  = GuiTools::GetWindowText(GetDlgItem(IDC_THUMBTEXT));
    bool AddText  = SendDlgItemMessage(IDC_ADDFILESIZE, BM_GETCHECK)!=0;

    thumb_->setParamString("Text", WCstringToUtf8(text));
    if(thumb_->existsParam("DrawText"))
        thumb_->setParam("DrawText", AddText);

    //if(thumb_->existsParam("Font"))
    {
        CString res;
        WinUtils::FontToString(&ThumbFont, res);
        thumb_->setParamString("Font", WCstringToUtf8(res));
    }

}

LRESULT  CThumbEditor::OnShowTextCheckboxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    ShowTextCheckboxChanged();
    return 0;
}

void CThumbEditor::ShowTextCheckboxChanged()
{
    bool bChecked = SendDlgItemMessage(IDC_ADDFILESIZE, BM_GETCHECK)!=0;
    GuiTools::EnableNextN(GetDlgItem(IDC_ADDFILESIZE), 7, bChecked);
}

LRESULT CThumbEditor::OnDrawFrameCheckboxClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    DrawFrameCheckboxChanged();
    return 0;
}

void CThumbEditor::DrawFrameCheckboxChanged()
{
    bool bChecked = SendDlgItemMessage(IDC_DRAWFRAME, BM_GETCHECK)!=0;
    GuiTools::EnableNextN(GetDlgItem(IDC_DRAWFRAME), 3, bChecked);
}

LRESULT CThumbEditor::OnFontSelect(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    // Font selection dialog
    CFontDialog dlg(&ThumbFont);
    dlg.DoModal(m_hWnd);
    return 0;
}
