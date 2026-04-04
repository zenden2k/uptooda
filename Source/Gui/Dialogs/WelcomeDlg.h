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
#ifndef WELCOMEDLG_H
#define WELCOMEDLG_H


#pragma once
// CWelcomeDlg
#include "atlheaders.h"
#include "resource.h"       // main symbols
#include "Gui/WizardCommon.h"
#include "Gui/Controls/HyperLinkControl.h"
#include "Gui/Controls/MyImage.h"
#include "Func/Common.h"
#include <atlcrack.h>
#include <memory>

#define ID_VIEWHISTORY 36220
class CWelcomeDlg :
    public CDialogImpl<CWelcomeDlg>,
    public CWinDataExchange<CWelcomeDlg>,
    public CWizardPage
{
public:
     CWelcomeDlg();
     ~CWelcomeDlg() override = default;
    enum { IDD = IDD_WELCOMEDLG };

    BEGIN_MSG_MAP(CWelcomeDlg)
        MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
        MSG_WM_CTLCOLORDLG(OnCtlColorMsgDlg)
        MSG_WM_CTLCOLORSTATIC(OnCtlColorMsgDlg)
        ///    MESSAGE_HANDLER( WM_ERASEBKGND, OnEraseBkg)
        COMMAND_ID_HANDLER(IDC_CLIPBOARD,  OnClipboardClick)
        COMMAND_ID_HANDLER(IDC_ADDFOLDER,  OnAddFolderClick)
        COMMAND_HANDLER(IDC_SCREENSHOT, BN_CLICKED, OnBnClickedScreenshot)
        COMMAND_HANDLER(IDC_RECORDSCREENDLG, BN_CLICKED, OnBnClickedRecordScreenDlg)
        COMMAND_HANDLER(IDC_RECORDSCREEN, BN_CLICKED, OnBnClickedRecordScreen)
        COMMAND_HANDLER(IDC_ADDIMAGES, BN_CLICKED, OnBnClickedAddimages)
        COMMAND_HANDLER(IDC_ADDVIDEO, BN_CLICKED, OnBnClickedAddvideo)
        COMMAND_HANDLER(IDC_REGIONPRINT, BN_CLICKED, OnBnClickedRegionPrint)
        COMMAND_HANDLER(IDC_LASTREGIONSCREENSHOT, BN_CLICKED, OnBnClickedLastRegionScreenshot)
        COMMAND_HANDLER(IDC_MEDIAFILEINFO, BN_CLICKED, OnBnClickedMediaInfo)
        COMMAND_HANDLER(IDC_DOWNLOADIMAGES, BN_CLICKED, OnBnClickedDownloadImages)
        COMMAND_HANDLER(IDC_ADDFILES, BN_CLICKED, OnBnClickedAddFiles)
        COMMAND_HANDLER(IDC_REUPLOADIMAGES, BN_CLICKED, OnBnClickedReuploadImages)
        COMMAND_HANDLER(IDC_SHORTENURL, BN_CLICKED, OnBnClickedShortenUrl)
        MESSAGE_HANDLER(WM_CLIPBOARDUPDATE, OnClipboardUpdate) // Windows Vista and later
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_MY_DPICHANGED, OnDPICHanged)
        REFLECT_NOTIFICATIONS()
    END_MSG_MAP()

    BEGIN_DDX_MAP(WelcomeDlg)
        DDX_CONTROL(IDC_LISTBOX/*IDC_LINKSCONTROL*/, ListBox)
        DDX_CONTROL(IDC_LEFTBITMAP/*IDC_LINKSCONTROL*/, LeftImage)
        DDX_CONTROL(IDC_STATICLOGO/*IDC_LINKSCONTROL*/, LogoImage)
        DDX_CONTROL(IDC_TITLE /*IDC_LINKSCONTROL*/, appNameImage_)
    END_DDX_MAP()
    // Handler prototypes:
    //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDPICHanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnBnClickedScreenshot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedRecordScreenDlg(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedRecordScreen(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedAddvideo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedAddimages(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedRegionPrint(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedLastRegionScreenshot(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedMediaInfo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedDownloadImages(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedAddFiles(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnClipboardClick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnAddFolderClick(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedReuploadImages(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnBnClickedShortenUrl(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT OnClipboardUpdate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    void clipboardUpdated();
    void lastRegionAvailabilityChanged(bool available);
    LRESULT OnEraseBkg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    CHyperLinkControl ListBox;
    LRESULT OnCtlColorMsgDlg(HDC hdc, HWND hwndChild);
    CBrushHandle br;
    bool OnShow() override;
    void SetInitialFocus() override;
    void updateImages(int dpiX, int dpiY);
    void fillHyperLinksControl(int dpiX, int dpiY);
    bool QuickRegionPrint;
private:
    CMyImage LeftImage, LogoImage, appNameImage_;
    CFont NewFont, font2_;
    std::unique_ptr<Gdiplus::Bitmap> createLeftImage(int dpiX, int dpiY);
};



#endif // WELCOMEDLG_H
