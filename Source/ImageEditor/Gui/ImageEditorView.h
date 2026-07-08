#ifndef IMAGEEDITOR_IMAGEEDITORVIEW_H
#define IMAGEEDITOR_IMAGEEDITORVIEW_H

// ImageEditorView.h : interface of the CImageEditorView class
//
/////////////////////////////////////////////////////////////////////////////
#include <map>
#include "ImageEditor/Canvas.h"
#include <map>
#include <atlscrl.h>
#pragma once

namespace ImageEditor {

class CImageEditorView : public CScrollWindowImpl<CImageEditorView>, public ImageEditor::Canvas::Callback
{
    public:
        typedef CScrollWindowImpl<CImageEditorView> TBase;
        DECLARE_WND_CLASS_EX(L"CImageEditorView", CS_HREDRAW | CS_VREDRAW | CS_DBLCLKS, COLOR_APPWORKSPACE)
        enum { ID_AUTOSCROLL_TIMER = 100, AUTOSCROLL_INTERVAL_MS = 15  };
        CImageEditorView();
        BOOL PreTranslateMessage(MSG* pMsg);

        BEGIN_MSG_MAP(CImageEditorView)
            MESSAGE_HANDLER( WM_CREATE, OnCreate )
            MESSAGE_HANDLER( WM_PAINT, OnPaint )
            MESSAGE_HANDLER( WM_MOUSEMOVE, OnMouseMove )
            MESSAGE_HANDLER( WM_LBUTTONDOWN, OnLButtonDown )
            MESSAGE_HANDLER( WM_LBUTTONUP, OnLButtonUp )
            MESSAGE_HANDLER( WM_RBUTTONUP, OnRButtonUp )
            MESSAGE_HANDLER( WM_LBUTTONDBLCLK , OnLButtonDblClick )
            MESSAGE_HANDLER( WM_ERASEBKGND, OnEraseBackground )
            MESSAGE_HANDLER( WM_CONTEXTMENU, OnContextMenu )
            MESSAGE_HANDLER( WM_SETCURSOR, OnSetCursor )
            MESSAGE_HANDLER( WM_KEYDOWN, OnKeyDown )
            MESSAGE_HANDLER( WM_KEYUP, OnKeyUp )
            MESSAGE_HANDLER(WM_TIMER, OnTimer)
            //MESSAGE_HANDLER( WM_SIZE, OnSize )
            REFLECT_NOTIFICATIONS()
            CHAIN_MSG_MAP(TBase);
        END_MSG_MAP()

        // Handler prototypes (uncomment arguments if needed):
        //    LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
        //    LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
        //    LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)
        void setCanvas(ImageEditor::Canvas *canvas);
        void updateView(Canvas* canvas, Gdiplus::Rect rect, bool fullRender) override;
    protected:
        
        LRESULT OnPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnMouseMove(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnLButtonDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnLButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnRButtonUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnLButtonDblClick(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnEraseBackground(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnContextMenu(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnSetCursor(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnKeyDown(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnKeyUp(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
        LRESULT OnSize(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);

        POINT oldPoint{};
        void canvasSizeChanged() override;
    private:
        Canvas *canvas_;
        std::map<CursorType, CCursor> cursorCache_;
        std::map<int,CIcon> brushCursorCache_;
        CBrush backgroundBrush_;
        HCURSOR getCachedCursor(CursorType cursorType);
        bool mouseDown_;
        float dpiScaleX_, dpiScaleY_;
        int  scrollDx_ = 0, scrollDy_ = 0;
        POINT lastCursorScreen_ = {0, 0};
        DWORD lastCursorTick_ = 0;
        double velLeadX_ = 0.0, velLeadY_ = 0.0;
        HICON createBrushCursor(int size);
        static int speedFromDistance(int dist, int maxDist = 150, int maxSpeed = 40);
        static double positionalSpeed(int distOutside);
        void computeAutoScrollDelta(int cx, int cy, const RECT& rc, int& dx, int& dy) const;
        bool isAutoScrollActive_ = false;
};

}
#endif // IMAGEEDITOR_IMAGEEDITORVIEW_H
