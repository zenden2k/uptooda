#include "DragndropOverlay.h"

#include "Core/i18n/Translator.h"
#include "Func/WinUtils.h"

LRESULT CDragndropOverlay::OnPaint(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    CRect clientRect;
    GetClientRect(&clientRect);
    CPaintDC paintDc(m_hWnd);

    paintDc.BitBlt(0, 0, clientRect.Width(), clientRect.Height(), backBufferDc_, 0, 0, SRCCOPY);

    return 0;
}

LRESULT CDragndropOverlay::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    CClientDC dc(m_hWnd);
    LOGFONT lf;
    WinUtils::StringToFont(_T("Arial,15,b,204"), &lf, dc);
    font_.CreateFontIndirect(&lf);

    CRect clientRect;
    GetClientRect(&clientRect);

    backBufferDc_.CreateCompatibleDC(dc);
    backBufferBm_.CreateCompatibleBitmap(dc, clientRect.Width(), clientRect.Height());
    oldBm_ = backBufferDc_.SelectBitmap(backBufferBm_);

    updateBackBuffer();
    return 0;
}

LRESULT CDragndropOverlay::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    backBufferDc_.SelectBitmap(oldBm_);
    return 0;
}

void CDragndropOverlay::updateBackBuffer() {
    CRect clientRect;
    GetClientRect(&clientRect);
    
    CBrush br;
    // We use layered extended style on Windows 8+
    bool isLayered = GetExStyle() & WS_EX_LAYERED;
    br.CreateSolidBrush(isLayered ? RGB(128, 128, 128): RGB(170, 170, 170));

    backBufferDc_.SetDCPenColor(RGB(255, 255, 255));
    backBufferDc_.FillRect(&clientRect, br);

    HFONT oldFont = backBufferDc_.SelectFont(font_);
    int index = 0;
    for (auto& item : items_) {
        if (activeItemIndex_ == index) {
            CBrush br2;
            br2.CreateSolidBrush(isLayered? RGB(100, 100, 100): RGB(130, 130, 130));

            HBRUSH oldBr = backBufferDc_.SelectBrush(br2);
            backBufferDc_.Rectangle(&item.rc);
            backBufferDc_.SelectBrush(oldBr);
        }
        COLORREF oldTextColor = backBufferDc_.SetTextColor(RGB(255, 255, 255));
        backBufferDc_.SetBkMode(TRANSPARENT);
        backBufferDc_.DrawText(item.text, -1, &item.rc, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        backBufferDc_.SetTextColor(oldTextColor);
        index++;
    }
   
    backBufferDc_.SelectFont(oldFont);
}

void CDragndropOverlay::calculateItemRectangles() {
    CRect clientRect;
    GetClientRect(&clientRect);
    int totalHeight = clientRect.Height(); 
    int top = 0;
    for (auto& item : items_) {
        int bottom;
        if (item.heighPerc != 0.0f) {
            bottom = top + static_cast<int>(item.heighPerc * totalHeight);
        } else {
            // The last element can take up the rest of the space
            bottom = totalHeight;
        }
        item.rc = { 0, top, clientRect.right, bottom };
        top = bottom;
    }
}

LRESULT CDragndropOverlay::OnEraseBkg(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    bHandled = true;
    return TRUE;
}

LRESULT CDragndropOverlay::OnMouseMove(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    int xPos = GET_X_LPARAM(lParam);
    int yPos = GET_Y_LPARAM(lParam);
    
    dragMove(xPos, yPos);
    return 0;
}

void CDragndropOverlay::addItem(ItemId id, float heightPerc, CString text) {
    items_.emplace_back(id, heightPerc, text);
    calculateItemRectangles();
}

void CDragndropOverlay::dragMove(int x, int y) {
    POINT pt{ x, y };

    int index = 0;
    int oldIndex = activeItemIndex_;
    activeItemIndex_ = -1;
    for (auto& item : items_) {
        if (PtInRect(&item.rc, pt)) {
            activeItemIndex_ = index;
        }

        index++;
    }
    if (oldIndex != activeItemIndex_) {
        updateBackBuffer();
        InvalidateRect(nullptr);
    }
}

std::optional<CDragndropOverlay::ItemId> CDragndropOverlay::itemAtPos(int x, int y) {
    POINT pt{ x, y };

    for (auto& item : items_) {
        if (PtInRect(&item.rc, pt)) {
            return item.id;
        }
    }
    return std::nullopt;
}
