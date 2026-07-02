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

#include "ThumbSettingsPage.h"

#include "3rdpart/GdiplusH.h"
#include <gdipluspixelformats.h>
#include "Core/i18n/Translator.h"
#include "Gui/GuiTools.h"
#include "Core/Images/Thumbnail.h"
#include "ThumbEditor.h"
#include "InputDialog.h"
#include "Func/IuCommonFunctions.h"
#include "Func/WinUtils.h"
#include "Core/Images/Utils.h"
#include "Core/Images/GdiPlusImage.h"
#include "Core/ServiceLocator.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "Gui/Helpers/DPIHelper.h"

CThumbSettingsPage::CThumbSettingsPage()
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    params_ = settings->DefaultImageUploadParams.getThumb();
    m_CatchFormChanges = false;
}


LRESULT CThumbSettingsPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    // Translating controls

    TRC(IDC_THUMBTEXTCHECKBOX, "Thumbnail text:");
    TRC(IDC_THUMBBACKGROUNDLABEL, "Background color:");
    TRC(IDC_WIDTHCHECKBOX, "Width:");
    TRC(IDC_HEIGHTCHECKBOX, "Height:");
    TRC(IDC_THUMBSCOMBOLABEL, "Thumbnail Preset:");
    TRC(IDC_EDITTHUMBNAILPRESET, "Edit");
    TRC(IDC_NEWTHUMBNAIL, "Create a Copy");
    TRC(IDC_THUMBFORMATLABEL, "Format:");
    TRC(IDC_THUMBQUALITYLABEL, "Quality:");

    ThumbBackground.SubclassWindow(GetDlgItem(IDC_THUMBBACKGROUND));
    //RECT rc = {13, 170, 290, 400};
    img.SubclassWindow(GetDlgItem(IDC_COMBOPREVIEW));
    //img.Create(m_hWnd, rc);
    img.loadImage(0);
    thumbsCombo_ = GetDlgItem(IDC_THUMBSCOMBO);
    tooltipControl_.Create(m_hWnd);

    SendDlgItemMessage(IDC_THUMBQUALITYSPIN, UDM_SETRANGE, 0, (LPARAM) MAKELONG((short)100, (short)1) );
    SetDlgItemText(IDC_THUMBTEXT, U2W(params_.Text));

    GuiTools::AddComboBoxItems(m_hWnd, IDC_THUMBFORMATLIST, 6, TR("Same format as image"),
        _T("JPEG"), _T("PNG"), _T("GIF"), _T("WebP"), _T("WebP (lossless)"));

    std::vector<CString> files;
    CString folder = IuCommonFunctions::GetDataFolder() + _T("\\Thumbnails\\");
    WinUtils::GetFolderFileList(files, folder, _T("*.xml"));
    for (const auto& fileName: files) {
        thumbsCombo_.AddString(WinUtils::GetOnlyFileName(fileName));
    }

    thumbTextMacrosButton_.Attach(GetDlgItem(IDC_THUMBMACROSES));
    CString tooltipText = TR("Macros list");
    CToolInfo tip(TTF_SUBCLASS, thumbTextMacrosButton_, 0, nullptr, const_cast<LPTSTR>(tooltipText.GetString()));
    tooltipControl_.AddTool(tip);

    thumbTextEdit_.Attach(GetDlgItem(IDC_THUMBTEXT));

    createResources();

    SendDlgItemMessage(IDC_THUMBTEXTCHECKBOX, BM_SETCHECK, params_.AddImageSize);
    thumbsCombo_.SelectString(-1, U2W(params_.TemplateName));
    SetDlgItemText(IDC_THUMBTEXT, U2W(params_.Text));
    SetDlgItemInt(IDC_THUMBQUALITYEDIT, params_.Quality);
    SendDlgItemMessage(IDC_THUMBFORMATLIST, CB_SETCURSEL, params_.Format);
    bool enableWidth = params_.ResizeMode == ThumbCreatingParams::trByWidth || params_.ResizeMode == ThumbCreatingParams::trByBoth;
    bool enableHeight = params_.ResizeMode == ThumbCreatingParams::trByHeight || params_.ResizeMode == ThumbCreatingParams::trByBoth;
    SendDlgItemMessage(IDC_WIDTHCHECKBOX, BM_SETCHECK, enableWidth);
    SendDlgItemMessage(IDC_HEIGHTCHECKBOX, BM_SETCHECK, enableHeight);
    SetDlgItemInt(IDC_WIDTHEDIT, params_.Width);
    SetDlgItemInt(IDC_HEIGHTEDIT, params_.Height);
    BOOL b;
    ThumbBackground.SetColor(params_.BackgroundColor);
    OnThumbComboChanged(0, 0, 0, b);
    ::EnableWindow(GetDlgItem(IDC_WIDTHEDIT), enableWidth);
    ::EnableWindow(GetDlgItem(IDC_HEIGHTEDIT), enableHeight);
    m_CatchFormChanges = true;
    return 1;  // Let the system set the focus
}

LRESULT CThumbSettingsPage::OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    createResources();
    showSelectedThumbnailPreview();
    return 0;
}

bool CThumbSettingsPage::apply()
{
    WtlGuiSettings* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    params_.AddImageSize = SendDlgItemMessage(IDC_THUMBTEXTCHECKBOX, BM_GETCHECK) == BST_CHECKED;
    TCHAR buf[256] = _T("\0");
    GetDlgItemText(IDC_THUMBSCOMBO, buf, 255);
    params_.TemplateName = W2U(buf);
    params_.Format = static_cast<ThumbCreatingParams::ThumbFormatEnum>(SendDlgItemMessage(IDC_THUMBFORMATLIST, CB_GETCURSEL));
    params_.Quality = GetDlgItemInt(IDC_THUMBQUALITYEDIT);
    params_.Text = W2U(GuiTools::GetWindowText(GetDlgItem(IDC_THUMBTEXT)));
    params_.Width = GetDlgItemInt(IDC_WIDTHEDIT);
    params_.Height = GetDlgItemInt(IDC_HEIGHTEDIT);

    bool setWidth = SendDlgItemMessage(IDC_WIDTHCHECKBOX, BM_GETCHECK) == BST_CHECKED;
    bool setHeight = SendDlgItemMessage(IDC_HEIGHTCHECKBOX, BM_GETCHECK) == BST_CHECKED;

    if (setWidth && setHeight) {
        params_.ResizeMode = ThumbCreatingParams::trByBoth;
    } else if (setHeight) {
        params_.ResizeMode = ThumbCreatingParams::trByHeight;
    } else {
        params_.ResizeMode = ThumbCreatingParams::trByWidth;
    }

    params_.Size = 0;
    params_.BackgroundColor = ThumbBackground.GetColor();
    settings->DefaultImageUploadParams.setThumb(params_);


    for (const auto& it: thumb_cache_) {
        it.second->saveToFile();
    }
    return TRUE;
}

LRESULT  CThumbSettingsPage::OnBnClickedNewThumbnail(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    CreateNewThumbnail();
    return 0;
}

LRESULT CThumbSettingsPage::OnThumbComboChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    showSelectedThumbnailPreview();
    return 0;
}

LRESULT CThumbSettingsPage::OnEditThumbnailPreset(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    std::string fileName = getSelectedThumbnailName();
    bool isStandartThumbnail = (fileName == "default" || fileName == "classic" || fileName == "classic2" || fileName == "flat"
        || fileName == "transp");

    if (isStandartThumbnail) {
        // User should create copy of default thumbnail preset first
        if (!CreateNewThumbnail()) {
            return 0;
        }
    }
    fileName = getSelectedThumbnailFileName();
    std::unique_ptr<Thumbnail> autoPtrThumb;
    Thumbnail *thumb = nullptr;
    const auto it = thumb_cache_.find(fileName);
    if (it != thumb_cache_.end()) {
        thumb = it->second.get();
    }

    if(!thumb) {
        autoPtrThumb = std::make_unique<Thumbnail>();
        thumb = autoPtrThumb.get();
        if(!thumb->loadFromFile(fileName))
        {
            GuiTools::LocalizedMessageBox(m_hWnd, TR("Couldn't load thumbnail preset!"));
            return 0;
        }
    }
    CThumbEditor dlg(thumb);
    if(dlg.DoModal(m_hWnd) == IDOK)
    {
        if (autoPtrThumb) {
            thumb_cache_[fileName] = std::move(autoPtrThumb);
        }

        showSelectedThumbnailPreview();
    }
    return 0;
}

std::string CThumbSettingsPage::getSelectedThumbnailFileName() const
{
    CString buf;
    int index = thumbsCombo_.GetCurSel();
    if (index < 0) {
        return {};
    }
    if (thumbsCombo_.GetLBText(index, buf) < 0) {
        return {};
    }
    CString thumbFileName = buf;
    Thumbnail thumb;
    CString folder = IuCommonFunctions::GetDataFolder() + _T("\\Thumbnails\\");
    return WCstringToUtf8(folder + thumbFileName+".xml");
}

std::string CThumbSettingsPage::getSelectedThumbnailName() const
{
    CString buf;
    int index = thumbsCombo_.GetCurSel();
    if (index < 0) {
        return {};
    }
    thumbsCombo_.GetLBText(index, buf);
    return WCstringToUtf8(buf);
}

void CThumbSettingsPage::showSelectedThumbnailPreview()
{
    using namespace Gdiplus;
    std::string fileName = getSelectedThumbnailFileName();
    if (fileName.empty()) {
        return;
    }

    std::unique_ptr<Thumbnail> autoPtrThumb;
    Thumbnail * thumb = nullptr;
    const auto it = thumb_cache_.find(fileName);

    if (it != thumb_cache_.end()) {
        thumb = it->second.get();
    }
    if(!thumb)
    {
        autoPtrThumb = std::make_unique<Thumbnail>();
        thumb = autoPtrThumb.get();

        if(!thumb->loadFromFile(fileName))
        {
            ServiceLocator::instance()->logger()->write(ILogger::logError, _T("CThumbSettingsPage"), TR("Couldn't load thumbnail preset!"));
            return;
        }
    }
    ImageConverter conv;
    conv.setThumbCreatingParams(params_);
    conv.setThumbnail(thumb);
    std::unique_ptr<Bitmap> bm = ImageUtils::BitmapFromResource(GetModuleHandle(0), MAKEINTRESOURCE(IDR_PNG2), _T("PNG"));
    if(!bm) {
        GuiTools::LocalizedMessageBox(m_hWnd, TR("Couldn't load thumbnail preset!"));
        return;
    }

    std::unique_ptr<Gdiplus::Bitmap> toUse = createSampleImage(400, 300);
    //Bitmap *toUse = bm->Clone(0, 0, bm->GetWidth(), bm->GetHeight(), PixelFormatDontCare);
    if (toUse) {
        GdiPlusImage source(toUse.release());
        std::unique_ptr<AbstractImage> result = conv.createThumbnail(&source, 50 * 1024, 1);
        if (result) {
            auto* resultImg = dynamic_cast<GdiPlusImage*>(result.get());
            if (resultImg) {
                img.loadImage(0, std::shared_ptr<Image>(resultImg->releaseBitmap()));
            }
        }
    }

    //delete toUse;
}

std::unique_ptr<Gdiplus::Bitmap> CThumbSettingsPage::createSampleImage(int width, int height) {
    using namespace Gdiplus;
    auto bm = std::make_unique<Bitmap>(width, height, PixelFormat32bppARGB);
    Graphics gr(bm.get());

    gr.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    gr.SetPixelOffsetMode(PixelOffsetModeHalf);
    ImageAttributes attr;
    attr.SetWrapMode(WrapModeTileFlipXY);
    Rect bounds(0, 0, width, height);
    LinearGradientBrush brush(bounds, Color(71, 124, 155), Color(104, 178, 112),
        LinearGradientModeVertical);

    gr.FillRectangle(&brush, bounds);

    auto logo = ImageUtils::BitmapFromResource(_Module.GetResourceInstance(), MAKEINTRESOURCE(IDR_PNG2), _T("PNG"));

    if (logo) {
        constexpr int horMargin = 5;
        constexpr int vertMargin = 5;
        int w = static_cast<int>(logo->GetWidth());
        int h = static_cast<int>(logo->GetHeight());
        Size sz = ImageUtils::ProportionalSize(Size(w, h), Size(width - horMargin * 2, height - vertMargin *2 ));

        Rect dst((width - sz.Width) / 2, (height - sz.Height) / 2, sz.Width, sz.Height);
        gr.DrawImage(logo.get(), dst, 0, 0, w, h, UnitPixel, &attr);
    }
    return bm;
}

bool CThumbSettingsPage::CreateNewThumbnail() {
    std::string fileName = getSelectedThumbnailFileName();
    if (fileName.empty()) {
        return false;
    }
    std::string newName = IuCoreUtils::ExtractFileNameNoExt(fileName) + "_copy";

    CInputDialog dlg(TR("Creating new thumbnail preset"), TR("Enter new thumbnail preset name:"), Utf8ToWCstring(newName));
    dlg.setForbiddenCharacters(_T("\\/:*?\"<>|"));

    if (dlg.DoModal(m_hWnd) == IDOK) {
        newName = WCstringToUtf8(dlg.getValue());
    } else {
        return false;
    }
    std::string srcFolder = IuCoreUtils::ExtractFilePath(fileName) + "/";
    std::string destination = srcFolder + newName + ".xml";
    if (IuCoreUtils::FileExists(destination)) {
        GuiTools::LocalizedMessageBox(m_hWnd, TR("Profile with such name already exists!"), APP_NAME, MB_ICONERROR);
        return false;
    }
    Thumbnail* thumb = nullptr;
    std::unique_ptr<Thumbnail> thumbPtr;
    if (thumb_cache_.count(fileName)) {
        thumb = thumb_cache_[fileName].get();
    } else {
        thumbPtr = std::make_unique<Thumbnail>();
        thumb = thumbPtr.get();
        thumb->loadFromFile(fileName);
    }
    std::string sprite = thumb->getSpriteFileName();
    thumb->setSpriteFileName(newName + '.' + IuCoreUtils::ExtractFileExt(sprite));
    if (!sprite.empty()) {
        std::string newSpriteName = srcFolder + newName + '.' + IuCoreUtils::ExtractFileExt(sprite);
        if (!IuCoreUtils::CopyFileToDest(sprite, newSpriteName)) {
            LOG(ERROR) << "Unable to copy file: " << std::endl << std::endl << "Source: " << sprite << std::endl << "Destination: " << newSpriteName;
            return false;
        }
    }

    if (!IuCoreUtils::CopyFileToDest(fileName, destination) ) {
        LOG(ERROR) << "Unable to copy file: " << std::endl << std::endl << "Source: " << fileName << std::endl << "Destination: " << destination;
        return false;
    }

    if (!thumb->saveToFile(destination)) {
        LOG(ERROR) << "Unable to save thumbnail template to file '" << destination << "'";
        return false;
    }
    GuiTools::AddComboBoxItems(m_hWnd, IDC_THUMBSCOMBO, 1, U2WC(newName));
    thumbsCombo_.SelectString(-1, U2W(newName));
    GuiTools::EnableDialogItem(m_hWnd, IDC_EDITTHUMBNAILPRESET, true);
    showSelectedThumbnailPreview();
    return true;
}

void CThumbSettingsPage::createResources() {
    const int dpi = DPIHelper::GetDpiForDialog(m_hWnd);
    int iconWidth = DPIHelper::GetSystemMetricsForDpi(SM_CXSMICON, dpi);
    int iconHeight = DPIHelper::GetSystemMetricsForDpi(SM_CYSMICON, dpi);

    if (iconInfo_) {
        iconInfo_.DestroyIcon();
    }
    iconInfo_.LoadIconWithScaleDown(MAKEINTRESOURCE(IDI_ICONINFO), iconWidth, iconHeight);
    thumbTextMacrosButton_.SetIcon(iconInfo_);
}

LRESULT CThumbSettingsPage::OnThumbTextCheckboxClick(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    ThumbTextCheckboxChange();
    showSelectedThumbnailPreview();
    return 0;
}

void CThumbSettingsPage::ThumbTextCheckboxChange()
{
    bool bChecked = SendDlgItemMessage(IDC_THUMBTEXTCHECKBOX, BM_GETCHECK) == BST_CHECKED;
    ::EnableWindow(GetDlgItem(IDC_THUMBTEXT), bChecked);
    thumbTextMacrosButton_.EnableWindow(bChecked);
    params_.AddImageSize = bChecked;
    params_.AddImageSize = bChecked;

}

LRESULT CThumbSettingsPage::OnThumbTextChange(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
   if(!m_CatchFormChanges) return 0;
    params_.Text = W2U(GuiTools::GetWindowText(GetDlgItem(IDC_THUMBTEXT)));
    showSelectedThumbnailPreview();
    return 0;
}

LRESULT CThumbSettingsPage::OnWidthEditChange(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    if (!m_CatchFormChanges) {
        return 0;
    }

    bool setWidth = SendDlgItemMessage(IDC_WIDTHCHECKBOX, BM_GETCHECK) == BST_CHECKED;
    bool setHeight = SendDlgItemMessage(IDC_HEIGHTCHECKBOX, BM_GETCHECK) == BST_CHECKED;

    if (!setWidth && !setHeight) {
        setWidth = true;
    }
    params_.Width = GetDlgItemInt(IDC_WIDTHEDIT);
    if (params_.Width < 0) {
        params_.Width = ThumbCreatingParams::DEFAULT_THUMB_WIDTH;
    }
    params_.Height = GetDlgItemInt(IDC_HEIGHTEDIT);
    if (params_.Height < 0) {
        params_.Height = 0;
    }

    if (!params_.Width && !params_.Height) {
        params_.Width = ThumbCreatingParams::DEFAULT_THUMB_WIDTH;
    }

    if (setWidth && setHeight) {
        params_.ResizeMode = ThumbCreatingParams::trByBoth;
    }
    else if (setHeight) {
        params_.ResizeMode = ThumbCreatingParams::trByHeight;
    }
    else {
        params_.ResizeMode = ThumbCreatingParams::trByWidth;
    }

    ::EnableWindow(GetDlgItem(IDC_WIDTHEDIT), setWidth);
    ::EnableWindow(GetDlgItem(IDC_HEIGHTEDIT), setHeight);

    showSelectedThumbnailPreview();
    return 0;
}

LRESULT CThumbSettingsPage::OnThumbMacrosButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    const std::vector<std::pair<CString, CString>> items {
        { _T("%width%"), TR("image width")},
        { _T("%height%"), TR("image height") },
        { _T("%size%"), TR("file size") },
    };

    RECT rc {};
    ::GetWindowRect(hWndCtl, &rc);
    POINT menuOrigin { rc.left, rc.bottom };

    CMenu macrosMenu;

    int id = 1;
    macrosMenu.CreatePopupMenu();

    for (const auto& item : items) {
        CString title = item.first + _T(" - ") + item.second;
        macrosMenu.AppendMenu(MF_STRING, id++, title);
    }

    TPMPARAMS excludeArea;
    ZeroMemory(&excludeArea, sizeof(excludeArea));
    excludeArea.cbSize = sizeof(excludeArea);
    excludeArea.rcExclude = rc;

    int result = macrosMenu.TrackPopupMenuEx(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY, menuOrigin.x, menuOrigin.y, m_hWnd, &excludeArea);
    if (result && (result - 1 < items.size())) {
        thumbTextEdit_.ReplaceSel(items[result - 1].first, TRUE);
    }

    return 0;
}
