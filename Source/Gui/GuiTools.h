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
#ifndef IU_GUITOOLS_H
#define IU_GUITOOLS_H
#pragma once

#include "atlheaders.h"
#include "3rdpart/GdiplusH.h"
#include <memory>

namespace GuiTools
{
    int AddComboBoxItem(HWND hDlg, int itemId, LPCTSTR item);
    bool AddComboBoxItems(HWND hDlg, int itemId, int itemCount, ...);
    void GetCheck(HWND dlg, int id, bool& check);
    bool IsChecked(HWND dlg, int id);

    inline bool GetCheck(HWND dlg, int id) {
        return IsChecked(dlg, id);
    }
    void SetCheck(HWND dlg, int id, bool check);
    [[nodiscard]] HFONT MakeLabelBold(HWND Label);
    void EnableNextN(HWND Control, int n, bool Enable);
    bool InsertMenu(HMENU hMenu, int pos, UINT id, LPCTSTR szTitle, HBITMAP bm = 0);
    void FillRectGradient(HDC hdc, const RECT& FillRect, COLORREF start, COLORREF finish, bool Horizontal);
    RECT GetDialogItemRect(HWND dialog, int itemId);
    void ShowDialogItem(HWND dlg, int itemId, bool show);
    void EnableDialogItem(HWND dlg, int itemId, bool enable);
    void RemoveWindowStyleEx(HWND hWnd, DWORD styleEx);

    // Converts pixels to Win32 dialog units
    int dlgX(int WidthInPixels);
    int dlgY(int HeightInPixels);

    CString GetWindowText(HWND wnd);
    CString GetDlgItemText(HWND dialog, int controlId);

    int GetWindowLeft(HWND Wnd);

    HFONT MakeFontUnderLine(HFONT font);
    HFONT MakeFontBold(HFONT font);
    HFONT MakeFontSmaller(HFONT font);
    [[nodiscard]] HFONT MakeLabelItalic(HWND Label);

    int GetFontSize(int nFontHeight);
    int GetFontHeight(int nFontSize);
    HFONT GetSystemDialogFont(UINT dpi = 0);

    int ScreenBPP();
    BOOL Is32BPP();

    RECT AutoSizeStaticControl(HWND control);

    struct IconInfo
    {
        int     nWidth;
        int     nHeight;
        int     nBitsPerPixel;
    };

    IconInfo GetIconInfo(HICON hIcon);

    void GetScreenBounds(RECT& rect);
    HRGN CloneRegion(HRGN source);
    HFONT MakeFontBigger(HFONT font);
    HWND CreateToolTipForWindow(HWND hwnd, const CString& text);
    void AddToolTip(HWND hwndTT, HWND hwnd, const CString& text);
    CHARFORMAT LogFontToCharFormat(const LOGFONT & lf);
    LOGFONT CharFormatToLogFont(const CHARFORMAT & cf);
    HICON LoadSmallIcon(int resourceId, UINT dpi = 0);
    HICON LoadBigIcon(int resourceId, UINT dpi = 0);
    int LocalizedMessageBox(HWND hWnd, LPCWSTR lpText, LPCWSTR lpCaption = _T(""), UINT uType = MB_OK);
    BOOL SetClientRect(HWND hWnd, int x, int y);
    BOOL IsWindowCloaked(HWND hwnd);
    BOOL IsWindowVisibleOnScreen(HWND hwnd);

    void SetWindowPointer(HWND hwnd, void* pthis);
    // We need this function to check in lambda if CWindow* 'this' is still alive.
    // The function checks if the window exists and belongs to the current class.
    bool CheckWindowPointer(HWND hwnd, void* pthis);
    void ClearWindowPointer(HWND hwnd);

    COLORREF GetDefaultHyperlinkColor(HWND hwnd);

    bool IsColorBright(COLORREF color);
    COLORREF AdjustColorBrightness(COLORREF color, int delta);
    enum ArrowOrientation {
        ARROW_DOWN, // ▼
        ARROW_UP, // ▲
        ARROW_LEFT, // ◀
        ARROW_RIGHT // ▶
    };
    std::unique_ptr<Gdiplus::Bitmap> CreateDropDownArrowBitmap(HWND wnd, int width, int height, ArrowOrientation orientation = ARROW_DOWN);
    HICON CreateDropDownArrowIcon(HWND wnd, UINT dpi, ArrowOrientation orientation = ARROW_DOWN);

    HICON GetMenuArrowIcon();

    void SetControlAccessibleName(HWND hwnd, const WCHAR* name);
    void ClearControlAccessibleName(HWND hwnd);

    HICON GetWindowIcon(HWND hwnd);
    bool DisableDwmAnimations(HWND hwnd, BOOL disable = TRUE);

    bool IsToastImageFormatSupported(const CString& filePath);

    void DeleteAllToolbarButtons(CToolBarCtrl& toolbar);

    HBITMAP GetAssociatedIconAsBitmap(LPCWSTR filePath, HWND hwnd, int iconSize, SIIGBF flags = SIIGBF_ICONONLY);
    }
#endif
