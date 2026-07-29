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

#include "ProgressRingControl.h"

#include "Core/Images/Utils.h"


CProgressRingControl::CProgressRingControl() :
    backBufferBm_(nullptr),
    oldBm_(nullptr),
    backBufferDc_(nullptr),
    backBufferWidth_(0),
    backBufferHeight_(0),
    timerCounter_(0)

{
}

CProgressRingControl::~CProgressRingControl()
{
    if (backBufferDc_) {
        ::SelectObject(backBufferDc_, oldBm_);
        DeleteDC(backBufferDc_);
    }
    if (backBufferBm_) {
        DeleteObject(backBufferBm_);
    }
}

LRESULT CProgressRingControl::OnPaint(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
    using namespace Gdiplus;
    CPaintDC dc(m_hWnd);
    bHandled = true;

    CRect clientRect;
    GetClientRect(&clientRect);
    HBRUSH sysBr = GetSysColorBrush(COLOR_BTNFACE);
    FillRect(backBufferDc_, &clientRect, sysBr);

    Graphics graphics(backBufferDc_);
    graphics.SetSmoothingMode(SmoothingModeHighQuality);

    float penSize = 4.0f;
    //SolidBrush br(Color(GetRValue(colorBtnFace), GetGValue(colorBtnFace), GetBValue(colorBtnFace)));
    int arcWidth = std::min<>(clientRect.Width(), clientRect.Height())- penSize*2;
    //graphics.FillRectangle(&br, 0, 0, clientRect.Width(), clientRect.Height());
    LinearGradientBrush lgBrush(Rect(-1, -1, arcWidth+10, arcWidth + 10),
        Color().Black, Color(0, 200, 200, 200), static_cast<REAL>(timerCounter_));
    Pen p(&lgBrush, penSize);
    graphics.DrawArc(&p, 2, 2, 2 + arcWidth, 2 + arcWidth, static_cast<REAL>(timerCounter_), 180);

    dc.BitBlt(0, 0, backBufferWidth_, backBufferHeight_, backBufferDc_, 0, 0, SRCCOPY);
    bHandled = true;
    return 0;
}

LRESULT CProgressRingControl::OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    KillTimer(kTimerId);
    return 0;
}

LRESULT CProgressRingControl::OnEraseBkg(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled)
{
    bHandled = true;
    return TRUE;
}

LRESULT CProgressRingControl::OnTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL&) {
    if (wParam == kTimerId) {
        InvalidateRect(nullptr);
        timerCounter_ += 12;
        if (timerCounter_ >= 360) {
            timerCounter_ = 0;
        }
    }
    return 0;
}

BOOL CProgressRingControl::SubclassWindow(HWND hWnd) {
    if (!CWindowImpl::SubclassWindow(hWnd)) {
        return FALSE;
    }
    initControl();
    return TRUE;
}

void CProgressRingControl::initControl() {
    CRect clientRect;
    GetClientRect(&clientRect);
    CClientDC dc(m_hWnd);
    backBufferWidth_ = clientRect.Width();
    backBufferHeight_ = clientRect.Height();

    backBufferDc_ = ::CreateCompatibleDC(dc);
    backBufferBm_ = ::CreateCompatibleBitmap(dc, backBufferWidth_, backBufferHeight_);
    oldBm_ = static_cast<HBITMAP>(::SelectObject(backBufferDc_, backBufferBm_));

    SetTimer(kTimerId, 50);
}

LRESULT CProgressRingControl::OnShowWindow(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
    if (wParam == TRUE && lParam == 0) {
        timerCounter_ = 0;
    }
    return 0;
}
