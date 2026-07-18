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

#include "ScreenCaptureWin.h"

#include <cassert>
#include <deque>

#include <dwmapi.h>

#include "atlheaders.h"
#include "Func/WinUtils.h"
#include "resource.h"
#include "Core/Images/Utils.h"
#include "Core/Logging.h"
#include "ScreenshotHelper.h"
#include "Gui/GuiTools.h"

namespace ScreenCapture {

using namespace Gdiplus;

void ProcessEvents(void)
{
    MSG msg;
    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

void ActivateWindowRepeat(HWND handle, int count)
{
    for (int i = 0; GetForegroundWindow() != handle && i < count; i++)
    {
        BringWindowToTop(handle);
        Sleep(1);
        ProcessEvents();
    }
}

HRGN CloneRegion(HRGN source)
{
    HRGN resultRgn = CreateRectRgn(0, 0, 0, 0);
    CombineRgn(resultRgn, source, resultRgn, RGN_OR);
    return resultRgn;
}

HWND GetTopParent(HWND wnd)
{
    // HWND res;
// while(GetParent(wnd)!=(HWND)0)
    if (GetWindowLong(wnd, GWL_STYLE) & WS_CHILD)
    {
        wnd = ::GetParent(wnd);
    }
    return wnd;
}

std::vector<RECT> monitorsRects;
BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM dwData)
{
    if (lprcMonitor)
    {
        monitorsRects.push_back(*lprcMonitor);
    }
    return TRUE;
}

void average_polyline(std::vector<POINT>& path, std::vector<POINT>& path2, unsigned n);

enum ChannelARGB {
    Blue = 0,
    Green = 1,
    Red = 2,
    Alpha = 3
};

// hack for stupid GDIplus
void transferOneARGBChannelFromOneBitmapToAnother(Bitmap& source, Bitmap& dest, ChannelARGB sourceChannel,
                                                  ChannelARGB destChannel )
{
    Rect r( 0, 0, source.GetWidth(), source.GetHeight() );
    BitmapData bdSrc;
    BitmapData bdDst;
    source.LockBits( &r,  ImageLockModeRead, PixelFormat32bppARGB, &bdSrc);
    dest.LockBits( &r,  ImageLockModeWrite, PixelFormat32bppARGB, &bdDst);
    BYTE* bpSrc = static_cast<BYTE*>(bdSrc.Scan0);
    BYTE* bpDst = static_cast<BYTE*>(bdDst.Scan0);
    bpSrc += sourceChannel;
    bpDst += destChannel;
    for ( int i = r.Height * r.Width; i > 0; i-- )
    {
        *bpDst = *bpSrc;
        if (*bpDst == 0)
        {
            bpDst -= destChannel;
            *bpDst = 0;
            *(bpDst + 1) = 0;
            *(bpDst + 2) = 0;
            bpDst += destChannel;
        }
        bpSrc += 4;
        bpDst += 4;
    }
    source.UnlockBits( &bdSrc );
    dest.UnlockBits( &bdDst );
}

void average_polyline(std::vector<POINT>& path, std::vector<POINT>& path2, unsigned n)
{
    if (path.size() > 2)
    {
        std::deque<POINT> tmp;
        unsigned i, j;
        for (j = 0; j < path.size(); j++)
        {
            tmp.push_back(path[j]);
        }
        for (i = 0; i < n; i++)
        {
            path2.clear();
            for (j = 0; j < tmp.size(); j++)
            {
                POINT p = {tmp[j].x, tmp[j].y};
                int d = 1;
                if (j)
                {
                    p.x += tmp[j - 1].x;
                    p.y += tmp[j - 1].y;
                    ++d;
                }
                if (j + 1 < tmp.size())
                {
                    p.x += tmp[j + 1].x;
                    p.y += tmp[j + 1].y;
                    ++d;
                }
                POINT newP = {p.x / d, p.y / d};
                path2.push_back(newP);
            }
            if (i < n - 1)
            {
                tmp.clear();
                for (j = 0; j < path2.size(); j++)
                {
                    tmp.push_back(path2[j]);
                }
            }
        }
    }
}

CRectRegion::CRectRegion()
{
}

CRectRegion::~CRectRegion()
{
}

CRectRegion::CRectRegion(int x, int y, int width, int height)
{
    if (!m_ScreenRegion.IsNull())
        m_ScreenRegion.DeleteObject();
    m_ScreenRegion.CreateRectRgn(x, y, x + width, y + height);
}

CRectRegion::CRectRegion(HRGN region)
{
    if (!m_ScreenRegion.IsNull())
        m_ScreenRegion.DeleteObject();
    m_ScreenRegion = region;
}

bool CRectRegion::IsEmpty()
{
    CRect rect(0, 0, 0, 0);
    m_ScreenRegion.GetRgnBox(&rect);
    return rect.IsRectEmpty() != 0;
}

std::shared_ptr<Gdiplus::Bitmap> CRectRegion::GetImage(HDC src)
{
    RECT regionBoundingRect;
    CRgn screenRegion = CloneRegion(m_ScreenRegion);
    RECT screenBounds;
    GuiTools::GetScreenBounds(screenBounds);
    CRgn FullScreenRgn;
    FullScreenRgn.CreateRectRgnIndirect(&screenBounds);
    if ( !m_bFromScreen ) {
        FullScreenRgn.OffsetRgn(-screenBounds.left, -screenBounds.top);
    }
    else screenRegion.OffsetRgn(screenBounds.left, screenBounds.top);
    screenRegion.CombineRgn(FullScreenRgn, RGN_AND);
    screenRegion.GetRgnBox(&regionBoundingRect);
    int bmWidth = regionBoundingRect.right - regionBoundingRect.left;
    int bmHeight = regionBoundingRect.bottom  - regionBoundingRect.top;

    if (bmWidth <= 0 || bmHeight <=0) {
        LOG(WARNING) << "Cannot make screenshot of empty region";
        return {};
    }

    CBitmap tempBm; // Temporary bitmap and device context
    CDC tempDC;    //  which were added to avoid artifacts with BitBlt
    HDC dc = GetDC( 0 );
    tempBm.CreateCompatibleBitmap( dc, bmWidth, bmHeight );
    tempDC.CreateCompatibleDC( dc );
    HBITMAP oldBm = tempDC.SelectBitmap( tempBm );
    ReleaseDC( 0, dc );

    screenRegion.OffsetRgn( -regionBoundingRect.left, -regionBoundingRect.top);

    if (!::BitBlt(tempDC, 0, 0, bmWidth,
                  bmHeight, src, regionBoundingRect.left, regionBoundingRect.top, SRCCOPY | CAPTUREBLT)) {
        return {};
    }
    drawCursor(tempDC, regionBoundingRect.left, regionBoundingRect.top);
    auto resultBm = std::make_shared<Bitmap>(bmWidth, bmHeight, PixelFormat32bppARGB);
    Graphics gr( resultBm.get() );

    Bitmap srcBm( tempBm, 0);
    gr.SetClip( screenRegion );

    // Each call to the Graphics::GetHDC should be paired with a call to the Graphics::ReleaseHDC
    gr.DrawImage( &srcBm, 0, 0);
    gr.Flush();
    tempDC.SelectBitmap( oldBm );
    return resultBm;
}

CWindowHandlesRegion::CWindowHandlesRegion()
{
    init();
}

CWindowHandlesRegion::CWindowHandlesRegion(HWND wnd)
{
    init();
    CWindowHandlesRegionItem newItem;
    newItem.Include = true;
    newItem.wnd = wnd;
    m_hWnds.push_back(newItem);
}

void CWindowHandlesRegion::init() {
    topWindow = nullptr;
    m_WindowHidingDelay = 0;
    m_ClearBackground = false;
    m_RemoveCorners = true;
    m_PreserveShadow = true;
}

void CWindowHandlesRegion::setWindowHidingDelay(int delay)
{
    m_WindowHidingDelay = delay;
}

void CWindowHandlesRegion::setWindowCapturingFlags(WindowCapturingFlags flags)
{
    m_ClearBackground = flags.RemoveBackground;
    m_RemoveCorners = flags.RemoveCorners;
    m_PreserveShadow = flags.AddShadow;
}

COLORREF bgColor;

bool AreImagesEqual(Bitmap* b1, Bitmap* b2)
{
    bool result = true;
    int width = b1->GetWidth();
    int height = b1->GetHeight();
    Rect rect(0, 0, width, height);
    BitmapData b1Data;
    b1->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &b1Data);
    BitmapData b2Data;
    b2->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &b2Data);
    assert(sizeof(unsigned long) == 4);
    unsigned long* pImage1 = static_cast<unsigned long*>(b1Data.Scan0);
    unsigned long* pImage2 = static_cast<unsigned long*>(b2Data.Scan0);
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            if (*(pImage1++) != *(pImage2++))
            {
                result = false;
                break;
            }
        }
    }
    b1->UnlockBits(&b1Data);
    b2->UnlockBits(&b2Data);
    return result;
}

std::unique_ptr<Gdiplus::Bitmap> ComputeOriginal(Bitmap* whiteBGImage, Bitmap* blackBGImage)
{
    assert(whiteBGImage);
    assert(blackBGImage);

    int width = whiteBGImage->GetWidth();
    int height = whiteBGImage->GetHeight();
    std::unique_ptr<Bitmap> resultImage = std::make_unique<Bitmap>(width, height, PixelFormat32bppARGB);
    Gdiplus::Rect rect(0, 0, blackBGImage->GetWidth(), blackBGImage->GetHeight());
    // Access the image data directly for faster image processing
    BitmapData blackImageData;
    blackBGImage->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &blackImageData);
    BitmapData whiteImageData;
    whiteBGImage->LockBits(&rect, ImageLockModeRead, PixelFormat32bppARGB, &whiteImageData);
    BitmapData resultImageData;
    resultImage->LockBits(&rect, ImageLockModeWrite, PixelFormat32bppARGB, &resultImageData);
    void* pBlackImage = blackImageData.Scan0;
    void* pWhiteImage = whiteImageData.Scan0;
    void* pResultImage = resultImageData.Scan0;
    unsigned char* blackBGImageRGB = ( unsigned char*)pBlackImage /*new unsigned char[bytes]*/;
    unsigned char* whiteBGImageRGB = ( unsigned char*)pWhiteImage /*new unsigned char[bytes]*/;
    unsigned char* resultImageRGB = ( unsigned char*)pResultImage /* new unsigned char[bytes]*/;
    size_t offset = 0;
    int b0, g0, r0, b1, g1, r1, alphaR, alphaG, alphaB, resultR, resultG, resultB;
    for (int y = 0; y < height; y++)
    {
        for (int x = 0; x < width; x++)
        {
            // ARGB is in fact BGRA (little endian)
            b0 = blackBGImageRGB[offset + 0];
            g0 = blackBGImageRGB[offset + 1];
            r0 = blackBGImageRGB[offset + 2];
            b1 = whiteBGImageRGB[offset + 0];
            g1 = whiteBGImageRGB[offset + 1];
            r1 = whiteBGImageRGB[offset + 2];
            alphaR = r0 - r1 + 255;
            alphaG = g0 - g1 + 255;
            alphaB = b0 - b1 + 255;
            if (alphaG != 0)
            {
                resultR = r0 * 255 / alphaG;
                resultG = g0 * 255 / alphaG;
                resultB = b0 * 255 / alphaG;
            }
            else
            {
                // Could be any color since it is fully transparent.
                resultR = 255;
                resultG = 255;
                resultB = 255;
            }
            resultImageRGB[offset + 3] = static_cast<byte>(alphaR);
            resultImageRGB[offset + 2] = static_cast<byte>(resultR);
            resultImageRGB[offset + 1] = static_cast<byte>(resultG);
            resultImageRGB[offset + 0] = static_cast<byte>(resultB);
            offset += 4;
        }
    }
    blackBGImage->UnlockBits(&blackImageData);
    whiteBGImage->UnlockBits(&whiteImageData);
    // whiteBGImage2->UnlockBits(&whiteImageData2);
    resultImage->UnlockBits(&resultImageData);
    return resultImage;
}

void OnEraseBgrnd(HWND hWnd)
{
    PAINTSTRUCT ps;
    HDC dc = BeginPaint(hWnd, &ps);
    HBRUSH br = CreateSolidBrush(bgColor);
    RECT rc;
    GetClientRect(hWnd, &rc);
    FillRect(dc, &rc, br);
    EndPaint(hWnd, &ps);
    DeleteObject(br);
}

LRESULT CALLBACK WndProcedure(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
    switch (Msg)
    {
    // If the user wants to close the application
    case WM_DESTROY:
        // then close it
        // PostQuitMessage(WM_QUIT);
        break;

    case WM_PAINT:
        OnEraseBgrnd(hWnd);
        return 1;
        break;

    default:
        // Process the left-over messages
        return DefWindowProc(hWnd, Msg, wParam, lParam);
    }
    // If something was not done, let it go
    return 0;
}

HWND CreateDummyWindow(const RECT& rc)
{
    HWND hWnd;
    WNDCLASSEX WndClsEx;
    LPCTSTR clsName = _T("DummyWindow");
    // Create the application window
    WndClsEx.cbSize        = sizeof(WNDCLASSEX);
    WndClsEx.style         = CS_HREDRAW | CS_VREDRAW;
    WndClsEx.lpfnWndProc   = WndProcedure;
    WndClsEx.cbClsExtra    = 0;
    WndClsEx.cbWndExtra    = 0;
    WndClsEx.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
    WndClsEx.hCursor       = LoadCursor(NULL, IDC_ARROW);
    WndClsEx.hbrBackground = static_cast<HBRUSH>(GetStockObject(WHITE_BRUSH));
    WndClsEx.lpszMenuName  = NULL;
    WndClsEx.lpszClassName = clsName;
    WndClsEx.hInstance     = GetModuleHandle(0);
    WndClsEx.hIconSm       = LoadIcon(NULL, IDI_APPLICATION);
    // Register the application
    RegisterClassEx(&WndClsEx);
    // Create the window object
    hWnd = CreateWindowEx( WS_EX_TOPMOST, clsName,
                           _T(""), WS_POPUP, rc.left, rc.top,
                           rc.right - rc.left,
                           rc.bottom - rc.top,
                           NULL,
                           NULL,
                           GetModuleHandle(0),
                           NULL);
    if ( !hWnd )
        return 0;
    return hWnd;
}

BOOL BringWindowToForeground(HWND hWnd)
{
    DWORD dwTimeout;
    //Actuallly changing system parameters doesn't work on modern Windows systems
    ::SystemParametersInfo(SPI_GETFOREGROUNDLOCKTIMEOUT, 0, &dwTimeout, 0);
    ::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, 0, 0);
    BOOL bNeedTopmost = !(::GetWindowLong(hWnd, GWL_EXSTYLE) & WS_EX_TOPMOST);
    if (bNeedTopmost)
        ::SetWindowPos(hWnd, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    HWND hCurWnd;
    for (int i = 0; (i < 10) && ((hCurWnd = ::GetForegroundWindow()) != hWnd); i++)
    {
        int nMyTID  = ::GetCurrentThreadId();
        int nCurTID = ::GetWindowThreadProcessId(hCurWnd, 0);
        ::AttachThreadInput(nMyTID, nCurTID, TRUE);
        ::SetForegroundWindow(hWnd);
        ::AttachThreadInput(nMyTID, nCurTID, FALSE);
        Sleep(20);
    }
    if (bNeedTopmost)
        ::SetWindowPos(hWnd, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    ::SystemParametersInfo(SPI_SETFOREGROUNDLOCKTIMEOUT, 0, &dwTimeout, 0);
    return TRUE;
}

enum Corner { TopLeft, TopRight, BottomLeft, BottomRight };

/**
 * Removes a pixel from the clipping region of the given graphics object, if
 * the bitmap is red at the coordinates of the pixel, or if it is null.
 * @param bmp The bitmap with the form corners masked in red
*/
void RemoveCornerPixel(Bitmap* bmp, Graphics* g, int y, int x)
{
    bool remove;
    if (bmp != 0)
    {
        Color color;
        bmp->GetPixel(x, y, &color);
        // detect a shade of red (the color is darker because of the window's shadow)
        remove = (color.GetR() > 0 && color.GetG() == 0 && color.GetB() == 0);
    }
    else
    {
        remove = true;
    }
    if (remove)
    {
        Region region (Rect(x, y, 1, 1));
        g->SetClip(&region, CombineModeExclude);
    }
}


/**
 * Removes a corner from the clipping region of the given graphics object.
 * @param bmp The bitmap with the form corners masked in red
*/
void RemoveCorner(Bitmap* bmp, Graphics* g, int minx, int miny, int maxx, Corner corner)
{
    int s1[5] = { 5, 3, 2, 1, 1 };
    int s2[5] = { 1, 1, 2, 3, 5 };
    int* shape;
    if (corner == TopLeft || corner == TopRight)
    {
        shape = s1;
    }
    else
    {
        shape = s2;
    }
    int maxy = miny + 5;
    if (corner == TopLeft || corner == BottomLeft)
    {
        for (int y = miny; y < maxy; y++)
        {
            for (int x = minx; x < minx + shape[y - miny]; x++)
            {
                RemoveCornerPixel(bmp, g, y, x);
            }
        }
    }
    else
    {
        for (int y = miny; y < maxy; y++)
        {
            for (int x = maxx - 1; x >= maxx - shape[y - miny]; x--)
            {
                RemoveCornerPixel(bmp, g, y, x);
            }
        }
    }
}

bool RemoveCorners(Bitmap* windowImage, Bitmap* redBGImage, Bitmap** outResult)
{
    const int cornerSize = 5;
    if (windowImage->GetWidth() > cornerSize * 2 && windowImage->GetHeight() > cornerSize * 2)
    {
        Bitmap* result = new Bitmap(windowImage->GetWidth(),  windowImage->GetHeight(), PixelFormat32bppARGB);
        Graphics g(result);
        g.Clear(Color::Transparent);
        // Remove the transparent pixels in the four corners
        RemoveCorner(redBGImage, &g, 0, 0, cornerSize, TopLeft);
        RemoveCorner(redBGImage, &g, windowImage->GetWidth() - cornerSize, 0, windowImage->GetWidth(), TopRight);
        RemoveCorner(redBGImage, &g, 0, windowImage->GetHeight() - cornerSize, cornerSize, BottomLeft);
        RemoveCorner(redBGImage, &g, windowImage->GetWidth() - cornerSize,
                     windowImage->GetHeight() - cornerSize, windowImage->GetWidth(), BottomRight);
        g.DrawImage(windowImage, 0, 0);
        *outResult = result;
        return true;
    }
    return false;
}

void DrawShadow(Graphics& g, Bitmap* shadowBitmap, int x, int y, int width, int height)
{
    TextureBrush brush(shadowBitmap);
    Bitmap bmpTemp(width, height, PixelFormat32bppARGB);
    Graphics gTemp(&bmpTemp);
    {
        // Draw on a temp bitmap with (0,0) offset, because the texture starts at (0,0)
        gTemp.FillRectangle(&brush, 0, 0, width, height);
        g.DrawImage(&bmpTemp, x, y);
    }
}

bool AddBorderShadow(Bitmap* input, bool roundedShadowCorners, Bitmap** out)
{
    const int width = static_cast<int>(input->GetWidth());
    const int height = static_cast<int>(input->GetHeight());
    Color c;
    input->GetPixel(0, 0, &c);
    bool topLeftRound = c.GetAlpha() < 20;
    input->GetPixel(width - 1, 0, &c);
    bool topRightRound = c.GetAlpha() < 20;
    input->GetPixel(0, height - 1, &c);
    //bool bottomLeftRound = c.GetAlpha() < 20;
    input->GetPixel(width - 1, height - 1, &c);
    //bool bottomRightRound = c.GetAlpha() < 20;
    std::unique_ptr<Bitmap> leftShadow = ImageUtils::BitmapFromResource(GetModuleHandle(0), MAKEINTRESOURCE(IDR_leftShadow), _T("PNG")); // Resources.leftShadow;
    std::unique_ptr<Bitmap> rightShadow = ImageUtils::BitmapFromResource(GetModuleHandle(0), MAKEINTRESOURCE(IDR_rightShadow), _T("PNG"));
    std::unique_ptr<Bitmap> topShadow = ImageUtils::BitmapFromResource(GetModuleHandle(0), MAKEINTRESOURCE(IDR_topShadow), _T("PNG"));
    std::unique_ptr<Bitmap> bottomShadow = ImageUtils::BitmapFromResource(GetModuleHandle(0), MAKEINTRESOURCE(IDR_bottomShadow), _T("PNG"));
    std::unique_ptr<Bitmap> topLeftShadow =
        ImageUtils::BitmapFromResource(GetModuleHandle(0), MAKEINTRESOURCE(
                             topLeftRound ? IDR_topLeftShadow : IDR_topLeftShadowSquare), _T("PNG"));
    std::unique_ptr<Bitmap> topRightShadow =
        ImageUtils::BitmapFromResource(GetModuleHandle(0), MAKEINTRESOURCE(
                             topRightRound ? IDR_topRightShadow : IDR_topRightShadowSquare), _T("PNG"));
    std::unique_ptr<Bitmap> bottomLeftShadow = ImageUtils::BitmapFromResource(GetModuleHandle(0), MAKEINTRESOURCE(IDR_bottomLeftShadow), _T("PNG"));
    std::unique_ptr<Bitmap> bottomRightShadow = ImageUtils::BitmapFromResource(GetModuleHandle(0), MAKEINTRESOURCE(IDR_bottomRightShadow), _T("PNG"));
    int leftMargin = leftShadow->GetWidth();
    int rightMargin = rightShadow->GetWidth();
    int topMargin = topShadow->GetHeight();
    int bottomMargin = bottomShadow->GetHeight();
    int resultWidth = leftMargin + width + rightMargin;
    int resultHeight = topMargin + height + bottomMargin;
    if (resultHeight - topRightShadow->GetHeight() - bottomRightShadow->GetHeight() <= 0
        || resultWidth - bottomLeftShadow->GetWidth() - bottomRightShadow->GetWidth() <= 0)
    {
        *out = input->Clone(0, 0, input->GetWidth(), input->GetHeight(), PixelFormat32bppARGB);
    }
    else
    {
        auto* bmpResult = new Bitmap(resultWidth, resultHeight, PixelFormat32bppARGB);
        Graphics g(bmpResult);
        g.Clear(Gdiplus::Color::Transparent);
        g.DrawImage(topLeftShadow.get(), 0, 0);
        g.DrawImage(topRightShadow.get(), resultWidth - topRightShadow->GetWidth(), 0);
        g.DrawImage(bottomLeftShadow.get(), 0, resultHeight - bottomLeftShadow->GetHeight());
        g.DrawImage(bottomRightShadow.get(),
                    (float)resultWidth - bottomRightShadow->GetWidth(), (float)resultHeight -
                    bottomRightShadow->GetHeight());
        DrawShadow(g, leftShadow.get(), 0, topLeftShadow->GetHeight(),
                   leftShadow->GetWidth(), resultHeight - topLeftShadow->GetHeight() - bottomLeftShadow->GetHeight());
        DrawShadow(g, rightShadow.get(), resultWidth - rightShadow->GetWidth(), topRightShadow->GetHeight(),
                   rightShadow->GetWidth(), resultHeight - topRightShadow->GetHeight() - bottomRightShadow->GetHeight());
        DrawShadow( g, topShadow.get(), topLeftShadow->GetWidth(), 0,
                    resultWidth - topLeftShadow->GetWidth() - topRightShadow->GetWidth(), topShadow->GetHeight());
        DrawShadow(g, bottomShadow.get(), bottomLeftShadow->GetWidth(), resultHeight - bottomShadow->GetHeight(),
                   resultWidth - bottomLeftShadow->GetWidth() - bottomRightShadow->GetWidth(), bottomShadow->GetHeight());
        g.DrawImage(input, leftMargin, topMargin);
        *out = bmpResult;
    }
    return *out != 0;
}

bool CheckRect(const RECT& rect, COLORREF color)
{
    HDC screenDC = ::GetDC(0);
    COLORREF pixel = GetPixel(screenDC, (rect.right - rect.left) / 2, (rect.bottom - rect.bottom / 2) - 1);
    bool result = pixel == color;
    ReleaseDC(0, screenDC);
    return result;
}

std::shared_ptr<Bitmap> CWindowHandlesRegion::CaptureWithTransparencyUsingDWM()
{
    std::shared_ptr<Bitmap> resultBm;
    std::shared_ptr<Bitmap> redBgBitmap;
    bool move = false;
    HWND target = topWindow;
    TCHAR Buffer[MAX_PATH];
    GetClassName(target, Buffer, sizeof(Buffer) / sizeof(TCHAR));
    if (lstrcmpi(Buffer, _T("Shell_TrayWnd")))
    {
        BringWindowToForeground(target);
        move = true;
    }
    SetForegroundWindow(target);
    // CRgn newRegion=GetWindowVisibleRegion(target);
    CRect actualWindowRect;
    ScreenshotHelper::getActualWindowRect(target, &actualWindowRect, false);
//    bool IsSimpleRectWindow = newRegion.GetRgnBox(&windowRect)==SIMPLEREGION;
    bgColor = RGB(255, 255, 255);
    HWND wnd = 0;
    CScreenCaptureEngine eng;
    CRectRegion reg(m_ScreenRegion);
    std::shared_ptr<Bitmap> bm1, bm2, bm3;
    HTHUMBNAIL thumb = 0;
    if (m_ClearBackground || m_RemoveCorners || m_PreserveShadow)
    {
        wnd = CreateDummyWindow(actualWindowRect);
    }
    if (wnd)
    {
        int i = 0;
        while (!CheckRect(actualWindowRect, bgColor))
        {
            WinUtils::TimerWait(50);
            if (i++ > 10) break;
        }
        if (DwmRegisterThumbnail(wnd, m_hWnds[0].wnd, &thumb) != S_OK)
        {
            DestroyWindow(wnd);
            return 0;
        }
        if (m_ClearBackground)
            ShowWindow(wnd, SW_SHOWNOACTIVATE);
        SIZE size;
        if (DwmQueryThumbnailSourceSize(thumb, &size) != S_OK)
            return 0;
        DWM_THUMBNAIL_PROPERTIES props;
        props.dwFlags = DWM_TNP_VISIBLE | DWM_TNP_RECTDESTINATION | DWM_TNP_OPACITY;
        props.fVisible = true;
        props.opacity = 255;
        RECT rcDest = {0, 0, size.cx, size.cy};
        props.rcDestination = rcDest;
        DwmUpdateThumbnailProperties(thumb, &props);
        ProcessEvents();
        eng.captureRegion(&reg);
        bm1 = eng.capturedBitmap();
    }
    if (m_ClearBackground)
    {
        bgColor = RGB(0, 0, 0);
        ::InvalidateRect(wnd, NULL, true);
        ProcessEvents();
        DwmFlush();
        eng.captureRegion(&reg);
        bm2 = eng.capturedBitmap();
    }
    if (m_ClearBackground || m_RemoveCorners)
    {
        ShowWindow(wnd, SW_SHOWNOACTIVATE);
        bgColor = RGB(255, 255, 255);
        ::InvalidateRect(wnd, NULL, true);
        ProcessEvents();
        eng.captureRegion(&reg);
        bm3 = eng.capturedBitmap();
    }
    std::shared_ptr<Bitmap> preResult = 0;
    if (m_ClearBackground)
    {
        if (AreImagesEqual(bm1.get(), bm3.get()))
        {
            std::shared_ptr<Bitmap> original = ComputeOriginal(bm1.get(), bm2.get());
            if (original)
            {
                preResult = original;
            }
            else
            {
                assert(bm3);
                preResult = bm3;
                bm3.reset();
            }
        }
        else
        {
            assert(bm3);
            preResult = bm3;
            bm3.reset();
        }
    }
    if ((m_RemoveCorners || (m_PreserveShadow)) /*&& !preResult*/ && !ScreenshotHelper::isWindowMaximized(target))   // We don't have to clear window corners if we already have capture with aplha-channel
    {
        bgColor = RGB(255, 0, 0);
        ShowWindow(wnd, SW_SHOWNOACTIVATE);
        ::InvalidateRect(wnd, NULL, false);
        ProcessEvents();
        ActivateWindowRepeat(target, 250);
        eng.captureRegion(&reg);
        redBgBitmap = eng.capturedBitmap();
        Bitmap* ress = nullptr;
        if (RemoveCorners(preResult ? preResult.get() : bm1.get(), redBgBitmap.get(), &ress))
        {
            assert(ress);
            preResult.reset(ress);
        }
    }
    if (preResult && m_PreserveShadow && !ScreenshotHelper::isWindowMaximized(target))
    {
        Bitmap* shadowed = nullptr;
        AddBorderShadow(preResult.get(), true, &shadowed);
        preResult.reset(shadowed);
    }
    resultBm = preResult;
    if (target && move)
        ::SetWindowPos(target, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    DestroyWindow(wnd);
    DwmUnregisterThumbnail(thumb);
    return resultBm;
}

// TODO : fix vista maximized window capturing
std::shared_ptr<Gdiplus::Bitmap> CWindowHandlesRegion::GetImage(HDC src)
{
    if (m_hWnds.empty()) {
        return {};
    }
    RECT captureRect = {0, 0, 0, 0};
    if (!m_ScreenRegion.IsNull())
        m_ScreenRegion.DeleteObject();
    m_ScreenRegion.CreateRectRgnIndirect(&captureRect);
    for (auto & m_hWnd : m_hWnds)
    {
        CRgn newRegion = ScreenshotHelper::getWindowVisibleRegion(m_hWnd.wnd);
        m_ScreenRegion.CombineRgn(newRegion, m_hWnd.Include ? RGN_OR : RGN_DIFF);
    }
    bool move = false;
    bool parentIsInList = false;
    if (m_bFromScreen)
    {
        topWindow = GetTopParent(m_hWnds[0].wnd);
        if (topWindow == m_hWnds[0].wnd)
            parentIsInList = true;
        for (size_t i = 1; i < m_hWnds.size(); i++)
        {
            HWND curTopWindow = GetTopParent(m_hWnds[i].wnd);
            if (curTopWindow == m_hWnds[i].wnd)
                parentIsInList = true;
            if (topWindow != curTopWindow)
            {
                topWindow = 0;
            }
        }
        if (topWindow)
        {
            TCHAR Buffer[MAX_PATH];
            GetClassName(topWindow, Buffer, std::size(Buffer));
            if (lstrcmpi(Buffer, _T("Shell_TrayWnd")))
            {
                move = true;
                ::SetWindowPos(topWindow, HWND_TOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                WinUtils::TimerWait(m_WindowHidingDelay);
            }
        }
    }
    CRect scr;
    GuiTools::GetScreenBounds(scr);
    m_ScreenRegion.OffsetRgn(-scr.left, -scr.top);
    std::shared_ptr<Bitmap> resultBm;
    if (m_bFromScreen && parentIsInList /*&& GetParent(topWindow)==HWND_DESKTOP */ &&  IsWindowsVistaOrGreater() &&
        IsCompositionActive() && topWindow && !(GetWindowLong(topWindow, GWL_STYLE) & WS_CHILD)
        && (m_ClearBackground || m_RemoveCorners || m_PreserveShadow))
    {
        resultBm = CaptureWithTransparencyUsingDWM();
    }

    if (!resultBm)
    {
        resultBm = CRectRegion::GetImage(src);
        if (topWindow && move)
            ::SetWindowPos(topWindow, HWND_NOTOPMOST, 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
    }
    return resultBm;
}

CWindowHandlesRegion::~CWindowHandlesRegion()
{
}

std::vector<ScreenCapture::CWindowHandlesRegion::CWindowHandlesRegionItem>::const_iterator CWindowHandlesRegion::cbegin() const {
    return m_hWnds.cbegin();
}

std::vector<ScreenCapture::CWindowHandlesRegion::CWindowHandlesRegionItem>::const_iterator CWindowHandlesRegion::cend() const {
    return m_hWnds.cend();
}

std::vector<ScreenCapture::CWindowHandlesRegion::CWindowHandlesRegionItem>::size_type CWindowHandlesRegion::size() const {
    return m_hWnds.size();
}

void CWindowHandlesRegion::AddWindow(HWND wnd, bool Include)
{
    CWindowHandlesRegionItem newItem{};
    newItem.wnd = wnd;
    newItem.Include = Include;
    RemoveWindow(wnd);
    m_hWnds.push_back(newItem);
}

void CWindowHandlesRegion::RemoveWindow(HWND wnd)
{
    for (size_t i = 0; i < m_hWnds.size(); i++)
    {
        if (m_hWnds[i].wnd == wnd)
        {
            m_hWnds.erase(m_hWnds.begin() + i);
            return;
        }
    }
}

bool CWindowHandlesRegion::IsEmpty()
{
    return m_hWnds.empty();
}

void CWindowHandlesRegion::Clear()
{
    m_hWnds.clear();
}


CScreenCaptureEngine::CScreenCaptureEngine()
{
    m_captureDelay = 0;
    m_source = 0;
    monitorMode_ = kAllMonitors;
    monitor_ = NULL;
}

bool CScreenCaptureEngine::captureScreen(bool drawCursor)
{
    CRect screenBounds;
    GuiTools::GetScreenBounds(screenBounds);

    CRect captureRect;

    if (monitorMode_ != kAllMonitors) {
        MONITORINFO mi;
        memset(&mi, 0, sizeof(mi));
        mi.cbSize = sizeof(mi);
        if (!GetMonitorInfo(monitor_, &mi)) {
            LOG(ERROR) << "Unable get info about monitor";
            return false;
        }
        captureRect = mi.rcMonitor;
        captureRect.OffsetRect(-screenBounds.left, -screenBounds.top);
    } else {
        captureRect = screenBounds;
        captureRect.OffsetRect(-screenBounds.left, -screenBounds.top);
    }

    CRectRegion capturingRegion(captureRect.left, captureRect.top, captureRect.Width(), captureRect.Height());
    capturingRegion.setDrawCursor(drawCursor);
    return captureRegion(&capturingRegion);
}

void CScreenCaptureEngine::setDelay(int msec)
{
    m_captureDelay = msec;
}

void CScreenCaptureEngine::setMonitorMode(MonitorMode monitorMode, HMONITOR monitor) {
    monitorMode_ = monitorMode;
    monitor_ = monitor;
}

std::shared_ptr<Gdiplus::Bitmap> CScreenCaptureEngine::capturedBitmap() const
{
    return m_capturedBitmap;
}

void CScreenCaptureEngine::setSource(HBITMAP src)
{
    m_source = src;
}

bool CScreenCaptureEngine::captureRegion(CScreenshotRegion* region)
{
//    delete m_capturedBitmap;
    //m_capturedBitmap = NULL;
    HDC srcDC;
    HDC screenDC = ::GetDC(0);
    HGDIOBJ oldBm = 0;
    if (m_source)
    {
        HDC sourceDC = CreateCompatibleDC(screenDC);
        srcDC = sourceDC;
        oldBm = SelectObject(srcDC, m_source);
    }
    else
    {
        srcDC = screenDC;
        if (m_captureDelay)
        {
            WinUtils::TimerWait(m_captureDelay);
        }
    }
    region->PrepareShooting(!(bool)(m_source != 0));
    std::shared_ptr<Gdiplus::Bitmap> capturedBitmap;
    m_capturedBitmap = region->GetImage(srcDC);
    region->AfterShooting();
    if (m_source)
    {
        SelectObject(srcDC, oldBm);
        DeleteDC(srcDC);
    }
    ReleaseDC(0, screenDC);
    return !!m_capturedBitmap;
}

//bool CScreenCaptureEngine::capturedBitmapReleased_ = false;
CFreeFormRegion::CFreeFormRegion()
{
}

void CFreeFormRegion::AddPoint(POINT point)
{
    m_curvePoints.push_back(point);
}

void CFreeFormRegion::Clear()
{
    m_curvePoints.clear();
}

bool CFreeFormRegion::IsEmpty() {
    if (m_curvePoints.empty()) {
        return true;
    }

    GraphicsPath grPath;
    std::vector<Point> points;
    std::vector<POINT> curveAvgPoints;
    average_polyline(m_curvePoints, curveAvgPoints, 29);
    points.reserve(curveAvgPoints.size());

    for (const auto& [x, y] : curveAvgPoints) {
        points.emplace_back(x, y);
    }

    if (points.empty()) {
        return true;
    }

    grPath.AddCurve(&points[0], static_cast<INT>(points.size()));
    Rect grPathRect;
    grPath.GetBounds(&grPathRect);
    int bmWidth = grPathRect.GetRight() - grPathRect.GetLeft();
    int bmHeight = grPathRect.GetBottom() - grPathRect.GetTop();
    return !(bmWidth * bmHeight);
}

std::shared_ptr<Gdiplus::Bitmap> CFreeFormRegion::GetImage(HDC src) {
    GraphicsPath grPath;
    std::vector<Point> points;
    std::vector<POINT> curveAvgPoints;
    average_polyline(m_curvePoints, curveAvgPoints, 29);
    points.reserve(curveAvgPoints.size());

    for (const auto& [x, y] : curveAvgPoints) {
        points.emplace_back(x, y);
    }

    grPath.AddCurve(&points[0], static_cast<INT>(points.size()));
    Rect grPathRect;
    grPath.GetBounds(&grPathRect);
    int bmWidth = grPathRect.GetRight() - grPathRect.GetLeft();
    int bmHeight = grPathRect.GetBottom() - grPathRect.GetTop();
    CDC mem2;
    mem2.CreateCompatibleDC(src);
    CBitmap bm2;
    bm2.CreateCompatibleBitmap(src, bmWidth, bmHeight);
    HBITMAP oldBm = mem2.SelectBitmap(bm2);
    ::BitBlt(mem2, 0, 0, bmWidth, bmHeight, src, grPathRect.GetLeft(), grPathRect.GetTop(), SRCCOPY | CAPTUREBLT);
    mem2.SelectBitmap(oldBm);
    Matrix matrix(1.0f, 0.0f, 0.0f, 1.0f, REAL(-grPathRect.GetLeft()), REAL(-grPathRect.GetTop()));
    grPath.Transform(&matrix);
    Bitmap b(bm2, 0);
    SolidBrush gdipBrush(Color(255, 0, 0, 0));
    Bitmap alphaBm(bmWidth, bmHeight, PixelFormat32bppARGB);
    Graphics alphaGr(&alphaBm);
    alphaGr.SetPixelOffsetMode(PixelOffsetModeHighQuality);
    alphaGr.SetSmoothingMode(SmoothingModeAntiAlias);
    alphaGr.FillPath(&gdipBrush, &grPath);
    auto finalBitmap = std::make_shared<Bitmap>(bmWidth, bmHeight, PixelFormat32bppARGB);
    Graphics gr(finalBitmap.get());
    gr.SetPixelOffsetMode(PixelOffsetModeHighQuality);
    gr.SetSmoothingMode(SmoothingModeAntiAlias);
    gr.DrawImage(&b, 0, 0);
    SolidBrush gdipBrush2(Color(100, 123, 0, 0));
    Pen pn(Color(255, 40, 255), 1.0f);
    Pen pn2(Color(40, 0, 255), 1.0f);
    transferOneARGBChannelFromOneBitmapToAnother(alphaBm, *finalBitmap, Alpha, Alpha);
    return finalBitmap;
}

CFreeFormRegion::~CFreeFormRegion()
{
}

CActiveWindowRegion::CActiveWindowRegion()
{
}

std::shared_ptr<Gdiplus::Bitmap> CActiveWindowRegion::GetImage(HDC src)
{
    Clear();
    AddWindow(GetForegroundWindow(), true);
    return CWindowHandlesRegion::GetImage(src);
}

void CScreenshotRegion::drawCursor(HDC dc, int offsetX, int offsetY) const {
    if (!drawCursor_) {
        return;
    }

    CURSORINFO cursorInfo {};
    cursorInfo.cbSize = sizeof(cursorInfo);

    if (GetCursorInfo(&cursorInfo)) {
        if (cursorInfo.hCursor && (cursorInfo.flags & CURSOR_SHOWING)) {
            ICONINFO iconInfo {};
            if (GetIconInfo(cursorInfo.hCursor, &iconInfo)) {
                DrawIconEx(dc, cursorInfo.ptScreenPos.x - iconInfo.xHotspot - offsetX,
                    cursorInfo.ptScreenPos.y - iconInfo.yHotspot - offsetY,
                    cursorInfo.hCursor, 0, 0, 0, NULL, DI_NORMAL
                );

                if (iconInfo.hbmColor) {
                    DeleteObject(iconInfo.hbmColor);
                }

                if (iconInfo.hbmMask) {
                    DeleteObject(iconInfo.hbmMask);
                }
            }
        }
    }
}

}
