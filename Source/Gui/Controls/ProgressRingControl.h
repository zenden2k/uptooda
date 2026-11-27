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

#ifndef IU_GUI_CONTROLS_PROGRESSRINGCONTROL_H
#define IU_GUI_CONTROLS_PROGRESSRINGCONTROL_H

#pragma once
#include "atlheaders.h"
#include "3rdpart/GdiPlusH.h"
// CProgressRingControl

class CProgressRingControl :
    public CWindowImpl<CProgressRingControl>
{
public:
    CProgressRingControl();
    ~CProgressRingControl();

    const unsigned int kTimerId = 1;

    DECLARE_WND_CLASS(_T("CProgressRingControl"))

    BEGIN_MSG_MAP(CProgressRingControl)
        MESSAGE_HANDLER(WM_PAINT, OnPaint)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
        MESSAGE_HANDLER(WM_ERASEBKGND, OnEraseBkg)
        MESSAGE_HANDLER(WM_TIMER, OnTimer)
        MESSAGE_HANDLER(WM_SHOWWINDOW, OnShowWindow)
    END_MSG_MAP()

    LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnEraseBkg(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnShowWindow(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    BOOL SubclassWindow(HWND hWnd);
    // Handler prototypes:
    //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);

protected:
    HBITMAP backBufferBm_;
    HBITMAP oldBm_;
    HDC backBufferDc_;
    int backBufferWidth_, backBufferHeight_;
    int timerCounter_;
    void initControl();
};

#endif // IU_GUI_CONTROLS_PROGRESSRINGCONTROL_H
