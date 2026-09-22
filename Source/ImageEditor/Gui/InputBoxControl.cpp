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

#include "InputBoxControl.h"

#include <sstream>
#include <ComDef.h>
#include <strsafe.h>
//#include <dwrite.h>
//#include <dcommon.h>
#include <wincodec.h>
#include <Windows.ApplicationModel.Appointments.h>

#include "Core/Images/Utils.h"
#include "Gui/GuiTools.h"
#include "ImageEditor/Canvas.h"
#include "Core/i18n/Translator.h"
#include "Gui/Helpers/DPIHelper.h"

#ifndef TO_DEFAULTCOLOREMOJI
    #define TO_DEFAULTCOLOREMOJI 0x1000
#endif
#ifndef TO_DISPLAYFONTCOLOR
    #define TO_DISPLAYFONTCOLOR 0x2000
#endif

EXTERN_C const GUID DECLSPEC_SELECTANY IID_ITextServices = { 0x8d33f740, 0xcf58, 0x11ce, { 0xa8, 0x9d, 0x00, 0xaa, 0x00, 0x6c, 0xad, 0xc5 } };

//EXTERN_C const GUID DECLSPEC_SELECTANY IID_ITextHost = { 0xc5bdd8d0, 0xd26e, 0x11ce, { 0xa8, 0x9e, 0x00, 0xaa, 0x00, 0x6c, 0xad, 0xc5 } };

//EXTERN_C const GUID DECLSPEC_SELECTANY IID_ITextHost2 = { 0xc5bdd8d7, 0xd26e, 0x11ce, { 0xa8, 0x9e, 0x00, 0xaa, 0x00, 0x6c, 0xad, 0xc5 } };
const GUID IID_ITextHost = {
    0x13E670F4, // Data1
    0x1A5A, // Data2
    0x11CF, // Data3
    { 0xAB, 0xEB, 0x00, 0xAA, 0x00, 0xB6, 0x5E, 0xA1 } // Data4
};

const GUID IID_ITextHost2 = {
    0x13E670F5, // Data1
    0x1A5A, // Data2
    0x11CF, // Data3
    { 0xAB, 0xEB, 0x00, 0xAA, 0x00, 0xB6, 0x5E, 0xA1 } 
};
EXTERN_C const GUID DECLSPEC_SELECTANY IID_ITextServices2 = { 0x8d33f741, 0xcf58, 0x11ce, { 0xa8, 0x9d, 0x00, 0xaa, 0x00, 0x6c, 0xad, 0xc5 } };

EXTERN_C const GUID IID_IRicheditWindowlessAccessibility = {
    0x983E572D, 
    0x20CD,
    0x460B, 
    { 0x91, 0x04, 0x83, 0x11, 0x15, 0x92, 0xDD, 0x10 } 
};

namespace ImageEditor {

static HRESULT(STDAPICALLTYPE* pCreateTextServices)(IUnknown*, ITextHost*, IUnknown**) = nullptr;

CComPtr<ID2D1Bitmap> CreateD2DBitmapFromHBITMAP(ID2D1RenderTarget* target, HBITMAP hBitmap) {
    if (!hBitmap)
        return nullptr;

    BITMAP bmp {};
    GetObject(hBitmap, sizeof(BITMAP), &bmp);

    if (bmp.bmWidth == 0 || bmp.bmHeight == 0) {
        return {};
    }
    D2D1_BITMAP_PROPERTIES props = D2D1::BitmapProperties(
        D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED));

    CComPtr<IWICImagingFactory> wicFactory;
    CoCreateInstance(CLSID_WICImagingFactory, nullptr, CLSCTX_INPROC_SERVER,
        IID_PPV_ARGS(&wicFactory));

    CComPtr<IWICBitmap> wicBitmap;
    HRESULT hr = wicFactory->CreateBitmapFromHBITMAP(
        hBitmap, nullptr, WICBitmapUseAlpha, &wicBitmap);
    if (FAILED(hr))
        return nullptr;

    CComPtr<IWICFormatConverter> converter;
    wicFactory->CreateFormatConverter(&converter);
    converter->Initialize(wicBitmap, GUID_WICPixelFormat32bppPBGRA,
        WICBitmapDitherTypeNone, nullptr, 0.0f, WICBitmapPaletteTypeCustom);

    CComPtr<ID2D1Bitmap> d2dBitmap;
    CComPtr<IWICBitmapSource> source;
    converter.QueryInterface(&source);

    CComPtr<IWICBitmapLock> lock;
    UINT width, height;
    source->GetSize(&width, &height);

    D2D1_SIZE_U size = { width, height };

    target->CreateBitmapFromWicBitmap(source, &props, &d2dBitmap);

    return d2dBitmap;
}

RECT ScaleRectToLogical(const RECT& physicalRect, int dpi) {
    RECT logicalRect;
    logicalRect.left = MulDiv(physicalRect.left, 96, dpi);
    logicalRect.top = MulDiv(physicalRect.top, 96, dpi);
    logicalRect.right = MulDiv(physicalRect.right, 96, dpi);
    logicalRect.bottom = MulDiv(physicalRect.bottom,96, dpi);
    return logicalRect;
}

POINT ScalePointToLogical(const POINT& physicalPoint, int dpi) {
    POINT logicalPoint;
    logicalPoint.x = MulDiv(physicalPoint.x, 96, dpi);
    logicalPoint.y = MulDiv(physicalPoint.y, 96, dpi);

    return logicalPoint;
}

InputBoxControl::InputBoxControl(Canvas* canvas):
    canvas_(canvas)
{
    d2dMode_ = false;
    ZeroMemory(&charFormat_, sizeof(charFormat_));
    ZeroMemory(&paraFormat_, sizeof(paraFormat_));
    ZeroMemory(&logFont_, sizeof(logFont_));

    cursor_ = ::LoadCursor(nullptr, IDC_IBEAM);
    richEditUIA_ = std::make_unique<WindowlessRichEditUIA>(this);
}

InputBoxControl::~InputBoxControl() {
    if (IsWindow())
        DestroyWindow();
    Destroy();
}

HWND InputBoxControl::Create(HWND hParent, const RECT& rc, DWORD style, DWORD exStyle) {
    d2dMode_ = IsWindows8OrGreater();
    HMODULE h  = ::LoadLibrary(_T("msftedit.dll"));
    if (!h) {
        return nullptr;
    }
    pCreateTextServices = reinterpret_cast<decltype(pCreateTextServices)>(::GetProcAddress(h, "CreateTextServices"));
    if (!pCreateTextServices)
        return nullptr;
    RECT rcCopy = rc;
    POINT scrollOffset = canvas_ ? canvas_->GetScrollOffset() : POINT{};
    canvasOrigin_ = {rc.left + scrollOffset.x, rc.top + scrollOffset.y};
    CWindowImpl::Create(hParent, rcCopy, L"", style, exStyle);
    ::GetClientRect(m_hWnd, &clientRect_);
    if (!CreateTextServices()) {
        DestroyWindow();
        return nullptr;
    }
    richEditUIA_->Initialize(services_, hostWindow_, clientRect_);
    return m_hWnd;
}

void InputBoxControl::Destroy() {
    services_.Release();
    services2_.Release();
    servicesUnk_.Release();
    //UiaReturnRawElementProvider(hostWindow_, 0, 0, NULL);
    /*if (m_hWnd)
        DestroyWindow();*/
}

bool InputBoxControl::CreateTextServices() {
    if (!pCreateTextServices)
        return false;
    CComPtr<IUnknown> punk;
    HRESULT hr = pCreateTextServices(nullptr, static_cast<ITextHost*>(this), &punk);
    if (FAILED(hr))
        return false;

    servicesUnk_ = punk;
    hr = punk->QueryInterface(IID_ITextServices, (void**)&services_);
    if (FAILED(hr))
        return false;

    hr = punk->QueryInterface(IID_ITextServices2, (void**)&services2_);

    ApplyDefaults();
    return true;
}

void InputBoxControl::ApplyDefaults() {
    if (!services_)
        return;

    LRESULT editStyle = 0;

    //services_->TxSendMessage(EM_SETEDITSTYLE, SES_SCROLLONKILLFOCUS, SES_SCROLLONKILLFOCUS, nullptr);
    
    if (d2dMode_ ) {
        services_->TxSendMessage(EM_SETEDITSTYLE, 0, SES_LOGICALCARET, nullptr);
    }
    services_->TxSendMessage(EM_GETEDITSTYLE, 0, 0, &editStyle);

    // Прозрачный фон, шрифт/параграф, событийная маска
    services_->TxSendMessage(EM_SETBKGNDCOLOR, 0, CLR_NONE, nullptr);
    services_->TxSendMessage(EM_SETCHARFORMAT, SCF_ALL, (LPARAM)&charFormat_, nullptr);
    services_->TxSendMessage(EM_SETPARAFORMAT, 0, (LPARAM)&paraFormat_, nullptr);

    DWORD mask = ENM_CHANGE | ENM_REQUESTRESIZE | ENM_SELCHANGE;
    services_->TxSendMessage(EM_SETEVENTMASK, 0, mask, nullptr);

    // Многострочность, автоскролл, не скрывать выделение
    LONG style = ES_MULTILINE | ES_AUTOVSCROLL  | ES_NOHIDESEL |  ES_WANTRETURN;
    services_->TxSendMessage(EM_SETOPTIONS, ECOOP_OR, style, nullptr);

    // ВАЖНО: Включаем поддержку emoji и улучшенного рендеринга
    //services_->TxSendMessage(EM_SETTEXTMODE, TM_RICHTEXT, 0, nullptr); // RTF режим вместо plaintext

    // Включаем расширенную типографику для emoji
    LONG langOptions = IMF_UIFONTS | IMF_AUTOFONT | IMF_DUALFONT;
    services_->TxSendMessage(EM_SETLANGOPTIONS, 0, langOptions, nullptr);

    // Плейнтекст режим — при необходимости можно убрать, если нужен RTF ввод по умолчанию
    //services_->TxSendMessage(EM_SETTEXTMODE, TM_PLAINTEXT, 0, nullptr);
    services_->TxSendMessage(EM_SETZOOM, 0, 0, nullptr);
  
    services_->TxSendMessage(EM_SETTYPOGRAPHYOPTIONS, TO_DEFAULTCOLOREMOJI | TO_DISPLAYFONTCOLOR,
        TO_DEFAULTCOLOREMOJI | TO_DISPLAYFONTCOLOR, nullptr);
}

// InputBox
void InputBoxControl::show(bool show) {
    if (visible_ == show)
        return;
    visible_ = show;
    if (!show) {
        if (services_) {
            services_->TxSendMessage(EM_SETSEL, 0, 0, nullptr);
        }
    }
    // The HWND only hosts text services and timers; pixels are drawn by TextElement.
    ShowWindow(SW_HIDE);
    if (services_) {
        if (show) {
            services_->OnTxInPlaceActivate(nullptr);
            services_->TxSendMessage(WM_SETFOCUS, 0, 0, nullptr);
            TxShowCaret(TRUE);
        } else {
            TxShowCaret(FALSE);
            services_->TxSendMessage(WM_KILLFOCUS, 0, 0, nullptr);
            services_->OnTxInPlaceDeactivate();
        }
    }
    invalidate();
}

void InputBoxControl::resize(int x, int y, int w, int h, std::vector<MovableElement::Grip> grips) {
    grips_ = std::move(grips);
    canvasOrigin_ = {x, y};
    POINT scrollOffset { 0, 0 };
    if (canvas_)
        scrollOffset = canvas_->GetScrollOffset();

    UINT flags = SWP_NOZORDER | SWP_NOACTIVATE;
    if (w < 0 || h < 0) {
        flags |= SWP_NOSIZE;
    }
    ::SetWindowPos(m_hWnd, 0, x - scrollOffset.x, y - scrollOffset.y, w, h, flags);
    ::GetClientRect(m_hWnd, &clientRect_);
}

bool InputBoxControl::CreateD2DBitmapFromGdiplus(Gdiplus::Bitmap* gdipBitmap, Gdiplus::Rect sourceRect, ID2D1Bitmap** d2dBitmap) {
    if (!renderTarget_ || !gdipBitmap || sourceRect.IsEmptyArea()) {
        return false;
    }

    // Получаем данные пикселей
    Gdiplus::BitmapData bitmapData;
    //Gdiplus::Rect lockRect(0, 0, sourceRect.Width, sourceRect.Height);
    if (gdipBitmap->LockBits(&sourceRect, Gdiplus::ImageLockModeRead, PixelFormat32bppPARGB, &bitmapData) == Gdiplus::Ok) {
        FLOAT dpiX = 96.0f, dpiY = 96.0f;
        //renderTarget_->GetDpi(&dpiX, &dpiY);

        // Создаем D2D bitmap
        D2D1_BITMAP_PROPERTIES bitmapProps = D2D1::BitmapProperties(
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED),
            dpiX, dpiY);

        HRESULT hr = renderTarget_->CreateBitmap(
            D2D1::SizeU(sourceRect.Width, sourceRect.Height),
            bitmapData.Scan0,
            bitmapData.Stride,
            bitmapProps,
            d2dBitmap);

        gdipBitmap->UnlockBits(&bitmapData);
        return SUCCEEDED(hr);
    }

    return false;
}

void InputBoxControl::render(Gdiplus::Graphics* graphics, Gdiplus::Bitmap* background, Gdiplus::Rect layoutArea) {
    Gdiplus::Rect rc = canvas_->currentRenderingRect();
    Gdiplus::Rect rcRelative = rc;
    rcRelative.Intersect(layoutArea);
    rcRelative.Offset(-layoutArea.GetLeft(), -layoutArea.GetTop());

    CRect updateRect { rc.GetLeft(), rc.GetTop(), rc.GetRight(), rc.GetBottom() };
    CRect updateRectRelative { rcRelative.GetLeft(), rcRelative.GetTop(), rcRelative.GetRight(), rcRelative.GetBottom() };;


    if (!d2dMode_ || !InitializeD2D()) {
        HDC mainHdc = graphics->GetHDC();

        // Создаем memory DC и bitmap
        CDC memHdc;
        memHdc.CreateCompatibleDC(mainHdc);

        // Создаем DIB section для лучшего контроля
        BITMAPINFO bmi = {};
        bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bmi.bmiHeader.biWidth = layoutArea.Width;
        bmi.bmiHeader.biHeight = -layoutArea.Height; // Отрицательное значение для top-down bitmap
        bmi.bmiHeader.biPlanes = 1;
        bmi.bmiHeader.biBitCount = 32;
        bmi.bmiHeader.biCompression = BI_RGB;

        void* bits = nullptr;
        CBitmap textBitmap;
        textBitmap.CreateDIBSection(memHdc, &bmi, DIB_RGB_COLORS, &bits, nullptr, 0);
        HBITMAP oldBitmap = memHdc.SelectBitmap(textBitmap);

        // Копируем фон с основного graphics
        if (background) {
            Gdiplus::Graphics bgGraphics(memHdc);
            bgGraphics.DrawImage(background,
                Gdiplus::Rect(0, 0, layoutArea.Width, layoutArea.Height),
                layoutArea.X, layoutArea.Y, layoutArea.Width, layoutArea.Height,
                Gdiplus::UnitPixel);
        } else {
            // Заливаем нужным цветом
            RECT fillRect = { 0, 0, layoutArea.Width, layoutArea.Height };
            CBrush brush;
            brush.CreateSolidBrush(RGB(255, 255, 255)); // Или нужный вам цвет
            memHdc.FillRect(&fillRect, brush);
        }

        // Настройки рендеринга
        memHdc.SetBkMode(TRANSPARENT);

        // Рендерим текст
        RECTL rc = { 0, 0, layoutArea.Width, layoutArea.Height };
        services_->TxDraw(DVASPECT_CONTENT, 0, nullptr, nullptr, memHdc, nullptr, &rc, nullptr, nullptr, nullptr, 0, 0);
        if (visible_ && caretVisible_ && caretCreated_ && caretBlinkOn_) {
            const int caretWidth = std::max(1, caretWidth_);
            if (isCaretItalic()) {
                const int slant = std::max(1, caretHeight_ / 4);
                POINT points[] = { { caretPos_.x + slant, caretPos_.y },
                                   { caretPos_.x + slant + caretWidth, caretPos_.y },
                                   { caretPos_.x + caretWidth, caretPos_.y + caretHeight_ },
                                   { caretPos_.x, caretPos_.y + caretHeight_ } };
                CRgn caretRegion;
                caretRegion.CreatePolygonRgn(points, 4, WINDING);
                if (caretRegion) {
                    memHdc.FillRgn(caretRegion, static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)));
                }
            } else {
                RECT caretRect = { caretPos_.x, caretPos_.y, caretPos_.x + caretWidth, caretPos_.y + caretHeight_ };
                memHdc.FillRect(&caretRect, static_cast<HBRUSH>(::GetStockObject(BLACK_BRUSH)));
            }
        }
        graphics->ReleaseHDC(mainHdc);
        // Конвертируем в GDI+ Bitmap и рисуем
        Gdiplus::Bitmap gdipBitmap(textBitmap, nullptr);
        graphics->DrawImage(&gdipBitmap, layoutArea.X, layoutArea.Y);

        // Очистка
        memHdc.SelectBitmap(oldBitmap);
        return;
    }

    if (!services2_) {
        return;
    }

    HDC mainHdc = graphics->GetHDC();

    if (!mainHdc) {
        return;
    }

    RECT bindRect = { layoutArea.X, layoutArea.Y, layoutArea.GetRight(), layoutArea.GetBottom() };
    HRESULT hr = renderTarget_->BindDC(mainHdc, &bindRect);
    if (FAILED(hr)) {
        graphics->ReleaseHDC(mainHdc);
        return;
    }

    renderTarget_->BeginDraw();
    FLOAT dpiX = 96.0f, dpiY = 96.0f;
    renderTarget_->GetDpi(&dpiX, &dpiY);

    CRect bgRect = ScaleRectToLogical(updateRectRelative, dpiX);
    if (background) {
        CComPtr<ID2D1Bitmap> d2dBackground;
        rc.Offset(-layoutArea.X, -layoutArea.Y);

        if (CreateD2DBitmapFromGdiplus(background, rcRelative, &d2dBackground)) {
            //D2D1_RECT_F destRect = D2D1::RectF(bgRect.left, bgRect.top, static_cast<FLOAT>(bgRect.right), static_cast<FLOAT>(bgRect.bottom));
            D2D1_RECT_F destRect = D2D1::RectF(bgRect.left, bgRect.top, static_cast<FLOAT>(bgRect.right), static_cast<FLOAT>(bgRect.bottom));
            renderTarget_->DrawBitmap(d2dBackground, destRect);
        }
    } else {
        // Заливаем белым фоном
        CComPtr<ID2D1SolidColorBrush> whiteBrush;
        renderTarget_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &whiteBrush);
        D2D1_RECT_F rect = D2D1::RectF(0, 0, (FLOAT)bgRect.Width(), (FLOAT)bgRect.Height());
        renderTarget_->FillRectangle(rect, whiteBrush);
    }

    // Теперь рендерим текст поверх подготовленного фона
    RECTL textRect = { 0, 0, layoutArea.Width, layoutArea.Height };

    //updateRect = ScaleRectToLogical(updateRect, dpiX);

    services2_->TxDrawD2D(renderTarget_, &textRect, &updateRectRelative, 0);

    if (visible_ && caretVisible_ && caretCreated_ && caretBlinkOn_) {
        CComPtr<ID2D1SolidColorBrush> caretBrush;
        if (SUCCEEDED(renderTarget_->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::Black), &caretBrush))) {
            if (isCaretItalic()) {
                const FLOAT slant = static_cast<FLOAT>(std::max(1, caretHeight_ / 4));
                renderTarget_->DrawLine(
                    D2D1::Point2F(static_cast<FLOAT>(caretPos_.x) + slant, static_cast<FLOAT>(caretPos_.y)),
                    D2D1::Point2F(static_cast<FLOAT>(caretPos_.x), static_cast<FLOAT>(caretPos_.y + caretHeight_)),
                    caretBrush, static_cast<FLOAT>(std::max(1, caretWidth_)));
            } else {
                const D2D1_RECT_F caretRect
                    = D2D1::RectF(static_cast<FLOAT>(caretPos_.x), static_cast<FLOAT>(caretPos_.y),
                                  static_cast<FLOAT>(caretPos_.x + std::max(1, caretWidth_)),
                                  static_cast<FLOAT>(caretPos_.y + caretHeight_));
                renderTarget_->FillRectangle(caretRect, caretBrush);
            }
        }
    }

    renderTarget_->EndDraw();
    graphics->ReleaseHDC(mainHdc);
}

bool InputBoxControl::isCaretItalic() {
    CHARFORMAT2 format { };
    format.cbSize = sizeof(format);
    if (services_
        && SUCCEEDED(
            services_->TxSendMessage(EM_GETCHARFORMAT, SCF_SELECTION, reinterpret_cast<LPARAM>(&format), nullptr))) {
        return (format.dwMask & CFM_ITALIC) && (format.dwEffects & CFE_ITALIC);
    }
    return (charFormat_.dwEffects & CFE_ITALIC) != 0;
}

bool InputBoxControl::isVisible() { return visible_; }

void InputBoxControl::invalidate() {
    if (canvas_ && visible_ && clientRect_.right > 0 && clientRect_.bottom > 0) {
        RECT rect = { canvasOrigin_.x, canvasOrigin_.y, canvasOrigin_.x + clientRect_.right,
                      canvasOrigin_.y + clientRect_.bottom };
        canvas_->updateView(rect);
    }
}

LRESULT InputBoxControl::handleMessage(UINT message, WPARAM wParam, LPARAM lParam) {
    BOOL handled = TRUE;
    if (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST)
        return OnMouse(message, wParam, lParam, handled);
    if (message >= WM_KEYFIRST && message <= WM_KEYLAST)
        return OnKey(message, wParam, lParam, handled);
    if ((message >= WM_IME_STARTCOMPOSITION && message <= WM_IME_ENDCOMPOSITION) || message == WM_IME_CHAR
        || message == WM_IME_NOTIFY || message == WM_IME_REQUEST)
        return OnIme(message, wParam, lParam, handled);
    if (message == WM_CONTEXTMENU)
        return OnContextMenu(message, wParam, lParam, handled);
    return 0;
}

void InputBoxControl::setTextColor(Gdiplus::Color color) {
    textColor_ = color.ToCOLORREF();
    charFormat_.crTextColor = textColor_;
    charFormat_.dwMask |= CFM_COLOR;
    if (services_) {
        services_->TxSendMessage(EM_SETCHARFORMAT, SCF_SELECTION, (LPARAM)&charFormat_, nullptr);
    }
}

void InputBoxControl::setFont(LOGFONT font, DWORD changeMask) {
    LOG(WARNING) << "setFont mask=" << std::hex << changeMask;
    logFont_ = font;
    charFormat_.cbSize = sizeof(charFormat_);
    // Расширенная маска для всех стилей
    DWORD defaultMask = CFM_FACE | CFM_SIZE | CFM_CHARSET | CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE | CFM_STRIKEOUT;
    charFormat_.dwMask |= (changeMask ? changeMask : defaultMask);

    if (logFont_.lfHeight != 0) {
        int dpi = DPIHelper::GetDpiForWindow(m_hWnd);
        int pointSize = MulDiv(-logFont_.lfHeight, 72, dpi);
        charFormat_.yHeight = pointSize * 20;
    }

    wcsncpy_s(charFormat_.szFaceName, logFont_.lfFaceName, _TRUNCATE);

    charFormat_.dwEffects = 0; 

    if (logFont_.lfWeight >= FW_BOLD) {
        charFormat_.dwEffects |= CFE_BOLD;
    }

    if (logFont_.lfItalic) {
        charFormat_.dwEffects |= CFE_ITALIC;
    }

    if (logFont_.lfUnderline) {
        charFormat_.dwEffects |= CFE_UNDERLINE;
    }

    if (logFont_.lfStrikeOut) {
        charFormat_.dwEffects |= CFE_STRIKEOUT;
    }

    if (services_) {
        CHARRANGE selection = { 0 };

        if (services_) {
            services_->TxSendMessage(EM_EXGETSEL, 0, (LPARAM)&selection, nullptr);
        }

        bool hasSelection = (selection.cpMax > selection.cpMin);

        WPARAM flags = 0;
        if (visible_) {
            flags = SCF_SELECTION;
            
                /* if (hasSelection) {
                flags = SCF_SELECTION | SCF_DEFAULT;
            } else {
                flags = SCF_DEFAULT;
            }*/
        } else {
            flags = SCF_ALL;
        }
        services_->TxSendMessage(EM_SETCHARFORMAT, flags, (LPARAM)&charFormat_, nullptr);
       
        //Invalidate();
    }
}

void InputBoxControl::setRawText(const std::string& text) {
    if (!services_)
        return;
    std::stringstream ss(text);
    EDITSTREAM es {};
    es.dwCookie = (DWORD_PTR)&ss;
    es.pfnCallback = &InputBoxControl::EditStreamInCallback;
    services_->TxSendMessage(EM_STREAMIN, SF_RTF, (LPARAM)&es, nullptr);
    onTextChanged(L"");
}

std::string InputBoxControl::getRawText() {
    if (!services_)
        return {};
    std::stringstream ss;
    EDITSTREAM es {};
    es.dwCookie = (DWORD_PTR)&ss;
    es.pfnCallback = &InputBoxControl::EditStreamOutCallback;
    services_->TxSendMessage(EM_STREAMOUT, SF_RTF, (LPARAM)&es, nullptr);
    return ss.str();
}

bool InputBoxControl::isEmpty() {
    if (!services_)
        return true;
    LRESULT len {};
    services_->TxSendMessage(WM_GETTEXTLENGTH, 0, 0, &len);
    return len == 0;
}

// Stream callbacks
DWORD CALLBACK InputBoxControl::EditStreamOutCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG* pcb) {
    auto* ss = reinterpret_cast<std::stringstream*>(dwCookie);
    ss->write((const char*)pbBuff, cb);
    *pcb = cb;
    return 0;
}

DWORD CALLBACK InputBoxControl::EditStreamInCallback(DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, LONG* pcb) {
    auto* ss = reinterpret_cast<std::stringstream*>(dwCookie);
    ss->read((char*)pbBuff, cb);
    *pcb = static_cast<LONG>(ss->gcount());
    return 0;
}

// WTL handlers
LRESULT InputBoxControl::OnCreate(UINT, WPARAM, LPARAM, BOOL& bHandled) {
    hostWindow_ = m_hWnd;
    int dpi = DPIHelper::GetDpiForWindow(m_hWnd);
    // Базовый шрифт: Segoe UI 11pt
    wcscpy_s(logFont_.lfFaceName, L"Segoe UI Emoji");
    logFont_.lfHeight = -MulDiv(11, dpi, 72);
    logFont_.lfWeight = FW_NORMAL;

    charFormat_.cbSize = sizeof(charFormat_);
    charFormat_.dwMask = CFM_FACE | CFM_SIZE | CFM_COLOR | CFM_CHARSET /* | CFM_WEIGHT*/;
    charFormat_.yHeight = 11 * 20;
    charFormat_.crTextColor = textColor_;
    charFormat_.bCharSet = DEFAULT_CHARSET;
    // charFormat_.dwMask |= CFM_BOLD;
    // charFormat_.dwEffects |= CFE_BOLD;  // для жирного
    // charFormat_.dwEffects &= ~CFE_BOLD; // для обычного
    wcsncpy_s(charFormat_.szFaceName, logFont_.lfFaceName, _TRUNCATE);

    paraFormat_.cbSize = sizeof(paraFormat_);
    paraFormat_.dwMask = PFM_ALIGNMENT;
    paraFormat_.wAlignment = PFA_LEFT;

    bHandled = FALSE; // пусть базовый обработчик выполнится
    return 0;
}
LRESULT InputBoxControl::OnDestroy(UINT, WPARAM, LPARAM, BOOL&) {
    Destroy();
    return 0;
}
LRESULT InputBoxControl::OnSize(UINT, WPARAM, LPARAM lParam, BOOL&) {
    int w = LOWORD(lParam), h = HIWORD(lParam);
    ::GetClientRect(m_hWnd, &clientRect_);
    if (services_) {
        services_->TxSendMessage(WM_SIZE, 0, lParam, nullptr);
        //onSizeChanged(w, h);
    }
    return 0;
}

LRESULT InputBoxControl::OnPaint(UINT, WPARAM, LPARAM, BOOL&) {
    CPaintDC dc(m_hWnd);
    RECT clientRect;
    GetClientRect(&clientRect);
    RECT rc = { clientRect.left, clientRect.top, clientRect.right, clientRect.bottom };
    RECTL rcPaint = { dc.m_ps.rcPaint.left, dc.m_ps.rcPaint.top, dc.m_ps.rcPaint.right, dc.m_ps.rcPaint.bottom };

    if (!d2dMode_ || !InitializeD2D()) {
        if (!services_ ) {
            return 0;
        }
        HRESULT hr = services_->TxDraw(DVASPECT_CONTENT, 0, nullptr, nullptr, dc, nullptr, reinterpret_cast<LPCRECTL>(&rc), nullptr, nullptr, nullptr, 0, 0);
        if (FAILED(hr)) {
            LOG(ERROR) << _com_error(hr).ErrorMessage();
        }
        
        return 0;
    }
   
    if (!services2_) {
        return 0;
    }

    HRESULT hr = renderTarget_->BindDC(dc, &rc);
    if (FAILED(hr)) {
        return 0;
    }
    FLOAT dpiX = 96, dpiY = 96;
    renderTarget_->GetDpi(&dpiX, &dpiY);
    renderTarget_->BeginDraw();

    //rc = ScaleRectToLogical(rc, dpi);
    POINT caretPos = ScalePointToLogical(caretPos_, dpiX);
    int caretWidth = MulDiv(caretWidth_, 96, dpiX);
    int caretHeight = MulDiv(caretHeight_, 96, dpiY);

    hr = services2_->TxDrawD2D(renderTarget_, reinterpret_cast<LPCRECTL>(&rc), nullptr, 0);

    if (caretVisible_ && caretCreated_ && caretBlinkOn_) {

        if (caretBitmap_  && !d2dCaretBitmap_) {
            d2dCaretBitmap_ = CreateD2DBitmapFromHBITMAP(renderTarget_, caretBitmap_);
        }

        if (caretBitmap_ && d2dCaretBitmap_) {
            D2D1_RECT_F destRect = D2D1::RectF(
                static_cast<FLOAT>(caretPos.x),
                static_cast<FLOAT>(caretPos.y),
                static_cast<FLOAT>(caretPos.x + caretWidth),
                static_cast<FLOAT>(caretPos.y + caretHeight));

            renderTarget_->DrawBitmap(d2dCaretBitmap_, destRect);
        } else {
            CComPtr<ID2D1SolidColorBrush> brush;
            renderTarget_->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::Black), &brush);

            D2D1_RECT_F caretRect = D2D1::RectF(
                static_cast<FLOAT>(caretPos.x),
                static_cast<FLOAT>(caretPos.y),
                static_cast<FLOAT>(caretPos.x + caretWidth_),
                static_cast<FLOAT>(caretPos.y + caretHeight_));

            renderTarget_->FillRectangle(&caretRect, brush);
        }
    }
    
    if (FAILED(hr)) {
        LOG(ERROR) << _com_error(hr).ErrorMessage();
    }
    HRESULT endHr = renderTarget_->EndDraw();

    return 0;
}

LRESULT InputBoxControl::OnSetFocus(UINT, WPARAM wParam, LPARAM, BOOL&) {
    DWORD oldMask = charFormat_.dwMask;
    charFormat_.dwMask = 0;
    if (services_) {
        services_->OnTxInPlaceActivate(nullptr);
        services_->TxSendMessage(WM_SETFOCUS, wParam, 0, nullptr);
    }
    TxShowCaret(TRUE);

    return 0;
}
LRESULT InputBoxControl::OnKillFocus(UINT, WPARAM wParam, LPARAM, BOOL&) {
    TxShowCaret(FALSE);
    //charFormat_.dwMask = 0;

    LRESULT result = 0;
    if (services_)
        services_->TxSendMessage(WM_KILLFOCUS, wParam, 0, &result);

    services_->OnTxInPlaceDeactivate();

    return result;
}

LRESULT InputBoxControl::OnTimer(UINT, WPARAM id, LPARAM lParam, BOOL&) {
    if (id == CARET_TIMER_ID) {
        caretBlinkOn_ = !caretBlinkOn_;
        if (caretVisible_) {
            invalidate();
        }
        return 0;
    }
    LRESULT result = 0;
    if (services_)
        services_->TxSendMessage(WM_TIMER, id, lParam, &result);
    return result;
}

LRESULT InputBoxControl::OnMouse(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    bHandled = false;
    LRESULT result = 0;
    if (services_)
        services_->TxSendMessage(uMsg, wParam, lParam, &result);
    return result;
}

LRESULT InputBoxControl::OnKey(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    if (uMsg == WM_KEYDOWN) {
        if (wParam == VK_ESCAPE) {
            onEditCanceled();
            bHandled = TRUE;
            return 0;
        }
        if (wParam == VK_RETURN && (GetKeyState(VK_CONTROL) & 0x8000)) {
            onEditFinished();
            bHandled = TRUE;
            return 0;
        }
    }

    LRESULT result = 0;
    if (services_)
        services_->TxSendMessage(uMsg, wParam, lParam, &result);
    return result;
}

LRESULT InputBoxControl::OnIme(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL&) {
    LRESULT result = 0;
    if (services_)
        services_->TxSendMessage(uMsg, wParam, lParam, &result);
    return result;
}

LRESULT InputBoxControl::OnContextMenu(UINT, WPARAM, LPARAM lParam, BOOL&) {
    int x = GET_X_LPARAM(lParam), y = GET_Y_LPARAM(lParam);

    if (x == -1 && y == -1) {
        POINT pt = caretPos_;
        ClientToScreen(&pt);
        x = pt.x;
        y = pt.y;
    }
    LRESULT canUndo = 0, canRedo = 0;
    CHARRANGE selection = { 0 };
    LRESULT textLength = 0;

    if (services_) {
        services_->TxSendMessage(EM_CANUNDO, 0, 0, &canUndo);
        services_->TxSendMessage(EM_CANREDO, 0, 0, &canRedo);
        services_->TxSendMessage(EM_EXGETSEL, 0, (LPARAM)&selection, nullptr);
        services_->TxSendMessage(WM_GETTEXTLENGTH, 0, 0, &textLength);
    }

    bool hasSelection = (selection.cpMax > selection.cpMin);
    bool hasText = (textLength > 0);
    bool canPaste = IsClipboardFormatAvailable(CF_TEXT) || IsClipboardFormatAvailable(CF_UNICODETEXT);

    CMenu contextMenu;
    contextMenu.CreatePopupMenu();
    contextMenu.AppendMenu(MF_STRING | (canUndo ? MF_ENABLED : MF_GRAYED),
        ID_EDIT_UNDO, TR("Undo"));
    contextMenu.AppendMenu(MF_STRING | (canRedo ? MF_ENABLED : MF_GRAYED),
        ID_EDIT_REDO, TR("Redo"));

    contextMenu.AppendMenu(MF_SEPARATOR);

    contextMenu.AppendMenu(MF_STRING | (hasSelection ? MF_ENABLED : MF_GRAYED),
        ID_EDIT_CUT, TR("Cut") + CString("\tCtrl+X"));
    contextMenu.AppendMenu(MF_STRING | (hasSelection ? MF_ENABLED : MF_GRAYED),
        ID_EDIT_COPY, TR("Copy") + CString("\tCtrl+C"));
    contextMenu.AppendMenu(MF_STRING | (canPaste ? MF_ENABLED : MF_GRAYED),
        ID_EDIT_PASTE, TR("Paste")+ CString("\tCtrl+V"));
    contextMenu.AppendMenu(MF_STRING | (hasSelection ? MF_ENABLED : MF_GRAYED),
        ID_EDIT_DELETE, TR("Delete")+ CString("\tDel"));

    contextMenu.AppendMenu(MF_SEPARATOR);

    contextMenu.AppendMenu(MF_STRING | (hasText ? MF_ENABLED : MF_GRAYED),
        ID_EDIT_SELECT_ALL, TR("Select All")+ CString("\tCtrl+A"));


    contextMenuOpened_ = true;
    UINT id = contextMenu.TrackPopupMenu(TPM_RETURNCMD | TPM_RIGHTBUTTON, x, y, m_hWnd);
    contextMenuOpened_ = false;

    switch (id) {
    case ID_EDIT_UNDO:
        services_->TxSendMessage(EM_UNDO, 0, 0, nullptr);
        break;

    case ID_EDIT_REDO:
        services_->TxSendMessage(EM_REDO, 0, 0, nullptr);
        break;

    case ID_EDIT_CUT:
        services_->TxSendMessage(WM_CUT, 0, 0, nullptr);
        break;

    case ID_EDIT_COPY:
        services_->TxSendMessage(WM_COPY, 0, 0, nullptr);
        break;

    case ID_EDIT_PASTE:
        //SetFocus();
        services_->TxSendMessage(WM_PASTE, 0, 0, nullptr);
        break;

    case ID_EDIT_DELETE:
        services_->TxSendMessage(WM_CLEAR, 0, 0, nullptr);
        break;

    case ID_EDIT_SELECT_ALL:
        services_->TxSendMessage(EM_SETSEL, 0, -1, nullptr);
        break;

    default:
        break;
    }
    return 0;
}
LRESULT InputBoxControl::OnSetCursor(UINT, WPARAM, LPARAM, BOOL& bHandled) {
    if (contextMenuOpened_) {
        bHandled = TRUE;
        HCURSOR arrow = LoadCursor(nullptr, IDC_ARROW);
        ::SetCursor(arrow);
        return TRUE;
    }

    POINT position {};
    GetCursorPos(&position);
    ScreenToClient(&position);

    CClientDC hdc(m_hWnd);

    RECT rect = { 0 };
    GetClientRect(&rect);

    services_->OnTxSetCursor(
        DVASPECT_CONTENT,
        0,
        nullptr,
        nullptr,
        hdc,
        nullptr,
        &rect,
        position.x,
        position.y);

    return 0;
}

LRESULT InputBoxControl::OnGetObject(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    return 0;
    if ((LONG)lParam == UiaRootObjectId) {
        /*CComQIPtr<IRicheditWindowlessAccessibility> accesibility = services_;
        /* if (accesibility) {
            accesibility->CreateProvider()
        }
        CComQIPtr<IRawElementProviderSimple> pProvider = services_;
        if (pProvider) {
            return UiaReturnRawElementProvider(m_hWnd, wParam, lParam, pProvider);
        }*/
        if (richEditUIA_ && richEditUIA_->GetUiaProvider()) {
            return UiaReturnRawElementProvider(hostWindow_, wParam, lParam,
                richEditUIA_->GetUiaProvider());
        }
    } else if ((LONG)lParam == OBJID_CLIENT) {
        CComQIPtr<IAccessible> pAcc = services_;
        if (pAcc) {
            return LresultFromObject(IID_IAccessible, wParam, pAcc);
        }
    }
    return 0;
}

LRESULT InputBoxControl::OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    renderTarget_.Release();
    return 0;
}

bool InputBoxControl::InitializeD2D() {
    if (!d2dFactory_) {
        HRESULT hr = D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &d2dFactory_);
        if (FAILED(hr)) {
            d2dMode_ = false;
            return false;
        }
    }

    if (!renderTarget_) {
        int dpi = DPIHelper::GetDpiForWindow(hostWindow_);

        D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties(
            D2D1_RENDER_TARGET_TYPE_DEFAULT,
            D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED), dpi, dpi);

        HRESULT hr = d2dFactory_->CreateDCRenderTarget(&props, &renderTarget_);
        return SUCCEEDED(hr);
    }
    return true;
}

// IUnknown
STDMETHODIMP InputBoxControl::QueryInterface(REFIID riid, void** ppv) {
    if (!ppv) {
        return E_POINTER;
    }

    *ppv = nullptr;
    if (riid == IID_IUnknown || riid == IID_ITextHost) {
        *ppv = static_cast<ITextHost*>(this); // Windows 7
        AddRef();
        return S_OK;
    }

    // (Windows 8+)
    if (riid == IID_ITextHost2) {
        *ppv = static_cast<ITextHost2*>(this);
        AddRef();
        return S_OK;
    }

    return E_NOINTERFACE;
}

// ITextHost
HDC InputBoxControl::TxGetDC() {
    return ::GetDC(m_hWnd);
}

INT InputBoxControl::TxReleaseDC(HDC hdc) {
    return ::ReleaseDC(m_hWnd, hdc);
}

void InputBoxControl::TxInvalidateRect(LPCRECT prc, BOOL) {
    invalidate();
}

void InputBoxControl::TxViewChange(BOOL fUpdate) {
    if (fUpdate)
        invalidate();
}

BOOL InputBoxControl::TxCreateCaret(HBITMAP hbmp, INT xWidth, INT yHeight) {
    int dpi = DPIHelper::GetDpiForWindow(m_hWnd);
    caretWidth_ = MulDiv(xWidth, USER_DEFAULT_SCREEN_DPI, dpi);
    caretHeight_ = MulDiv(yHeight,  USER_DEFAULT_SCREEN_DPI, dpi);

    /*caretWidth_ = xWidth;
    caretHeight_ = yHeight;*/

    caretCreated_ = true;
    if (d2dMode_) {
        if (caretBitmap_ != hbmp) {
            d2dCaretBitmap_.Release();
        }

        if (hbmp == reinterpret_cast<HBITMAP>(0x120)) {
            caretBitmap_ = nullptr;

        } else {
            caretBitmap_ = hbmp;
        }
    }
    return TRUE;
}

BOOL InputBoxControl::TxShowCaret(BOOL fShow) {
    caretVisible_ = fShow;
    if (fShow) {
        caretBlinkOn_ = true;
        SetTimer(CARET_TIMER_ID, GetCaretBlinkTime(), nullptr);
    } else {
        KillTimer(CARET_TIMER_ID);
    }
    invalidate();
    return TRUE;
}

BOOL InputBoxControl::TxSetCaretPos(INT x, INT y) {
    int dpi = DPIHelper::GetDpiForWindow(m_hWnd);
    x = MulDiv(x, USER_DEFAULT_SCREEN_DPI, dpi);
    y = MulDiv(y,  USER_DEFAULT_SCREEN_DPI, dpi);
    caretPos_ = { x, y };
    invalidate();
    return TRUE;
}

BOOL InputBoxControl::TxSetTimer(UINT idTimer, UINT uTimeout) { return SetTimer(idTimer, uTimeout) != 0; }
void InputBoxControl::TxKillTimer(UINT idTimer) {
    if (IsWindow())
    KillTimer(idTimer);
}

void InputBoxControl::TxSetCapture(BOOL fCapture) {
    // CImageEditorView owns capture while forwarding a text selection drag.
}

void InputBoxControl::TxSetFocus() {
    ::SetFocus(GetParent());
}

void InputBoxControl::TxSetCursor(HCURSOR hcur, BOOL fText) {
    cursor_ = hcur;
    ::SetCursor(hcur ? hcur : ::LoadCursor(nullptr, IDC_ARROW));
}

HRESULT InputBoxControl::TxGetClientRect(LPRECT prc) {
    GetClientRect(&clientRect_);
    *prc = clientRect_;
    return S_OK;
}

HRESULT InputBoxControl::TxGetExtent(LPSIZEL lpExtent) {
    int dpi = DPIHelper::GetDpiForWindow(m_hWnd);
    lpExtent->cx = MulDiv(clientRect_.right - clientRect_.left, 2540, dpi);
    lpExtent->cy = MulDiv(clientRect_.bottom - clientRect_.top, 2540, dpi);

    //lpExtent->cx = lpExtent->cy = 0;
    return S_OK;
}

HRESULT InputBoxControl::OnTxCharFormatChange(CONST CHARFORMATW* pCF) {
    if (pCF) {
        DWORD mask = pCF->dwMask;

        // Цвет текста
        if (mask & CFM_COLOR) {
            textColor_ = pCF->crTextColor;
            charFormat_.crTextColor = pCF->crTextColor;
            charFormat_.dwMask |= CFM_COLOR;
        }

        // Имя шрифта
        if (mask & CFM_FACE) {
            wcsncpy_s(charFormat_.szFaceName, pCF->szFaceName, _TRUNCATE);
            wcsncpy_s(logFont_.lfFaceName, pCF->szFaceName, _TRUNCATE);
            charFormat_.dwMask |= CFM_FACE;
        }

        // Размер шрифта
        if (mask & CFM_SIZE) {
            charFormat_.yHeight = pCF->yHeight;
            charFormat_.dwMask |= CFM_SIZE;

            // Обновляем LOGFONT
            int dpi = DPIHelper::GetDpiForWindow(m_hWnd);
            int pointSize = pCF->yHeight / 20; // yHeight в twips (1/20 пункта)
            logFont_.lfHeight = -MulDiv(pointSize, dpi, 72);
        }

        // Стили текста (жирный, курсив, подчеркнутый, зачеркнутый)
        if (mask & (CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE | CFM_STRIKEOUT)) {
            charFormat_.dwEffects = pCF->dwEffects;
            charFormat_.dwMask |= (mask & (CFM_BOLD | CFM_ITALIC | CFM_UNDERLINE | CFM_STRIKEOUT));

            // Обновляем LOGFONT
            logFont_.lfWeight = (pCF->dwEffects & CFE_BOLD) ? FW_BOLD : FW_NORMAL;
            logFont_.lfItalic = (pCF->dwEffects & CFE_ITALIC) ? TRUE : FALSE;
            logFont_.lfUnderline = (pCF->dwEffects & CFE_UNDERLINE) ? TRUE : FALSE;
            logFont_.lfStrikeOut = (pCF->dwEffects & CFE_STRIKEOUT) ? TRUE : FALSE;
        }

        // Набор символов
        if (mask & CFM_CHARSET) {
            charFormat_.bCharSet = pCF->bCharSet;
            charFormat_.dwMask |= CFM_CHARSET;
            logFont_.lfCharSet = pCF->bCharSet;
        }

        // Защищенный текст
        if (mask & CFM_PROTECTED) {
            if (pCF->dwEffects & CFE_PROTECTED) {
                charFormat_.dwEffects |= CFE_PROTECTED;
            } else {
                charFormat_.dwEffects &= ~CFE_PROTECTED;
            }
            charFormat_.dwMask |= CFM_PROTECTED;
        }

        // Ссылка
        if (mask & CFM_LINK) {
            if (pCF->dwEffects & CFE_LINK) {
                charFormat_.dwEffects |= CFE_LINK;
            } else {
                charFormat_.dwEffects &= ~CFE_LINK;
            }
            charFormat_.dwMask |= CFM_LINK;
        }

        // Смещение (надстрочный/подстрочный текст)
        if (mask & CFM_OFFSET) {
            charFormat_.yOffset = pCF->yOffset;
            charFormat_.dwMask |= CFM_OFFSET;
        }
    }
    return S_OK;
}
HRESULT InputBoxControl::TxGetPropertyBits(DWORD dwMask, DWORD* pdwBits) {
    DWORD bits = TXTBIT_RICHTEXT | TXTBIT_MULTILINE | TXTBIT_WORDWRAP | TXTBIT_USECURRENTBKG | TXTBIT_CLIENTRECTCHANGE;
    if (d2dMode_) {
        bits |= TXTBIT_D2DDWRITE | TXTBIT_D2DSUBPIXELLINES;
    }

    *pdwBits = bits & dwMask;
    return S_OK;
}
HRESULT InputBoxControl::TxNotify(DWORD iNotify, void* pv) {
    switch (iNotify) {
    case EN_CHANGE: {
        /* CComBSTR str;
        services_->TxGetText(&str);*/
        onTextChanged(L"");
        break;
    }
    case EN_REQUESTRESIZE: {
        auto* rr = reinterpret_cast<REQRESIZE*>(pv);
        CRect windowRect;
        GetClientRect(&windowRect);
        int w = rr->rc.right - rr->rc.left;
        int h = rr->rc.bottom - rr->rc.top;
        //h = MulDiv(h, 96, DPIHelper::GetDpiForWindow(m_hWnd));
        SetWindowPos(0, 0, 0, windowRect.Width(), h, SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOZORDER);
        onResized(windowRect.Width(), h);
        break;
    }
    case EN_SELCHANGE: {
        auto* sc = reinterpret_cast<SELCHANGE*>(pv);
        onSelectionChanged(sc->chrg.cpMin, sc->chrg.cpMax, logFont_);
        break;
    }
    default:
        break;
    }
    return S_OK;
}

HRESULT InputBoxControl::TxGetWindow(HWND* phwnd) {
    *phwnd = m_hWnd;
    return S_OK;
}

HRESULT InputBoxControl::TxDestroyCaret() {
    caretBitmap_ = 0;
    caretCreated_ = false;
    d2dCaretBitmap_.Release();
    return S_OK;
}

HCURSOR InputBoxControl::TxSetCursor2(HCURSOR hcur, BOOL) {
    HCURSOR res = cursor_;
    cursor_ = hcur;
    SetCursor(cursor_);
    return res;
}

HRESULT InputBoxControl::TxGetEditStyle(DWORD dwItem, DWORD* pdwData) {
    if (!pdwData)
        return E_POINTER;
    // dwItem: GETESTYLE_* (см. TextServ.h). Вернём базовые флаги.
    *pdwData = ES_MULTILINE | ES_AUTOVSCROLL | ES_NOHIDESEL | ES_WANTRETURN;
    return S_OK;
}

HRESULT InputBoxControl::TxGetWindowStyles(DWORD* pdwStyle, DWORD* pdwExStyle) {
    if (pdwStyle)
        *pdwStyle = WS_CHILD | WS_TABSTOP | WS_CLIPSIBLINGS;
    if (pdwExStyle)
        *pdwExStyle = 0;
    return S_OK;
}

void InputBoxControl::setHostWindow(HWND wnd) {
    hostWindow_ = wnd;
}

std::unique_ptr<Gdiplus::Bitmap> InputBoxControl::prepareBackground(Gdiplus::Bitmap* source, const Gdiplus::Rect& rc) {
    // Создаем временный bitmap для конвертации
    auto tempBitmap = std::make_unique<Gdiplus::Bitmap>(rc.Width, rc.Height, PixelFormat32bppPARGB);
    Gdiplus::Graphics tempGraphics(tempBitmap.get());
    tempGraphics.Clear(Gdiplus::Color::Transparent);
    // Копируем нужную область
    tempGraphics.DrawImage(source,
        Gdiplus::Rect(0, 0, rc.Width, rc.Height),
        rc.X, rc.Y, rc.Width, rc.Height,
        Gdiplus::UnitPixel);

    return tempBitmap;
}

}
