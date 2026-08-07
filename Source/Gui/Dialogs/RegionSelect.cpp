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

#include "RegionSelect.h"

#include "ScreenCapture/ScreenshotHelper.h"
#include "Gui/GuiTools.h"
#include "Core/Settings/WtlGuiSettings.h"

namespace {

struct WindowsListItem
{
    HWND handle;
    RECT rect;
};

std::vector<WindowsListItem> windowsList;

BOOL CALLBACK EnumChildProc(HWND hwnd, LPARAM lParam)
{
    if(IsWindowVisible(hwnd)){
        EnumChildWindows(hwnd, EnumChildProc, 0);
        WindowsListItem newItem{};
        newItem.handle = hwnd;
        GetWindowRect(hwnd, &newItem.rect);
        windowsList.push_back(newItem);
    }
    return true;
}


BOOL CALLBACK RegionEnumWindowsProc(HWND hwnd,LPARAM lParam)
{
    bool onlyTopWindows = lParam;
    if(GuiTools::IsWindowVisibleOnScreen(hwnd)) {
        if (!onlyTopWindows) {
            EnumChildWindows(hwnd, EnumChildProc, 0);
        }
        WindowsListItem newItem{};
        newItem.handle = hwnd;
        GetWindowRect(hwnd, &newItem.rect);
        windowsList.push_back(newItem);
    }
    return true;
}

HWND WindowUnderCursor(POINT pt, HWND exclude, bool onlyTopWindows = false)
{
    if(windowsList.empty()) {
        EnumWindows(RegionEnumWindowsProc, onlyTopWindows);
    }
    for (const auto& curItem : windowsList) {
        if (::PtInRect(&curItem.rect, pt) && curItem.handle != exclude && GuiTools::IsWindowVisibleOnScreen(curItem.handle)) {
            //LOG(ERROR) << curItem.handle << " " << GuiTools::GetWindowText(curItem.handle) << " " << IsIconic(curItem.handle);
            return curItem.handle;
        }
    }
    return nullptr;
}

}

CRegionSelect RegionSelect;

using namespace ScreenCapture;

// CRegionSelect
CRegionSelect::CRegionSelect()
{
    m_bDocumentChanged = false;
    m_bFinish = false;
    m_bSaveAsRegion = false;
    m_ResultRegion = nullptr;
    RectCount = 0;
    Down = false;
    m_brushSize = 0;
    End.x = -1;
    End.y = -1;
    Start.x = -1;
    Start.y = -1;
    DrawingPen = nullptr;
    DrawingBrush = nullptr;
    lineType = 0;
    cxOld = -1;
    cyOld = -1;
    pen = CreatePen(PS_SOLID, 2, 0); //Solid black line, width = 2 pixels
    CrossCursor = LoadCursor(nullptr, IDC_CROSS);
    HandCursor = LoadCursor(nullptr, IDC_HAND);
    m_SelectionMode = SelectionMode::smRectangles;
    hSelWnd = nullptr;
    m_PrevWindowRect = { 0, 0, 0, 0 };
    m_btoolWindowTimerRunning = false;
    m_bPainted = false;
    m_DoubleBuffer = nullptr;
    gdipBm = nullptr;
    m_Width = 0;
    m_Height = 0;
    m_bResult = false;
    m_brushColor = RGB(0, 0, 0);
    m_bPictureChanged = false;
}

void CRegionSelect::setSelectionMode(SelectionMode selMode, bool onlyTopWindows) {
    m_SelectionMode = selMode;
    onlyTopWindows_ = onlyTopWindows;
}

SelectionMode CRegionSelect::selectionMode() const {
    return m_SelectionMode;
}

CRegionSelect::~CRegionSelect()
{
    if (DrawingPen) {
        DeleteObject(DrawingPen);
    }
    if (DrawingBrush) {
        DeleteObject(DrawingBrush);
    }
    if (pen) {
        DeleteObject(pen);
    }
    if (CrossCursor) {
        DeleteObject(CrossCursor);
    }
    Detach();
}

LRESULT CRegionSelect::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return 0;  // Let the system set the focus
}

LRESULT CRegionSelect::OnPaint(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& bHandled) {
    CPaintDC dc(m_hWnd);

    int w = dc.m_ps.rcPaint.right - dc.m_ps.rcPaint.left;
    int h = dc.m_ps.rcPaint.bottom - dc.m_ps.rcPaint.top;

    if (m_bPictureChanged) {
        if (Down) {
            return 0;
        }
        RECT rc;
        GetClientRect(&rc);
        CRgn newRegion;
        newRegion.CreateRectRgn(0, 0, rc.right, rc.bottom);
        SelectClipRgn(doubleDC, newRegion);
        BitBlt(doubleDC, dc.m_ps.rcPaint.left, dc.m_ps.rcPaint.top, w, h, memDC2, dc.m_ps.rcPaint.left, dc.m_ps.rcPaint.top,SRCCOPY);
        newRegion.CombineRgn(m_SelectionRegion,RGN_DIFF);
        CBrush br;
        SelectClipRgn(doubleDC, newRegion);
        br.CreateSolidBrush(RGB(200, 200, 0));
        BLENDFUNCTION bf;

        bf.BlendOp = AC_SRC_OVER;
        bf.BlendFlags = 1;
        bf.SourceConstantAlpha = 40;
        bf.AlphaFormat = 0; ///*0 */ /*AC_SRC_ALPHA*/0;

        if (RectCount)
            if (AlphaBlend(doubleDC, dc.m_ps.rcPaint.left, dc.m_ps.rcPaint.top, w, h, alphaDC, dc.m_ps.rcPaint.left, dc.m_ps.rcPaint.top, w,
                           h, bf) == FALSE) {
            }
        newRegion.DeleteObject();
        newRegion.CreateRectRgn(0, 0, rc.right, rc.bottom);
        SelectClipRgn(doubleDC, newRegion);
        RECT SelWndRect;
        if (hSelWnd) {
            CRgn WindowRgn = ScreenshotHelper::getWindowVisibleRegion(hSelWnd);
            WindowRgn.OffsetRgn(topLeft);
            WindowRgn.GetRgnBox(&SelWndRect);
            CRect DrawingRect = SelWndRect;
            DrawingRect.DeflateRect(2, 2);
            HGDIOBJ oldPen = SelectObject(doubleDC, pen);
            SetROP2(doubleDC, R2_NOTXORPEN);
            SelectClipRgn(doubleDC, 0);
            Rectangle(doubleDC, DrawingRect.left, DrawingRect.top, DrawingRect.right, DrawingRect.bottom);
            SelectObject(doubleDC, oldPen);
        }
        m_bPictureChanged = false;
    }
    dc.BitBlt(dc.m_ps.rcPaint.left, dc.m_ps.rcPaint.top, w, h, doubleDC, dc.m_ps.rcPaint.left, dc.m_ps.rcPaint.top, SRCCOPY);


    m_bPainted = true;
    return 0;
}

LRESULT CRegionSelect::OnLButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    Down = true;
    Start.x = GET_X_LPARAM(lParam);
    Start.y = GET_Y_LPARAM(lParam);

    POINT newPoint = { GET_X_LPARAM(lParam), GET_Y_LPARAM(lParam) };
    m_curvePoints.push_back(newPoint);
    return 0;
}


LRESULT CRegionSelect::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    int cx = GET_X_LPARAM(lParam);
    int cy = GET_Y_LPARAM(lParam);
    DWORD fwKeys = wParam;
    POINT point = {cx, cy};
    HWND hNewSelWnd;
    bool bUpdateWindow = false;

    if (m_SelectionMode == SelectionMode::smWindowHandles) {
        POINT newP = point;
        ClientToScreen(&newP);

        if ((hNewSelWnd = WindowUnderCursor(newP, m_hWnd, onlyTopWindows_)) == nullptr) {
            hNewSelWnd = ::GetDesktopWindow();
        }
        else {
            /*HWND hChildWnd = MyChildWindowFromPoint(hNewSelWnd, point);
            if ( hChildWnd )
              hNewSelWnd = hChildWnd;*/
        }
        if (hNewSelWnd != hSelWnd) {
            CRgn FullScreenRgn;


            FullScreenRgn.CreateRectRgnIndirect(&m_screenBounds);
            RECT SelWndRect;
            ::GetWindowRect(hNewSelWnd, &SelWndRect);
            CRgn WindowRgn;

            if (::GetWindowRgn(hNewSelWnd, WindowRgn) != ERROR) {
                //WindowRegion.GetRgnBox( &WindowRect);
                WindowRgn.OffsetRgn(SelWndRect.left, SelWndRect.top);
            }

            CBrush br;
            br.CreateSolidBrush(RGB(200, 0, 0));

            m_bPictureChanged = true;
            m_PrevWindowRect = SelWndRect;

            CRgn repaintRgn;
            repaintRgn.CreateRectRgnIndirect(&SelWndRect);
            repaintRgn.OffsetRgn(topLeft);

            if (!m_prevWindowRgn.IsNull())
                repaintRgn.CombineRgn(m_prevWindowRgn, RGN_OR);


            m_prevWindowRgn = repaintRgn;
            InvalidateRgn(repaintRgn);
            repaintRgn.Detach();
            bUpdateWindow = true;
        }
        hSelWnd = hNewSelWnd;

        if (!(wParam & MK_RBUTTON)) {
            bUpdateWindow = true;
        }
    }
    //else
    {
        if(fwKeys & MK_LBUTTON && Down    )
        {
            HDC dc = GetDC();

            SelectObject(dc, pen);

            if(m_SelectionMode!= SelectionMode::smFreeform)
            {
                SetROP2(dc, R2_NOTXORPEN);
                if(End.x>-1)
                    Rectangle(dc, Start.x,Start.y, End.x, End.y);

                End.x = GET_X_LPARAM(lParam);
                End.y = GET_Y_LPARAM(lParam);

                bool Draw = true;
                if(m_SelectionMode == SelectionMode::smWindowHandles)
                {

                    if(abs(End.x-Start.x)<7 && abs(End.y-Start.y)<7)
                    {
                        Draw=false;
                    }
                    else
                    {
                        m_SelectionMode = SelectionMode::smRectangles;
                        hSelWnd = 0;
                        m_bPictureChanged = true;
                        /*Invalidate();*/
                        bUpdateWindow = true;
                    }
                }
                    if(Draw) Rectangle(dc, Start.x,Start.y, End.x, End.y);


            }
            else
            {
                SetROP2(doubleDC, R2_COPYPEN);
                POINT p = m_curvePoints.back();
                MoveToEx(doubleDC, p.x, p.y,0);
                HGDIOBJ oldPen = SelectObject(doubleDC, pen);
                POINT newPoint  = {LOWORD(lParam), HIWORD(lParam)};
                LineTo(doubleDC, newPoint.x, newPoint.y);
                m_curvePoints.push_back(newPoint);
                SelectObject(doubleDC, oldPen);

                RECT RectToRepaint;
                RectToRepaint.left = std::min(p.x, newPoint.x) - m_brushSize;
                RectToRepaint.top = std::min(p.y, newPoint.y) - m_brushSize;
                RectToRepaint.right = std::max(p.x, newPoint.x) + m_brushSize;
                RectToRepaint.bottom = std::max(p.y, newPoint.y) + m_brushSize;
                InvalidateRect(&RectToRepaint, false);

            }
            ReleaseDC(dc);
        }
}
        if(wParam & MK_RBUTTON)
        {
            HGDIOBJ oldPen2 = SelectObject(memDC2, DrawingPen);

            if(cxOld != -1)
            {
                SHORT shiftState = GetAsyncKeyState(VK_SHIFT);
                if(shiftState& 0x8000) {
                    if (!lineType ) {
                        lineType = abs(cx-cxOld) >= abs(cy-cyOld) ? 1 : 2;
                    }
                    if ( lineType == 1 ) {
                        cy = cyOld;
                    } else {
                        cx = cxOld;
                    }
                }


                SelectClipRgn(memDC2, 0);
                MoveToEx(memDC2, cxOld, cyOld,0);
                LineTo(memDC2, cx,cy);

                RECT RectToRepaint;
                RectToRepaint.left = std::min(cxOld, cx) - m_brushSize;
                RectToRepaint.top = std::min(cyOld, cy) - m_brushSize;
                RectToRepaint.right = std::max(cxOld, cx) + m_brushSize;
                RectToRepaint.bottom = std::max(cyOld, cy) + m_brushSize;
                CRgn rgn;
                rgn.CreateRectRgnIndirect(&RectToRepaint);
                m_bPictureChanged = true;
                m_bDocumentChanged = true;
                InvalidateRect(&RectToRepaint);
                UpdateWindow();
            }
            SelectObject(memDC2, oldPen2);
            cxOld = cx;
            cyOld = cy;
        }

    if(bUpdateWindow) UpdateWindow();
    return 0;
}

LRESULT CRegionSelect::OnEraseBg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    bHandled = true; //avoids flickering
    return TRUE;
}

LRESULT CRegionSelect::OnLButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if(!Down) return 0;
    Down = false;
    End.x = GET_X_LPARAM(lParam) + 1;
    End.y = GET_Y_LPARAM(lParam) + 1;
    CRgn newRegion;    RECT winRect;
    SHORT shiftState = GetAsyncKeyState(VK_SHIFT);

    WPARAM fwKeys = wParam;
    m_bPictureChanged = true;

    if(m_SelectionMode == SelectionMode::smFreeform)
    {
        Finish();
        return 0;
    }

    if (m_SelectionMode != SelectionMode::smWindowHandles || (abs(End.x - Start.x) > 7 && abs(End.y - Start.y) > 7)) {
        newRegion.CreateRectRgn(Start.x, Start.y, End.x, End.y);
    } else {
        ::GetWindowRect(hSelWnd, &winRect);

        newRegion.CreateRectRgnIndirect(&winRect);
        newRegion.OffsetRgn(topLeft);
        RECT invRect;
        newRegion.GetRgnBox(&invRect);

        Start.x = invRect.left;
        Start.y = invRect.top;
        End.x = invRect.right;
        End.y = invRect.bottom;
        if (fwKeys & MK_CONTROL) {
            m_SelectedWindowsRegion.AddWindow(hSelWnd, false);
        } else {
            m_SelectedWindowsRegion.AddWindow(hSelWnd, true);
        }
    }

    if(fwKeys & MK_CONTROL) {
        m_SelectionRegion.CombineRgn(newRegion, RGN_DIFF);

    } else if(shiftState& 0x8000) { // shift is down
        m_SelectionRegion.CombineRgn(newRegion, RGN_OR);
    } else {
        m_SelectionRegion.CombineRgn(newRegion, RGN_OR);
        Finish();
        return 0;
    }

    if(!RectCount) {
        RectCount++;
        Invalidate();
    } else {
        RectCount++;

        RECT RectToRepaint;
        //if(m_SelectionMode != smWindowHandles)
        {
            RectToRepaint.left = std::min(Start.x, End.x) - m_brushSize;
            RectToRepaint.top = std::min(Start.y, End.y) - m_brushSize;
            RectToRepaint.right = std::max(Start.x, End.x) + m_brushSize;
            RectToRepaint.bottom = std::max(Start.y, End.y) + m_brushSize;

            InvalidateRect(&RectToRepaint);
        }

    }

    Start.x = -1;
    Start.y = -1;
    End.x = -1;
    End.y = -1;
    return 0;
}

LRESULT CRegionSelect::OnRButtonDown(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    //Down = false;
    return 1;
}

bool CRegionSelect::Execute(HBITMAP screenshot, int width, int height, HMONITOR monitor)
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    m_bPictureChanged = false;
    m_bDocumentChanged = false;
    m_btoolWindowTimerRunning = false;
    if (!screenshot) {
        return false;
    }
    m_bFinish = false;
    m_bResult = false;
    Cleanup();

    m_SelectionRegion.CreateRectRgn(0,0,0,0);
    m_curvePoints.clear();

    m_Width = width;
    m_Height = height;

    RECT r = { 0, 0, m_Width, m_Height };
    if (!m_hWnd) {
        Create(nullptr, r, _T("ImageUploader_RegionWnd"), WS_POPUP, WS_EX_TOPMOST);
    }

    HDC dstDC = ::GetDC(m_hWnd);
     //doubleBm.
  ///* m_hBitmap = */CreateDIBSection(dstDC, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, 0, NULL, NULL);

    doubleBm.CreateCompatibleBitmap(dstDC, m_Width, m_Height);
    doubleDC.CreateCompatibleDC(dstDC);
    HBITMAP oldDoubleBm = doubleDC.SelectBitmap(doubleBm);

    m_bmScreenShot = screenshot;
    memDC2.CreateCompatibleDC(dstDC);
    HBITMAP oldBm = memDC2.SelectBitmap(m_bmScreenShot);

    setDrawingParams(settings->ScreenshotSettings.BrushColor, 3);
    Down = false;
    Start.x = -1;
    Start.y = -1;
    End.x = -1;
    End.y = -1;

    cxOld = -1;
    cyOld = -1;
   // HDC hDC;
    //hDC = GetDC(NULL);
     //alphaBm.
   ///* m_hBitmap =*/ CreateDIBSection(dstDC, (BITMAPINFO*)&bmi, DIB_RGB_COLORS, 0, NULL, NULL);

    alphaBm.CreateCompatibleBitmap(dstDC, m_Width, m_Height);

    CBrush br2;
    br2.CreateSolidBrush(RGB(0,0,150));
    alphaDC.CreateCompatibleDC(dstDC);
    HBITMAP oldAlphaBm = alphaDC.SelectBitmap(alphaBm);

    alphaDC.FillRect(&r, br2);
    m_bPictureChanged = true;

    ::ReleaseDC(m_hWnd, dstDC);

    CRect windowRc;
    GuiTools::GetScreenBounds(m_screenBounds);
    if (monitor) {
        MONITORINFO mi;
        memset(&mi, 0, sizeof(mi));
        mi.cbSize = sizeof(mi);
        if (!GetMonitorInfo(monitor, &mi)) {
            LOG(ERROR) << "Unable get info about monitor";
            return false;
        }

        windowRc = mi.rcMonitor;
    } else {
        windowRc = m_screenBounds;
    }

    //RECT screenBounds;
    MoveWindow(windowRc.left, windowRc.top, windowRc.Width(), windowRc.Height());
    topLeft.x = 0;
    topLeft.y = 0;
    ScreenToClient(&topLeft);

    InvalidateRect(nullptr, FALSE);
    ShowWindow(SW_SHOW);

    MSG msg;
    while(!m_bFinish && GetMessage(&msg, nullptr, NULL, NULL) )
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    memDC2.SelectBitmap(oldBm);
    memDC2.DeleteDC();

    doubleDC.SelectBitmap(oldDoubleBm);
    doubleDC.DeleteDC();

    alphaDC.SelectBitmap(oldAlphaBm);
    alphaDC.DeleteDC();

    alphaBm.DeleteObject();


    ShowWindow(SW_HIDE);
    return m_bResult;
}

void CRegionSelect::Finish()
{
    m_bResult = true;
    if(m_SelectionMode == SelectionMode::smRectangles
        || (m_SelectionMode == SelectionMode::smWindowHandles && wasImageEdited())) {
        m_ResultRegion = std::make_shared<CRectRegion>(m_SelectionRegion);
    } else if(m_SelectionMode == SelectionMode::smFreeform) {
        auto* newRegion = new CFreeFormRegion();
        for(const auto& item: m_curvePoints) {
            newRegion->AddPoint(item);
        }
        m_ResultRegion.reset(newRegion);
    } else if(m_SelectionMode == SelectionMode::smWindowHandles) {
        m_ResultRegion = std::make_shared<CWindowHandlesRegion>(m_SelectedWindowsRegion);
    }

    if(m_ResultRegion->IsEmpty()) {
        m_bResult = false;
    }

    m_bFinish = true;
}

void CRegionSelect::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
    if(nChar == VK_ESCAPE)  // Cancel capturing
    {
        m_bFinish = true;
        m_bResult = false;
    }

    else if (nChar == VK_RETURN)
    {
        Start.x = 0;   // If "Enter" key was pressed, we make a shot of entire screen
        Start.y = 0;
        End.x = 0;
        End.y = 0;
        Finish();
    }
}

BOOL CRegionSelect::OnSetCursor(CWindow wnd, UINT nHitTest, UINT message)
{
    if(m_SelectionMode != SelectionMode::smWindowHandles) {
        SetCursor(CrossCursor);
    } else {
        SetCursor(HandCursor);
    }
    return TRUE;
}

LRESULT  CRegionSelect::OnRButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    lineType = 0;
    cxOld = -1;
    cyOld = -1;
    return 0;
}
bool CRegionSelect::setDrawingParams(COLORREF color, int brushSize)
{
    if (brushSize < 1) {
        brushSize = 1;
    }

    if (brushSize == m_brushSize && color == m_brushColor) {
        return true;
    }

    if (DrawingPen) {
        DeleteObject(DrawingPen);
    }
    DrawingPen = CreatePen(PS_SOLID, brushSize, color);

    if (DrawingBrush) {
        DeleteObject(DrawingBrush);
    }
    DrawingBrush = CreateSolidBrush(color);

    m_brushSize = brushSize;
    m_brushColor = color;
    return true;
}

LRESULT CRegionSelect::OnMButtonUp(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    HWND hwnd = reinterpret_cast<HWND>(wParam);
    POINT ClientPoint, ScreenPoint;

    ScreenPoint.x = GET_X_LPARAM(lParam);
    ScreenPoint.y = GET_Y_LPARAM(lParam);
    ClientPoint = ScreenPoint;
    ::ScreenToClient(hwnd, &ClientPoint);

    CColorDialog ColorDialog(m_brushColor);
    if (ColorDialog.DoModal(m_hWnd) == IDOK) {
        COLORREF newColor =  ColorDialog.GetColor();
        setDrawingParams(newColor, m_brushSize);
        return TRUE;
    }
    return 0;
}

LRESULT CRegionSelect::OnChar(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    auto chCharCode = static_cast<TCHAR>(wParam);
    if(chCharCode == _T('[')) {
        setDrawingParams(m_brushColor,m_brushSize-1);
    } else if(chCharCode == _T(']')) {
        setDrawingParams(m_brushColor, m_brushSize+1 );
    }
    return 0;
}

void CRegionSelect::Cleanup()
{
    m_ResultRegion.reset();
    RectCount = 0 ;
    windowsList.clear();
    hSelWnd = nullptr;
    m_bPictureChanged = true;
    m_SelectedWindowsRegion.Clear();
    m_bPainted = false;

    if (!m_SelectionRegion.IsNull()) {
        m_SelectionRegion.DeleteObject();
    }

    if (!doubleDC.IsNull()) {
        doubleDC.DeleteDC();
    }

    if (!doubleBm.IsNull()) {
        doubleBm.DeleteObject();
    }
}

std::shared_ptr<CScreenshotRegion> CRegionSelect::region() const
{
    return m_ResultRegion;
}


LRESULT CRegionSelect::OnNcCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    SetWindowLong(GWL_EXSTYLE, GetWindowLong(GWL_EXSTYLE) & ~WS_EX_LAYOUTRTL);
    return TRUE;
}

LRESULT CRegionSelect::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    return 0;
}

LRESULT CRegionSelect::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    if (wParam == 1) {// timer ID =1
        Toolbar.ShowWindow(SW_SHOW);
        KillTimer(1);
        m_btoolWindowTimerRunning = false;
    }
    return 0;
}

bool CRegionSelect::wasImageEdited() const
{
    return m_bDocumentChanged;
}
