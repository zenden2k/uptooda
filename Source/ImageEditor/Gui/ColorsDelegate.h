#ifndef IMAGEEDITOR_GUI_COLORSDELEGATE_H
#define IMAGEEDITOR_GUI_COLORSDELEGATE_H

#include "atlheaders.h"
#include "Toolbar.h"
#include "../Canvas.h"
#include "3rdpart/GdiplusH.h"
#include "3rdpart/ColorButton.h"

namespace ImageEditor {
    class ColorsDelegate: public Toolbar::ToolbarItemDelegate, public CColorButtonListener {
    public:
        enum {kOffset = 7, kSquareSize = 16, kPadding = 3};

        ColorsDelegate(Toolbar* toolbar, int itemIndex, Canvas* canvas);
        SIZE CalcItemSize(Toolbar::Item& item, int x, int y, float dpiScaleX, float dpiScaleY) override;
        void DrawItem(Toolbar::Item& item, Gdiplus::Graphics* gr, int x, int y, float dpiScaleX, float dpiScaleY) override;
        std::vector<std::pair<RECT, CString>> getSubItemsHints() override;
        void setForegroundColor(Gdiplus::Color color );
        void setBackgroundColor(Gdiplus::Color color);
        Gdiplus::Color getForegroundColor() const;
        Gdiplus::Color getBackgroundColor() const;
        int itemIndex() const;
        void OnClick(int x, int y, float dpiScaleX, float dpiScaleY) override;
        void swapColors();
        void onBeforeDialogOpen(CColorButton* btn, /*out*/ HWND* parent) override;
        void onAfterDialogOpen(CColorButton* btn) override;
    protected:
        Gdiplus::Color foregroundColor_;
        Gdiplus::Color backgroundColor_;
        Toolbar* toolbar_;
        CColorButton foregroundColorButton_;
        CColorButton backgroundColorButton_;
        CButton foregroundButton_;
        CButton backgroundButton_;
        Gdiplus::Rect backgroundRect_;
        Gdiplus::Rect foregroundRect_;
        Gdiplus::Rect swapColorsButtonRect_;
        int toolbarItemIndex_;
        Canvas* canvas_;
        CFont font_;
        std::unique_ptr<Gdiplus::Bitmap> swapColorsIcon_;

        void OnForegroundButtonSelChanged(COLORREF color, BOOL valid );

        void OnBackgroundButtonSelChanged(COLORREF color, BOOL valid );
        static Gdiplus::Rect getForegroundRect(int x, int y, float dpiScaleX, float dpiScaleY);
        static Gdiplus::Rect getSwapColorsRect(int x, int y, float dpiScaleX, float dpiScaleY);
    };
}

#endif
