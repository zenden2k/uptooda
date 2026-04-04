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

#include "Gui/GuiTools.h"

#include <cmath>
#include <cstdarg>

#include <ShObjIdl.h>
#include <strsafe.h>
#include <dwmapi.h>
#include <initguid.h>
#include <oleacc.h>
#include <objbase.h>
#include <uiautomation.h>

#include "Func/WinUtils.h"
#include "Core/ServiceLocator.h"
#include "Core/i18n/Translator.h"
#include "3rdpart/GdiplusH.h"
#include "Func/Library.h"
#include "Gui/Helpers/DPIHelper.h"

// Hack: initguid.h must be included before oleacc.h but oleacc.h
#undef INITGUID
#include <InitGuid.h>
DEFINE_GUID(CLSID_AccPropServices, 0xb5f8350b, 0x0548, 0x48b1, 0xa6, 0xee, 0x88, 0xbd, 0x00, 0xb4, 0xa5, 0xe7);
DEFINE_GUID(PROPID_ACC_NAME, 0x608d3df8, 0x8128, 0x4aa7, 0xa4, 0x28, 0xf5, 0x5e, 0x49, 0x26, 0x72, 0x91);

namespace GuiTools
{
int AddComboBoxItem(HWND hDlg, int itemId, LPCTSTR item) {
    return ::SendDlgItemMessage(hDlg, itemId, CB_ADDSTRING, 0, reinterpret_cast<LPARAM>(item));
}

bool AddComboBoxItems(HWND hDlg, int itemId, int itemCount, ...) {
    bool result = true;
    std::va_list args;
    va_start(args, itemCount);
    for (int i = 0; i < itemCount; ++i) {
        if (AddComboBoxItem(hDlg, itemId, va_arg(args, LPCTSTR)) < 0)
            result = false;
    }
    va_end(args);
    return result;
}

bool IsChecked(HWND dlg, int id) {
    return ::SendDlgItemMessage(dlg, id,BM_GETCHECK, 0, 0) == BST_CHECKED;
}

void GetCheck(HWND dlg, int id, bool& check) {
    check = ::SendDlgItemMessage(dlg, id,BM_GETCHECK, 0, 0) == BST_CHECKED;
}

void SetCheck(HWND dlg, int id, bool check) {
    ::SendDlgItemMessage(dlg, id,BM_SETCHECK, check, 0);
}

HFONT MakeLabelBold(HWND Label) {
    HFONT Font = reinterpret_cast<HFONT>(SendMessage(Label, WM_GETFONT, 0, 0));

    if (!Font) return nullptr;

    LOGFONT alf;

    if (!(::GetObject(Font, sizeof(LOGFONT), &alf) == sizeof(LOGFONT))) return nullptr;

    alf.lfWeight = FW_BOLD;

    HFONT NewFont = CreateFontIndirect(&alf);
    SendMessage(Label,WM_SETFONT, reinterpret_cast<WPARAM>(NewFont), MAKELPARAM(false, 0));
    CWindowDC dc(0);
    alf.lfHeight = -MulDiv(13, GetDeviceCaps(dc, LOGPIXELSY), 72);
    return NewFont;
}

HFONT MakeLabelItalic(HWND Label) {
    HFONT Font = reinterpret_cast<HFONT>(SendMessage(Label, WM_GETFONT, 0, 0));

    if (!Font) return nullptr;

    LOGFONT alf;

    if (!(::GetObject(Font, sizeof(LOGFONT), &alf) == sizeof(LOGFONT))) return nullptr;

    alf.lfItalic = 1;

    HFONT NewFont = CreateFontIndirect(&alf);
    SendMessage(Label,WM_SETFONT, reinterpret_cast<WPARAM>(NewFont),MAKELPARAM(false, 0));
    return NewFont;
}

void EnableNextN(HWND Control, int n, bool Enable) {
    for (int i = 0; i < n; i++) {
        Control = GetNextWindow(Control, GW_HWNDNEXT);
        EnableWindow(Control, Enable);
    }
}

bool InsertMenu(HMENU hMenu, int pos, UINT id, LPCTSTR szTitle, HBITMAP bm){
    MENUITEMINFO MenuItem {};

    MenuItem.cbSize = sizeof(MenuItem);
    if(szTitle)
        MenuItem.fType = MFT_STRING;
    else MenuItem.fType = MFT_SEPARATOR;
    MenuItem.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
    if(bm)
        MenuItem.fMask |= MIIM_BITMAP;
    MenuItem.wID = id;
    MenuItem.hbmpItem = bm;
    MenuItem.dwTypeData = const_cast<LPWSTR>(szTitle);
    MenuItem.cch = lstrlen(szTitle);
    return InsertMenuItem(hMenu, pos, TRUE, &MenuItem)!=0;
}

void FillRectGradient(HDC hdc, const RECT& FillRect, COLORREF start, COLORREF finish, bool Horizontal) {
        RECT rectFill;
        float fStep;            //The size of each band in pixels
        HBRUSH hBrush;
        int i;  // Loop index

        float r, g, b;
        int n = 256;
        //FillRect.bottom--;
        COLORREF c;

        if(!Horizontal)
            fStep = (FillRect.bottom - FillRect.top) / 256.0f;
        else
            fStep = (FillRect.right - FillRect.left) / 256.0f;

        if( fStep < 1)
        {
            fStep = 1;
            if(!Horizontal)
                n = FillRect.bottom - FillRect.top;
            else
                n = (FillRect.right - FillRect.left);
        }

        r = static_cast<float>(GetRValue(finish)-GetRValue(start))/(n-1);

        g = static_cast<float>(GetGValue(finish)-GetGValue(start))/(n-1);

        b = static_cast<float>(GetBValue(finish)-GetBValue(start))/(n-1);

        //Begin paint
        for (i = 0; i < n; i++)
        {
            // Horizontal or vertical gradient
            if(!Horizontal)
                SetRect(&rectFill, FillRect.left, int((i * fStep)+FillRect.top),
                FillRect.right+1, int(FillRect.top+(i+1) * fStep));
            else
                SetRect(&rectFill, static_cast<int>(FillRect.left+(i * fStep)), FillRect.top,
                int(FillRect.left+((i+1) * fStep)), FillRect.bottom+1);
            if(i == n-1)
                c = finish;
            else
                c = RGB((int)GetRValue(start)+(r*i/**zR*/),(int)GetGValue(start)+(g*i/*zG*/),(int)GetBValue(start)+(b*i/**zB*/));

            hBrush=CreateSolidBrush(c);

            ::FillRect(hdc, &rectFill, hBrush);

            DeleteObject(hBrush);
        }
}
/*
* Fails on armv8
bool SelectDialogFilter(LPTSTR szBuffer, int nMaxSize, int nCount, LPCTSTR szName, LPCTSTR szFilter,...) {
    *szBuffer = 0;
    LPCTSTR *pszName, *pszFilter;
    pszName = &szName;
    pszFilter = &szFilter;

    for (int i = 0; i < nCount; i++) {
        int nLen = lstrlen(*pszName);
        lstrcpy(szBuffer, *pszName);
        szBuffer[nLen] = 0;
        szBuffer += nLen + 1;

        nLen = lstrlen(*pszFilter);
        lstrcpy(szBuffer, *pszFilter);
        szBuffer[nLen] = 0;
        szBuffer += nLen + 1;
        pszName += 2;
        pszFilter += 2;
    }
    *szBuffer = 0;
    return true;
}
*/
// Converts pixels to Win32 dialog units
int dlgX(int WidthInPixels) {
    LONG units = GetDialogBaseUnits();
    short baseunitX = LOWORD(units);
    return WidthInPixels * baseunitX / 4;
}

// Converts pixels to Win32 dialog units
int dlgY(int HeightInPixels) {
    LONG units = GetDialogBaseUnits();
    short baseunitY = HIWORD(units);
    return HeightInPixels * baseunitY / 8;
}

CString GetWindowText(HWND wnd) {
    int len = GetWindowTextLength(wnd);
    if (!len) {
        return {};
    }
    CString buf;
    GetWindowText(wnd, buf.GetBuffer(len + 1), len + 1);
    buf.ReleaseBuffer(-1);
    return buf;
}

CString GetDlgItemText(HWND dialog, int controlId) {
    return GetWindowText(::GetDlgItem(dialog, controlId ) );
}

/* MakeFontBold
    MakeFontUnderLine

    -----------------------
    These functions create bold/underlined fonts based on given font
*/
HFONT MakeFontBold(HFONT font) {
    if ( !font ) {
        return 0;
    }

    LOGFONT alf;

    bool ok = ::GetObject(font, sizeof(LOGFONT), &alf) == sizeof(LOGFONT);

    if(!ok) return 0;

    alf.lfWeight = FW_BOLD;

    HFONT NewFont = CreateFontIndirect(&alf);
    return NewFont;
}

HFONT MakeFontUnderLine(HFONT font)
{
    if(!font) return 0;

    LOGFONT alf;

    bool ok = ::GetObject(font, sizeof(LOGFONT), &alf) == sizeof(LOGFONT);

    if(!ok) return 0;

    alf.lfUnderline = 1;
    HFONT NewFont = CreateFontIndirect(&alf);

    return NewFont;
}

HFONT MakeFontSmaller(HFONT font) {
    if ( !font ) {
        return 0;
    }

    LOGFONT alf;
    bool ok = ::GetObject(font, sizeof(LOGFONT), &alf) == sizeof(LOGFONT);
    if ( !ok ) {
        return 0;
    }

    alf.lfHeight = GetFontSize( 12 );
    HFONT NewFont = CreateFontIndirect(&alf);
    return NewFont;
}

HFONT MakeFontBigger(HFONT font) {
    if ( !font ) {
        return 0;
    }

    LOGFONT alf;
    bool ok = ::GetObject(font, sizeof(LOGFONT), &alf) == sizeof(LOGFONT);
    if ( !ok ) {
        return 0;
    }

    alf.lfHeight += GetFontSize( 5 );
    alf.lfWeight = FW_BOLD;
    HFONT NewFont = CreateFontIndirect(&alf);
    return NewFont;
}
int GetFontSize(int nFontHeight) {
    HDC dc = ::GetDC(nullptr);
    int res =  - MulDiv( nFontHeight, 72, GetDeviceCaps(dc, LOGPIXELSY));
    ReleaseDC(nullptr, dc);
    return res;
}

int GetFontHeight(int nFontSize) {
    HDC dc = ::GetDC(nullptr);
    int res =  - MulDiv(nFontSize, GetDeviceCaps(dc, LOGPIXELSY), 72);
    ReleaseDC(nullptr, dc);
    return res;
}

HFONT GetSystemDialogFont(UINT dpi) {
    NONCLIENTMETRICS ncm;
    ncm.cbSize = sizeof(NONCLIENTMETRICS);

    if (dpi) {
        // Windows 10+
        static Library user32(L"user32.dll");
        if (user32) {
            typedef BOOL (WINAPI *SystemParametersInfoForDpiFunc)(
                UINT uiAction,
                UINT uiParam,
                PVOID pvParam,
                UINT fWinIni,
                UINT dpi
            );

            static SystemParametersInfoForDpiFunc pSystemParametersInfoForDpi =
                user32.GetProcAddress<SystemParametersInfoForDpiFunc>("SystemParametersInfoForDpi");

            if (pSystemParametersInfoForDpi) {
                // Windows 10 Anniversary Update (1607) и новее
                if (pSystemParametersInfoForDpi(SPI_GETNONCLIENTMETRICS,
                                               sizeof(NONCLIENTMETRICS),
                                               &ncm,
                                               0,
                                               dpi)) {
                    return CreateFontIndirect(&ncm.lfMessageFont);
                }
            }
        }
    }

    // Fallback
    if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS,
                            sizeof(NONCLIENTMETRICS),
                            &ncm,
                            0)) {
        CClientDC dc(NULL);
        int systemDpi = dc.GetDeviceCaps(LOGPIXELSX);

        if (dpi && systemDpi != dpi) {
            ncm.lfMessageFont.lfHeight = MulDiv(ncm.lfMessageFont.lfHeight,
                                               (int)systemDpi,
                                               USER_DEFAULT_SCREEN_DPI);
        }

        return CreateFontIndirect(&ncm.lfMessageFont);
    }

    return NULL;
}

int ScreenBPP(){
    int iRet = 0;
    HDC hdc = GetDC(NULL);
    if (hdc != NULL) {
        iRet = GetDeviceCaps(hdc, BITSPIXEL);
        ReleaseDC(NULL, hdc);
    }
    return iRet;
}

BOOL Is32BPP(){
    return ScreenBPP() >= 32;
}

RECT GetDialogItemRect(HWND dialog, int itemId) {
    HWND control = ::GetDlgItem( dialog, itemId );
    RECT controlRect={0,0,0,0};
    GetWindowRect(control, &controlRect );
    MapWindowPoints(0 /*means desktop*/, dialog, reinterpret_cast<LPPOINT>(&controlRect), 2);
    return controlRect;
}

void ShowDialogItem(HWND dlg, int itemId, bool show) {
    ::ShowWindow(GetDlgItem(dlg, itemId), show? SW_SHOW : SW_HIDE);
}

RECT AutoSizeStaticControl(HWND control) {
    CString text = GuiTools::GetWindowText(control);
    CClientDC dc(control);
    HFONT font = reinterpret_cast<HFONT>(SendMessage(control, WM_GETFONT, 0, 0));

    SIZE textSize;
    HGDIOBJ oldFont = SelectObject(dc, font);
    GetTextExtentPoint32(dc, text, text.GetLength(), &textSize);
    SetWindowPos(control, nullptr, 0, 0, textSize.cx, textSize.cy, SWP_NOMOVE|SWP_NOZORDER);
    RECT controlRect;
    GetWindowRect(control, &controlRect);
    MapWindowPoints(0 /*means desktop*/, GetParent(control), reinterpret_cast<LPPOINT>(&controlRect), 2);
    SelectObject(dc, oldFont);
    ::InvalidateRect(control, 0, FALSE);
    return controlRect;
}

void EnableDialogItem(HWND dlg, int itemId, bool enable) {
    ::EnableWindow( ::GetDlgItem( dlg, itemId ), enable );
}

IconInfo GetIconInfo(HICON hIcon)
{
    IconInfo myinfo;
    ZeroMemory(&myinfo, sizeof(myinfo));

    ICONINFO info;
    ZeroMemory(&info, sizeof(info));

    BOOL bRes = FALSE;

    bRes = GetIconInfo(hIcon, &info);
    if(!bRes)
        return myinfo;

    BITMAP bmp;
    ZeroMemory(&bmp, sizeof(bmp));

    if(info.hbmColor)
    {
        const int nWrittenBytes = GetObject(info.hbmColor, sizeof(bmp), &bmp);
        if(nWrittenBytes > 0)
        {
            myinfo.nWidth = bmp.bmWidth;
            myinfo.nHeight = bmp.bmHeight;
            myinfo.nBitsPerPixel = bmp.bmBitsPixel;
        }
    }
    else if(info.hbmMask)
    {
        // Icon has no color plane, image data stored in mask
        const int nWrittenBytes = GetObject(info.hbmMask, sizeof(bmp), &bmp);
        if(nWrittenBytes > 0)
        {
            myinfo.nWidth = bmp.bmWidth;
            myinfo.nHeight = bmp.bmHeight / 2;
            myinfo.nBitsPerPixel = 1;
        }
    }

    if(info.hbmColor)
        DeleteObject(info.hbmColor);
    if(info.hbmMask)
        DeleteObject(info.hbmMask);

    return myinfo;
}

int GetWindowLeft(HWND Wnd)
{
    RECT WindowRect = {0,0,0,0};

    GetWindowRect(Wnd,&WindowRect);
    HWND Parent = GetParent(Wnd);
    ScreenToClient(Parent, reinterpret_cast<LPPOINT>(&WindowRect));
    return WindowRect.left;
}

void GetScreenBounds(RECT& rect) {
    std::vector<RECT> monitorsRects;

    auto monitorEnumProc = [](HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData) -> BOOL {
        if (lprcMonitor && dwData) {
            auto* rects = reinterpret_cast<decltype(&monitorsRects)>(dwData);
            rects->push_back(*lprcMonitor);
        }
        return TRUE;
    };

    EnumDisplayMonitors(nullptr, nullptr, monitorEnumProc, reinterpret_cast<LPARAM>(&monitorsRects));

    CRect result;
    for (const auto& bounds : monitorsRects) {
        result.UnionRect(result, &bounds);
    }

    rect = result;
}

HRGN CloneRegion(HRGN source)
{
    HRGN resultRgn = CreateRectRgn(0, 0, 0, 0);
    CombineRgn(resultRgn, source, resultRgn, RGN_OR);
    return resultRgn;
}

HWND CreateToolTipForWindow(HWND hwnd, const CString& text) {
    HWND hwndTT = ::CreateWindowEx(WS_EX_TOPMOST, TOOLTIPS_CLASS, NULL,
        WS_POPUP | TTS_NOPREFIX | TTS_ALWAYSTIP,
        CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
        hwnd, NULL, _Module.GetModuleInstance(),NULL);

    ::SetWindowPos(hwndTT, HWND_TOPMOST, 0, 0, 0, 0,
        SWP_NOMOVE | SWP_NOSIZE | SWP_NOACTIVATE);

    RECT clientRect;
    ::GetClientRect(hwnd, &clientRect);

    TOOLINFO ti = {};
    ti.cbSize   = sizeof(TOOLINFO);
    ti.uFlags   = TTF_SUBCLASS;
    ti.hwnd     = hwnd;
    ti.hinst    = _Module.GetModuleInstance();
    auto textBuffer = std::make_unique<TCHAR[]>(text.GetLength() + 1);
    StringCchCopy(textBuffer.get(), text.GetLength() + 1, text);
    ti.lpszText = textBuffer.get();
    ti.rect  = clientRect;

    // Associate the tooltip with the "tool" window.
    SendMessage(hwndTT, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti));

    return hwndTT;
}

void AddToolTip(HWND hwndTT, HWND hwnd, const CString& text) {
    // Set up "tool" information. In this case, the "tool" is the entire parent window.
    RECT clientRect;
    ::GetClientRect(hwnd, &clientRect);
    TOOLINFO ti = { 0 };
    ti.cbSize = sizeof(TOOLINFO);
    ti.uFlags = TTF_SUBCLASS;
    ti.hwnd = hwnd;
    ti.hinst = _Module.GetModuleInstance();
    auto textBuffer = std::make_unique<TCHAR[]>(text.GetLength() + 1);
    StringCchCopy(textBuffer.get(), text.GetLength() + 1, text);
    ti.lpszText = textBuffer.get();
    ti.rect = clientRect;

    // Associate the tooltip with the "tool" window.
    SendMessage(hwndTT, TTM_ADDTOOL, 0, reinterpret_cast<LPARAM>(&ti));
}

CHARFORMAT LogFontToCharFormat(const LOGFONT & lf)
{
    CHARFORMAT cf;
    cf.cbSize = sizeof(CHARFORMAT);
    cf.dwMask =  CFM_FACE | CFM_SIZE | CFM_CHARSET
        | CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE | CFM_STRIKEOUT | CFM_OFFSET;
    cf.dwEffects = 0;

    if (lf.lfWeight >= FW_BOLD)
    {
        cf.dwEffects |= CFE_BOLD;
    }

    if (lf.lfUnderline)
    {
        cf.dwEffects |= CFE_UNDERLINE;
    }

    if (lf.lfItalic)
    {
        cf.dwEffects |= CFE_ITALIC;
    }

    if (lf.lfStrikeOut)
    {
        cf.dwEffects |= CFE_STRIKEOUT;
    }

    //temporary create DC
    CDC dc;
    dc.CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
    cf.yHeight = 20*long( 0.5 + fabs(double(72*lf.lfHeight)/dc.GetDeviceCaps(LOGPIXELSY)));
    dc.DeleteDC();

    cf.yOffset = 0;
    cf.bCharSet = lf.lfCharSet;
    cf.bPitchAndFamily = lf.lfPitchAndFamily;
    _tcscpy_s(cf.szFaceName, LF_FACESIZE, lf.lfFaceName);
    return cf;
}

LOGFONT CharFormatToLogFont(const CHARFORMAT & cf)
{
    LOGFONT lf;
    lf.lfCharSet = cf.bCharSet;
    lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
    lf.lfEscapement = 0;
    lf.lfOrientation = 0;
    lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
    lf.lfQuality = DEFAULT_QUALITY;
    lf.lfPitchAndFamily = DEFAULT_PITCH;

    if ( (cf.dwEffects & CFE_BOLD) == CFE_BOLD) {
        lf.lfWeight = FW_BOLD;
    }

    CDC dc;
    dc.CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
    lf.lfHeight = -MulDiv(cf.yHeight/20, dc.GetDeviceCaps(LOGPIXELSY), 72);
    dc.DeleteDC();

    lf.lfUnderline = ( (cf.dwEffects & CFE_UNDERLINE) == CFE_UNDERLINE);
    lf.lfStrikeOut = ( (cf.dwEffects & CFE_STRIKEOUT) == CFE_STRIKEOUT);
    lf.lfItalic = ( (cf.dwEffects & CFE_ITALIC) == CFE_ITALIC);

    lf.lfWidth = 0;
    _tcscpy_s(lf.lfFaceName, LF_FACESIZE, cf.szFaceName);

    return lf;
}

HICON LoadSmallIcon(int resourceId, int dpi) {
    const int iconWidth = DPIHelper::GetSystemMetricsForDpi(SM_CXSMICON, dpi);
    const int iconHeight = DPIHelper::GetSystemMetricsForDpi(SM_CYSMICON, dpi);

    HICON result = nullptr;
    LoadIconWithScaleDown(_Module.GetResourceInstance(), MAKEINTRESOURCE(resourceId), iconWidth, iconHeight, &result);
    return result;
}

HICON LoadBigIcon(int resourceId, int dpi) {
    const int iconWidth = DPIHelper::GetSystemMetricsForDpi(SM_CXICON, dpi);
    const int iconHeight = DPIHelper::GetSystemMetricsForDpi(SM_CYICON, dpi);

    HICON result = nullptr;
    LoadIconWithScaleDown(_Module.GetResourceInstance(), MAKEINTRESOURCE(resourceId), iconWidth, iconHeight, &result);
    return result;
}

void RemoveWindowStyleEx(HWND hWnd, DWORD styleEx) {
    LONG oldStyle = ::GetWindowLong(hWnd, GWL_EXSTYLE);
    ::SetWindowLong(hWnd, GWL_EXSTYLE, oldStyle & ~styleEx);
}

int LocalizedMessageBox(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption, UINT uType) {
    if (ServiceLocator::instance()->translator()->isRTL()) {
        uType |= MB_RTLREADING;
    }
    return MessageBox(hWnd, lpText, lpCaption, uType);
}

BOOL SetClientRect(HWND hWnd, int width, int height) {
    if (!IsWindow(hWnd)) {
        return FALSE;
    }

    RECT windowRect;
    GetWindowRect(hWnd, &windowRect);

    RECT clientRect = { 0, 0, width, height };

    DWORD style = GetWindowLong(hWnd, GWL_STYLE);
    DWORD exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);
    BOOL hasMenu = (GetMenu(hWnd) != NULL);


    if (!AdjustWindowRectEx(&clientRect, style, hasMenu, exStyle)) {
        return FALSE;
    }

    int newWidth = clientRect.right - clientRect.left;
    int newHeight = clientRect.bottom - clientRect.top;

    return SetWindowPos(hWnd, NULL, windowRect.left, windowRect.top, newWidth, newHeight, SWP_NOZORDER | SWP_NOACTIVATE);
}

BOOL IsWindowCloaked(HWND hwnd) {
    BOOL isCloaked = FALSE;
    return SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED,
     &isCloaked, sizeof(isCloaked))) && isCloaked;
}

BOOL IsWindowVisibleOnScreen(HWND hwnd) {
    return IsWindowVisible(hwnd) && !IsWindowCloaked(hwnd);
}

void SetWindowPointer(HWND hwnd, void* pthis) {
    ::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pthis));
}

bool CheckWindowPointer(HWND hwnd, void* pthis) {
    return ::IsWindow(hwnd)
        && ::GetWindowLongPtr(hwnd, GWLP_USERDATA) == reinterpret_cast<LONG_PTR>(pthis);
}

void ClearWindowPointer(HWND hwnd) {
    ::SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
}

COLORREF GetDefaultHyperlinkColor(HWND hwnd) {
    COLORREF color = 0;

    // Try to open theme data for hyperlinks
    HTHEME hTheme = OpenThemeData(hwnd, L"Hyperlink");
    if (hTheme) {
        if (SUCCEEDED(GetThemeColor(hTheme, 0, 0, TMT_TEXTCOLOR, &color))) {
            CloseThemeData(hTheme);
            return color;
        }
        CloseThemeData(hTheme);
    }

    // Fallback to system-defined color
    return GetSysColor(COLOR_HOTLIGHT);
}

bool IsColorBright(COLORREF color) {
    int r = GetRValue(color);
    int g = GetGValue(color);
    int b = GetBValue(color);

    double brightness = 0.299 * r + 0.587 * g + 0.114 * b;

    return brightness > 128;
}


COLORREF AdjustColorBrightness(COLORREF color, int delta) {
    auto clamp = [](int val) -> BYTE {
        return (BYTE)(val < 0 ? 0 : (val > 255 ? 255 : val)); // Clamp values between 0 and 255
    };

    // Extract color components
    int r = GetRValue(color);
    int g = GetGValue(color);
    int b = GetBValue(color);

    // Calculate brightness (perceptual formula)
    double brightness = 0.299 * r + 0.587 * g + 0.114 * b;

    // Calculate adjustment factors based on each channel's contribution to brightness
    double rFactor = 0.299 / brightness;
    double gFactor = 0.587 / brightness;
    double bFactor = 0.114 / brightness;

    // If the color is light, decrease brightness; if dark, increase brightness
    int direction = brightness > 160 ? -1 : +1;

    // Adjust the components with respect to their contribution to brightness
    r += direction * delta * rFactor;
    g += direction * delta * gFactor;
    b += direction * delta * bFactor;

    return RGB(clamp(r), clamp(g), clamp(b)); // Return the color with clamped values
}

CComPtr<IAccPropServices> pAccPropServices;

// Run when the UI is created.
void SetControlAccessibleName(HWND hwnd, const WCHAR* name)
{
    HRESULT hr = S_OK;
    if (!pAccPropServices) {
        hr = pAccPropServices.CoCreateInstance(CLSID_AccPropServices, nullptr, CLSCTX_INPROC);
    }

    if (SUCCEEDED(hr)) {
        // Now set the name on the control. This gets exposed through UIA
        // as the element's Name property.
        pAccPropServices->SetHwndPropStr(hwnd, OBJID_CLIENT, CHILDID_SELF, Name_Property_GUID, name);
    }
}

// Run when the UI is destroyed.
void ClearControlAccessibleName(HWND hwnd)
{
    if (pAccPropServices) {
        // Clear the custom accessible name set earlier on the control.
        MSAAPROPID props[] = { Name_Property_GUID };

        pAccPropServices->ClearHwndProps(hwnd, OBJID_CLIENT, CHILDID_SELF, props, ARRAYSIZE(props));
    }
}

std::unique_ptr<Gdiplus::Bitmap> CreateDropDownArrowBitmap(HWND wnd, int iconWidth, int iconHeight, ArrowOrientation orientation /*= ARROW_DOWN*/) {
    const COLORREF sysTextColor = ::GetSysColor(COLOR_BTNTEXT);
    const Gdiplus::Color arrowColor(
        GetRValue(sysTextColor),
        GetGValue(sysTextColor),
        GetBValue(sysTextColor));

    auto bmp = std::make_unique<Gdiplus::Bitmap>(iconWidth, iconHeight, PixelFormat32bppARGB);
    Gdiplus::Graphics graphics(bmp.get());

    graphics.Clear(Gdiplus::Color(0, 0, 0, 0));
    graphics.SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    graphics.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHalf); // Align to pixel centers

    const auto baseSize = std::min(iconWidth, iconHeight) / 2.0f;
    float arrowWidth, arrowHeight;
    float left, top;

#define roundPt(x) (x)
//#define roundPt(x) std::roundf(x)
    if (orientation == ARROW_LEFT || orientation == ARROW_RIGHT) {
        arrowHeight = baseSize;
        arrowWidth = roundPt(arrowHeight * 0.7f);
        left = roundPt((iconWidth - arrowWidth) * 0.6f);
        top = (iconHeight - arrowHeight) / 2;
    } else {
        arrowWidth = baseSize;
        arrowHeight = roundPt(arrowWidth * 0.7f);
        left = (iconWidth - arrowWidth) / 2;
        top = roundPt((iconHeight - arrowHeight) * 0.4f);
    }
#undef roundPt
    Gdiplus::PointF points[3];
    switch (orientation) {
    case ARROW_UP:
        points[0] = { left + arrowWidth / 2, top };
        points[1] = { left, top + arrowHeight };
        points[2] = { left + arrowWidth, top + arrowHeight };
        break;
    case ARROW_DOWN:
        points[0] = { left, top + arrowHeight / 4 };
        points[1] = { left + arrowWidth, top + arrowHeight / 4 };
        points[2] = { left + arrowWidth / 2, top + arrowHeight };
        break;
    case ARROW_LEFT:
        points[0] = { left, top + arrowHeight / 2 };
        points[1] = { left + arrowWidth, top };
        points[2] = { left + arrowWidth, top + arrowHeight };
        break;
    case ARROW_RIGHT:
        points[0] = { left + arrowWidth, top + arrowHeight / 2 };
        points[1] = { left, top };
        points[2] = { left, top + arrowHeight };
        break;
    }

    Gdiplus::SolidBrush brush(arrowColor);
    graphics.FillPolygon(&brush, points, 3);
    return bmp;
}

HICON CreateDropDownArrowIcon(HWND wnd, int dpi, ArrowOrientation orientation) {
    const int iconWidth = DPIHelper::GetSystemMetricsForDpi(SM_CXSMICON, dpi);
    const int iconHeight = DPIHelper::GetSystemMetricsForDpi(SM_CYSMICON, dpi);
    auto bmp = CreateDropDownArrowBitmap(wnd, iconWidth, iconHeight, orientation);
    HICON hIcon = nullptr;
    bmp->GetHICON(&hIcon);

    return hIcon;
}

HICON GetMenuArrowIcon() {
    const int iconWidth = GetSystemMetrics(SM_CXSMICON);
    const int iconHeight = GetSystemMetrics(SM_CYSMICON);

    CWindowDC dcScreen(nullptr);
    CDC dcMem;
    dcMem.CreateCompatibleDC(dcScreen);

    BITMAPINFO bmi = { 0 };
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = iconWidth;
    bmi.bmiHeader.biHeight = -iconHeight; // Top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;

    void* pBits = nullptr;
    CBitmap bmpColor;
    bmpColor.CreateDIBSection(dcMem, &bmi, DIB_RGB_COLORS, &pBits, nullptr, 0);

    CBitmapHandle hOldBmp = dcMem.SelectBitmap(bmpColor);

    dcMem.FillSolidRect(0, 0, iconWidth, iconHeight, RGB(0, 0, 0));

    CRect rc(0, 0, iconWidth, iconHeight);
    dcMem.DrawFrameControl(rc, DFC_MENU, DFCS_MENUARROW);

    CBitmap bmpMask;
    bmpMask.CreateBitmap(iconWidth, iconHeight, 1, 1, nullptr);

    ICONINFO ii = { 0 };
    ii.fIcon = TRUE;
    ii.hbmColor = bmpColor;
    ii.hbmMask = bmpMask;
    HICON hIcon = ::CreateIconIndirect(&ii);

    dcMem.SelectBitmap(hOldBmp);
    return hIcon;
}

HICON GetWindowIcon(HWND hwnd) {
    HICON hIcon {};
    //hIcon  = (HICON)SendMessage(hwnd, WM_GETICON, ICON_BIG, 0);

    if (!hIcon) {
        hIcon = (HICON)SendMessage(hwnd, WM_GETICON, ICON_SMALL, 0);
    }

    if (!hIcon) {
        hIcon = (HICON)GetClassLongPtr(hwnd, GCLP_HICON);
    }

    return hIcon;
}

bool DisableDwmAnimations(HWND hwnd, BOOL disable) {
    return SUCCEEDED(DwmSetWindowAttribute(hwnd,
        DWMWA_TRANSITIONS_FORCEDISABLED,
        &disable,
        sizeof(disable)));
}

bool IsToastImageFormatSupported(const CString& filePath) {
    CString ext = WinUtils::GetFileExt(filePath);
    ext.MakeLower();
    return ext == _T("png") || ext == _T("jpg") || ext == _T("jpeg") || ext == _T("bmp") || ext == _T("gif");
}

void DeleteAllToolbarButtons(CToolBarCtrl& toolbar) {
    int buttonCount = toolbar.GetButtonCount();

    for (int i = buttonCount - 1; i >= 0; i--) {
        toolbar.DeleteButton(i);
    }
}

HBITMAP GetAssociatedIconAsBitmap(LPCWSTR filePath, HWND hwnd, int iconSize, SIIGBF flags) {
    CComPtr<IShellItem> pShellItem;

    HRESULT hr = SHCreateItemFromParsingName(filePath, nullptr,
        IID_IShellItem, reinterpret_cast<void**>(&pShellItem));
    if (FAILED(hr)) {
        return { };
    }

    CComPtr<IShellItemImageFactory> pImageFactory;
    hr = pShellItem.QueryInterface(&pImageFactory);
    if (FAILED(hr)) {
        return { };
    }

    HBITMAP hBitmap {};
    hr = pImageFactory->GetImage({ iconSize, iconSize }, flags, &hBitmap);
    if (FAILED(hr)) {
        return {  };
    }

    return hBitmap;
}


}
