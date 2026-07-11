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

#include "Utils.h"

#include <cstdint>
#include <cmath>
#include <cassert>
#include <memory>
#include <filesystem>
#include <fstream>

#include <boost/format.hpp>
#include <libbase64.h>
#include <webp/demux.h>
#include <webp/encode.h>
#include "3rdpart/GdiplusH.h"
#include "Core/Logging.h"
#include "Func/WinUtils.h"
#include "Core/Images/ColorQuantizer.h"
#include "Core/Utils/StringUtils.h"
#include "Core/Utils/CoreUtils.h"
#include "Func/IuCommonFunctions.h"
#include "Gui/Dialogs/LogWindow.h"
#include "Core/ServiceLocator.h"
#include "Func/Library.h"
#include "Core/AppParams.h"
#include "ImageLoader.h"
#include "Core/Utils/IOException.h"
#include "3rdpart/FastGaussianBlurTemplate.h"

namespace ImageUtils {

using namespace Gdiplus;

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid)
{
    UINT num = 0;           // number of image encoders
    UINT size = 0;          // size of the image encoder array in bytes

    ImageCodecInfo* pImageCodecInfo = NULL;

    GetImageEncodersSize(&num, &size);
    if (size == 0)
        return -1;  // Failure

    pImageCodecInfo = static_cast<ImageCodecInfo*>(malloc(size));
    if (pImageCodecInfo == NULL)
        return -1;  // Failure

    GetImageEncoders(num, size, pImageCodecInfo);

    for (UINT j = 0; j < num; ++j)
    {
        if ( wcscmp(pImageCodecInfo[j].MimeType, format) == 0 )
        {
            *pClsid = pImageCodecInfo[j].Clsid;
            free(pImageCodecInfo);
            return j;  // Success
        }
    }

    free(pImageCodecInfo);
    return -1;  // Failure
}

std::unique_ptr<Gdiplus::Image> ImageFromResource(HINSTANCE hInstance, LPCTSTR szResName, LPCTSTR szResType) {
    ImageLoader loader;
    auto res = loader.loadFromResource(hInstance, szResName, szResType);
    if (!res) {
        return {};
    }
    return std::unique_ptr<Gdiplus::Image>(res->releaseImage());
}

std::unique_ptr<Gdiplus::Bitmap> BitmapFromResource(HINSTANCE hInstance, LPCTSTR szResName, LPCTSTR szResType)
{
    ImageLoader loader;
    auto res = loader.loadFromResource(hInstance, szResName, szResType);
    if (!res) {
        return {};
    }
    return std::unique_ptr<Gdiplus::Bitmap>(res->releaseBitmap());
}

void PrintRichEdit(HWND hwnd, Gdiplus::Graphics* graphics, Gdiplus::Bitmap* background, Gdiplus::Rect layoutArea) {
    using namespace Gdiplus;
    //Calculate the area to render.
    HDC hdc1 = ::GetDC(hwnd);

    double anInchX = 1440.0 / GetDeviceCaps(hdc1, LOGPIXELSX);
    double anInchY = 1440.0 / GetDeviceCaps(hdc1, LOGPIXELSY);
            ReleaseDC(hwnd,hdc1);

    //double anInch = 1440.0  /  GetDeviceCaps(hdc1, LOGPIXELSX);

    RECT rectLayoutArea;
    rectLayoutArea.top = static_cast<int>(layoutArea.GetTop() * anInchY);
    rectLayoutArea.bottom = static_cast<int>(layoutArea.GetBottom() * anInchY);
    rectLayoutArea.left = static_cast<int>(layoutArea.GetLeft() *anInchX  );
    rectLayoutArea.right = static_cast<int>(layoutArea.GetRight() * anInchX);

    HDC hdc = graphics->GetHDC();
    Gdiplus::Graphics gr2(hdc);
    SolidBrush br(Color(255,255,255));

    // We need to draw background on new HDC, otherwise the text will look ugly
    gr2.DrawImage(background,layoutArea.GetLeft(),layoutArea.GetTop(),layoutArea.GetLeft(), layoutArea.GetTop(), layoutArea.Width, layoutArea.Height,Gdiplus::UnitPixel/*gr2.GetPageUnit()*/);

    FORMATRANGE fmtRange;
    fmtRange.chrg.cpMax = -1;                    //Indicate character from to character to
    fmtRange.chrg.cpMin = 0;
    fmtRange.hdc = hdc;                                //Use the same DC for measuring and rendering
    fmtRange.hdcTarget = hdc;                    //Point at printer hDC
    fmtRange.rc = rectLayoutArea;            //Indicate the area on page to print
    fmtRange.rcPage = rectLayoutArea;    //Indicate size of page

    /*int characterCount = */::SendMessage(hwnd, EM_FORMATRANGE, 1, reinterpret_cast<LPARAM>(&fmtRange));

    //Release the device context handle obtained by a previous call
    graphics->ReleaseHDC(hdc);
}

void DrawRoundedRectangle(Gdiplus::Graphics* gr, Gdiplus::Rect r, int d, Gdiplus::Pen* p, Gdiplus::Brush*br){
    using namespace Gdiplus;
    GraphicsPath gp;
//    d = min(min(d, r.Width),r.Height);
    gp.AddArc(r.X, r.Y, d, d, 180, 90);
    gp.AddArc(max(r.X + r.Width - d,r.X), r.Y, d, d, 270, 90);
    gp.AddArc(max(r.X, r.X + r.Width - d), max(r.Y, r.Y + r.Height - d), d, d, 0, 90);
    gp.AddArc(r.X, max(r.Y, r.Y + r.Height - d), d, d, 90, 90);
    //gp.AddLine(r.X, max(r.Y, r.Y + r.Height - d), r.X, min(r.Y + d/2, r.GetBottom()));

    gp.CloseFigure();
    if ( br ) {
        gr->FillPath(br, &gp);
    }
    if (p) {
        gr->DrawPath(p, &gp);
    }

}

std::unique_ptr<Gdiplus::Bitmap> IconToBitmap(HICON ico)
{
    ICONINFO ii;
    GetIconInfo(ico, &ii);
    BITMAP bmp;
    GetObject(ii.hbmColor, sizeof(bmp), &bmp);

    Gdiplus::Bitmap temp(ii.hbmColor, NULL);
    Gdiplus::BitmapData lockedBitmapData;
    memset(&lockedBitmapData, 0, sizeof(lockedBitmapData));
    Gdiplus::Rect rc(0, 0, temp.GetWidth(), temp.GetHeight());

    Gdiplus::Status st = temp.LockBits(&rc, Gdiplus::ImageLockModeRead, temp.GetPixelFormat(), &lockedBitmapData);

    if (st != Gdiplus::Ok) {
        return nullptr;
    }

    std::unique_ptr<Gdiplus::Bitmap> image = std::make_unique<Gdiplus::Bitmap>(
        lockedBitmapData.Width, lockedBitmapData.Height, lockedBitmapData.Stride,
        PixelFormat32bppARGB, static_cast<BYTE *>(lockedBitmapData.Scan0));

    temp.UnlockBits(&lockedBitmapData);
    return image;
}

void ApplyGaussianBlur(Gdiplus::Bitmap* bm, int x,int y, int w, int h, float radius, Gdiplus::Rect exclude) {
    using namespace Gdiplus;
    Rect rc(x, y, w, h);

    BitmapData dataSource;

    if (bm->LockBits(& rc, ImageLockModeRead|ImageLockModeWrite, PixelFormat32bppARGB, & dataSource) == Ok)
    {
        uint8_t * source = static_cast<uint8_t *>(dataSource.Scan0);
        assert(static_cast<UINT>(h) == dataSource.Height);
        UINT stride;
        if (dataSource.Stride > 0) { stride = dataSource.Stride;
        } else {
            stride = - dataSource.Stride;
        }

        size_t myStride = 4 * (w /*& ~3*/);
        size_t bufSize = myStride * h;
        uint8_t* buf = new uint8_t[bufSize];

        uint8_t *bufCur = buf;
        uint8_t *sourceCur = source;
        uint8_t * temp = new uint8_t[bufSize];
        uint8_t* tempCur = temp;
        for (int i = 0; i < h; i++) {
            memcpy(bufCur, sourceCur, myStride);
            memcpy(tempCur, sourceCur, myStride);
            bufCur += myStride;
            sourceCur += stride;
            tempCur += myStride;
        }

        //std::chrono::steady_clock::time_point begin = std::chrono::steady_clock::now();
        fast_gaussian_blur(temp, buf, w, h, 4, radius, 3, kExtend);

        //std::chrono::steady_clock::time_point end = std::chrono::steady_clock::now();
        //LOG(ERROR) << "Time difference = " << std::chrono::duration_cast<std::chrono::milliseconds>(end - begin).count() << "[ms]" << std::endl;

        bufCur = buf;
        sourceCur = source;
        for (int i = 0; i < h; i++) {
            if (exclude.IsEmptyArea()) {
                memcpy(sourceCur, bufCur, myStride);
            } else {
                for (int j = 0; j < w; j++) {
                    if (!exclude.Contains(j, i)) {
                        memcpy(sourceCur + j * 4, bufCur + j * 4, 4);
                    }
                }
            }

            bufCur += myStride;
            sourceCur += stride;
        }
        delete[] temp;
        delete[] buf;

        bm->UnlockBits(&dataSource);
    }
}

void ApplyPixelateEffect(Gdiplus::Bitmap* bm, int xPos, int yPos, int w, int h, int blockSize, Gdiplus::Rect exclude) {
    using namespace Gdiplus;
    Rect rc(xPos, yPos, w, h);

    BitmapData dataSource;

    if (bm->LockBits(&rc, ImageLockModeRead | ImageLockModeWrite, PixelFormat32bppARGB, &dataSource) == Ok)
    {
        uint8_t * source = static_cast<uint8_t *>(dataSource.Scan0);
        assert(static_cast<UINT>(h) == dataSource.Height);
        UINT stride;
        if (dataSource.Stride > 0) {
            stride = dataSource.Stride;
        }
        else {
            stride = -dataSource.Stride;
        }

        int maxX, maxY;
        unsigned int red = 0, green = 0, blue = 0, alpha = 0, numPixels = 0;
        for (int y = 0; y < h; y += blockSize) {
            for (int x = 0; x < w; x += blockSize) {
                numPixels = red = green = blue = alpha = 0;
                maxX = (min)(x + blockSize, w);
                maxY = (min)(y + blockSize, h);
                for (int i = x; i < maxX; i++) {
                    for (int j = y; j < maxY; j++) {
                        size_t offset = j * stride + i*4;
                        alpha += source[offset + 3];
                        red += source[offset+2];
                        green += source[offset + 1];
                        blue += source[offset + 0];
                        numPixels++;
                    }
                }
                uint32_t pixel = (alpha / numPixels << 24) + (red / numPixels << 16) +  (green / numPixels << 8) + blue / numPixels;
                if (exclude.Contains(x, y)) {
                    continue;
                }
                for (int i = x; i < maxX; i++) {
                    for (int j = y; j < maxY; j++) {
                        uint32_t* data = (uint32_t*)(source + j * stride + i*4);
                        *data = pixel;
                    }
                }
            }
        }
        bm->UnlockBits(&dataSource);
    }

}

std::unique_ptr<Gdiplus::Bitmap> LoadImageFromFileWithoutLocking(const WCHAR* fileName, bool* isMultiFrame) {
    using namespace Gdiplus;
    auto img = LoadImageFromFileExtended(fileName);
    if (!img) {
        return nullptr;
    }
    std::unique_ptr<Bitmap> src(img->releaseBitmap());
    if ( !src || src->GetLastStatus() != Ok ) {
        return nullptr;
    }
    if (isMultiFrame) {
        *isMultiFrame = img->isSrcMultiFrame();
    }
    std::unique_ptr<Gdiplus::Bitmap> dst = std::make_unique<Bitmap>(src->GetWidth(), src->GetHeight(), PixelFormat32bppARGB);

    BitmapData srcData;
    BitmapData dstData;
    Rect rc(0, 0, src->GetWidth(), src->GetHeight());

    if (src->LockBits(& rc, ImageLockModeRead, PixelFormat32bppARGB, & srcData) == Ok)
    {
        if ( dst->LockBits(& rc, ImageLockModeWrite, PixelFormat32bppARGB, & dstData) == Ok ) {
            uint8_t * srcBits = static_cast<uint8_t *>(srcData.Scan0);
            uint8_t * dstBits = static_cast<uint8_t *>(dstData.Scan0);
            unsigned int stride;
            if (srcData.Stride > 0) {
                stride = srcData.Stride;
            } else {
                stride = - srcData.Stride;
            }
            memcpy(dstBits, srcBits, src->GetHeight() * stride);

            dst->UnlockBits(&dstData);
        }
        src->UnlockBits(&srcData);
    }
    return dst;
}

Gdiplus::Color StringToColor(const std::string& str) {
    if ( str.empty() ) {
        return Gdiplus::Color();
    }
    try {
        BYTE r = 0, g = 0, b = 0, a = 255;
        if ( str[0] == '#' && str.length() == 7 ) {
            r = static_cast<BYTE>(std::stoul(str.substr(1, 2), nullptr, 16));
            g = static_cast<BYTE>(std::stoul(str.substr(3, 2), nullptr, 16));
            b = static_cast<BYTE>(std::stoul(str.substr(5, 2), nullptr, 16));

            return Gdiplus::Color(r, g, b);
        } else if ( str.substr(0,4) == "rgba" && str.length() >= 14 ) {
            std::vector<std::string> tokens;
            IuStringUtils::Split(str.substr(5, str.length()-6 ),",", tokens,4);
            if ( tokens.size() == 4 ) {
                char * e = nullptr;
                errno = 0;
                a = static_cast<BYTE>(round(std::strtod(tokens[3].c_str(), &e) * 255));
                if (errno != 0 || (e && *e != '\0')) {
                    return Gdiplus::Color();
                }
                r = static_cast<BYTE>(std::stoul(tokens[0]));
                g = static_cast<BYTE>(std::stoul(tokens[1]));
                b = static_cast<BYTE>(std::stoul(tokens[2]));
                return Gdiplus::Color(a, r, g, b);
            }
        } else if ( str.substr(0,3) == "rgb" && str.length() >= 10 ) {
            std::vector<std::string> tokens;
            IuStringUtils::Split(str.substr(4, str.length()-5 ), ",", tokens,3);
            if ( tokens.size() == 3 ) {
                return Gdiplus::Color( static_cast<BYTE>(std::stoul(tokens[0])), static_cast<BYTE>(std::stoul(tokens[1])),
                    static_cast<BYTE>(std::stoul(tokens[2])));
            }
        }
    } catch (const std::invalid_argument&) {
    } catch (const std::out_of_range&) {
    }
    return Gdiplus::Color();
}


struct BGRA_COLOR
{
    BYTE b;
    BYTE g;
    BYTE r;
    BYTE a;
};

// hack for stupid GDIplus
void Gdip_RemoveAlpha(Gdiplus::Bitmap& source, Gdiplus::Color color )
{
    using namespace Gdiplus;
    Rect r( 0, 0, source.GetWidth(),source.GetHeight() );
    BitmapData  bdSrc;
    source.LockBits( &r,  ImageLockModeRead , PixelFormat32bppARGB,&bdSrc);

    BYTE* bpSrc = static_cast<BYTE*>(bdSrc.Scan0);

    //bpSrc += (int)sourceChannel;

    for ( int i = r.Height * r.Width; i > 0; i-- )
    {
        BGRA_COLOR * c = reinterpret_cast<BGRA_COLOR *>(bpSrc);

        if(c->a!=255)
        {
            //c = 255;
            DWORD * d= reinterpret_cast<DWORD*>(bpSrc);
            *d= color.ToCOLORREF();
            c ->a= 255;
        }
        bpSrc += 4;
    }
    source.UnlockBits( &bdSrc );
}

void DrawGradient(Graphics& gr, Rect rect, Color& Color1, Color& Color2)
{
    Bitmap bm(rect.Width, rect.Height, &gr);
    Graphics temp(&bm);
    LinearGradientBrush
        brush(/*TextBounds*/ Rect(0, 0, rect.Width, rect.Height), Color1, Color2, LinearGradientModeVertical);

    temp.FillRectangle(&brush, Rect(0, 0, rect.Width, rect.Height));
    gr.DrawImage(&bm, rect.X, rect.Y);
}

void DrawStrokedText(Graphics& gr, LPCTSTR Text, RectF Bounds, const Font& font, const Color& ColorText, const Color& ColorStroke,
    int HorPos, int VertPos,
    int width)
{
    RectF OriginalTextRect, NewTextRect;
    FontFamily ff;
    font.GetFamily(&ff);
    gr.SetPageUnit(UnitPixel);
    gr.MeasureString(Text, -1, &font, PointF(0, 0), &OriginalTextRect);

    Font NewFont(&ff, 48, font.GetStyle(), UnitPixel);
    gr.MeasureString(Text, -1, &NewFont, RectF(0, 0, 5000, 1600), &NewTextRect);
    OriginalTextRect.Height = OriginalTextRect.Height - OriginalTextRect.Y;
    float newwidth, newheight;
    newheight = OriginalTextRect.Height;
    newwidth = OriginalTextRect.Height / NewTextRect.Height * NewTextRect.Width;
    float k = 2 * width * NewTextRect.Height / OriginalTextRect.Height;
    SolidBrush br(ColorText);
    Bitmap temp(static_cast<int>(NewTextRect.Width), static_cast<int>(NewTextRect.Height), &gr);

    Graphics gr_temp(&temp);
    StringFormat format;
    gr_temp.SetPageUnit(UnitPixel);
    GraphicsPath path;
    gr_temp.SetSmoothingMode(SmoothingModeHighQuality);
    path.AddString(Text, -1, &ff, static_cast<int>(NewFont.GetStyle()), NewFont.GetSize(), Point(0, 0), &format);

    Pen pen(ColorStroke, static_cast<float>(k));
    pen.SetAlignment(PenAlignmentCenter);

    float x, y;
    gr_temp.DrawPath(&pen, &path);
    gr_temp.FillPath(&br, &path);
    gr.SetSmoothingMode(SmoothingModeHighQuality);
    gr.SetInterpolationMode(InterpolationModeHighQualityBicubic);

    if (HorPos == 0)
        x = 2;
    else
        if (HorPos == 1)
            x = (Bounds.Width - newwidth) / 2;
        else
            x = (Bounds.Width - newwidth) - 2;

    if (VertPos == 0)
        y = 2;
    else
        if (VertPos == 1)
            y = (Bounds.Height - newheight) / 2;
        else
            y = (Bounds.Height - newheight) - 2;

    gr.DrawImage(&temp, static_cast<int>(Bounds.GetLeft() + x), static_cast<int>(Bounds.GetTop() + y), static_cast<int>(newwidth), static_cast<int>(newheight));
}

// hack for stupid GDIplus
void ChangeAlphaChannel(Bitmap& source, Bitmap& dest, int sourceChannel, int destChannel)
{
    Rect r(0, 0, source.GetWidth(), source.GetHeight());
    BitmapData bdSrc;
    BitmapData bdDst;
    source.LockBits(&r, ImageLockModeRead, PixelFormat32bppARGB, &bdSrc);
    dest.LockBits(&r, ImageLockModeWrite, PixelFormat32bppARGB, &bdDst);

    BYTE* bpSrc = reinterpret_cast<BYTE*>(bdSrc.Scan0);
    BYTE* bpDst = reinterpret_cast<BYTE*>(bdDst.Scan0);
    bpSrc += static_cast<int>(sourceChannel);
    bpDst += static_cast<int>(destChannel);

    for (int i = r.Height * r.Width; i > 0; i--)
    {
        // if(*bpSrc != 255)
        {
            *bpDst = static_cast<BYTE>((float(255 - *bpSrc) / 255) *  *bpDst);
        }

        /*if(*bpDst == 0)
        {
        bpDst -=(int)destChannel;
        *bpDst = 0;
        *(bpDst+1) = 0;
        *(bpDst+2) = 0;
        bpDst +=(int)destChannel;
        }*/
        bpSrc += 4;
        bpDst += 4;
    }
    source.UnlockBits(&bdSrc);
    dest.UnlockBits(&bdDst);
}

Rect MeasureDisplayString(Graphics& graphics, CString text, RectF boundingRect, Font& font) {
    CharacterRange charRange(0, text.GetLength());
    Region pCharRangeRegions;
    StringFormat strFormat;
    strFormat.SetMeasurableCharacterRanges(1, &charRange);
    graphics.MeasureCharacterRanges(text, text.GetLength(), &font, boundingRect, &strFormat, 1, &pCharRangeRegions);
    Rect rc;
    pCharRangeRegions.GetBounds(&rc, &graphics);

    return rc;
}

bool MySaveImage(/* nullable */ Bitmap* img, const CString& szFilename, CString& szBuffer, SaveImageFormat Format, int Quality, LPCTSTR Folder)
{
    if (Format == sifUknown) {
        Format = sifJPEG;
    } else if (Format == sifDetectByExtension) {
        Format = GetFormatByFileName(szFilename);
    }

    TCHAR* szImgTypes[5] = { _T("jpg"), _T("png"), _T("gif"), _T("webp"), _T("webp")};

    CString szNameBuffer, buffer2;

    if (IuCommonFunctions::IsImage(szFilename)) {
        szNameBuffer = WinUtils::GetOnlyFileName(szFilename);
    } else {
        szNameBuffer = szFilename;
    }
    CString userFolder;
    if (Folder) {
        userFolder = Folder;
    }
    if (userFolder.Right(1) != _T('\\')) {
        userFolder += _T('\\');
    }
    buffer2.Format(_T("%s%s.%s"), (Folder ? userFolder : AppParams::instance()->tempDirectoryW()).GetString(), szNameBuffer.GetString(),
                    szImgTypes[Format]);
    if (!img) {
        szBuffer = buffer2;
        return true;
    }
    CString resultFilename = WinUtils::GetUniqFileName(buffer2);

    bool res = SaveImageToFile(img, resultFilename, nullptr, Format, Quality);

    szBuffer = resultFilename;
    return res;
}

CString GdiplusStatusToString(Gdiplus::Status statusID) {
    switch (statusID) {

    case Gdiplus::Ok: return "Ok"; break;
    case Gdiplus::GenericError: return "GenericError"; break;
    case Gdiplus::InvalidParameter: return "InvalidParameter"; break;
    case Gdiplus::OutOfMemory: return "OutOfMemory"; break;
    case Gdiplus::ObjectBusy: return "ObjectBusy"; break;
    case Gdiplus::InsufficientBuffer: return "InsufficientBuffer"; break;
    case Gdiplus::NotImplemented: return "NotImplemented"; break;
    case Gdiplus::Win32Error: return "Win32Error"; break;
    case Gdiplus::Aborted: return "Aborted"; break;
    case Gdiplus::FileNotFound: return "FileNotFound"; break;
    case Gdiplus::ValueOverflow: return "ValueOverflow"; break;
    case Gdiplus::AccessDenied: return "AccessDenied"; break;
    case Gdiplus::UnknownImageFormat: return "UnknownImageFormat"; break;
    case Gdiplus::FontFamilyNotFound: return "FontFamilyNotFound"; break;
    case Gdiplus::FontStyleNotFound: return "FontStyleNotFound"; break;
    case Gdiplus::NotTrueTypeFont: return "NotTrueTypeFont"; break;
    case Gdiplus::UnsupportedGdiplusVersion: return "UnsupportedGdiplusVersion"; break;
    case Gdiplus::GdiplusNotInitialized: return "GdiplusNotInitialized"; break;
    case Gdiplus::PropertyNotFound: return "PropertyNotFound"; break;
    case Gdiplus::PropertyNotSupported: return "PropertyNotSupported"; break;
    //case Gdiplus::ProfileNotFound: return "ProfileNotFound"; break;
    default: return "Status Type Not Found."; break;

    }
}

std::unique_ptr<Bitmap> RemoveAlpha(Bitmap* bm, Color color) {
    auto res = std::make_unique<Bitmap>(bm->GetWidth(), bm->GetHeight(), PixelFormat32bppARGB);

    if (res->GetLastStatus() != Ok) {
        return {};
    }
    Graphics gr(res.get());
    SolidBrush br(color);
    gr.FillRectangle(&br, 0, 0, bm->GetWidth(), bm->GetHeight());
    gr.DrawImage(bm, 0, 0);

    return res;
}

bool SaveImageToFile(Gdiplus::Bitmap* img, const CString& fileName, IStream* stream, SaveImageFormat Format, int Quality, CString* mimeType) {
    const CString folder = WinUtils::GetFilePath(fileName);

    if (!folder.IsEmpty()) {
        if (!WinUtils::FileExists(folder)) {
            if (!WinUtils::CreateFolder(folder)) {
                throw IOException("Could not create the folder '" + W2U(folder) + "'", W2U(fileName));
            }
        }  else if (!WinUtils::IsDirectory(folder)) {
            throw IOException("This is not a folder: '" + W2U(folder) + "'", W2U(fileName));
        }
    }

    std::unique_ptr<Bitmap> quantizedImage;

    if (Format == sifDetectByExtension) {
        Format = GetFormatByFileName(fileName);
    }

    Gdiplus::Status result;
    TCHAR szMimeTypes[5][12] = { _T("image/jpeg"), _T("image/png"), _T("image/gif"), _T("image/webp"), _T("image/webp") };
    CLSID clsidEncoder;
    EncoderParameters eps;
    eps.Count = 1;

    if (mimeType) {
        *mimeType = szMimeTypes[Format];
    }
    if (Format == sifJPEG) // JPEG
    {
        eps.Parameter[0].Guid = EncoderQuality;
        eps.Parameter[0].Type = EncoderParameterValueTypeLong;
        eps.Parameter[0].NumberOfValues = 1;
        eps.Parameter[0].Value = &Quality;
    } else if (Format == sifPNG) // PNG
    {
        eps.Parameter[0].Guid = EncoderCompression;
        eps.Parameter[0].Type = EncoderParameterValueTypeLong;
        eps.Parameter[0].NumberOfValues = 1;
        eps.Parameter[0].Value = &Quality;
    } else if (Format == sifGIF) { // GIF
        ColorQuantizer quantizer;
        quantizedImage = quantizer.getQuantized(img, (Quality < 50) ? 16 : 256);
        if (quantizedImage) {
            img = quantizedImage.get();
        }
    } else if (Format == sifWebp) {
       return SaveBitmapAsWebp(img, fileName, stream, false, Quality);
    } else if (Format == sifWebpLossless) {
        return SaveBitmapAsWebp(img, fileName, stream, true, Quality);
    }
    if (GetEncoderClsid(szMimeTypes[Format], &clsidEncoder) != -1) {
        if (Format == 0) {
            result = stream ? img->Save(stream, &clsidEncoder, &eps) : img->Save(fileName, &clsidEncoder, &eps);
        } else {
            result = stream ? img->Save(stream, &clsidEncoder, &eps) : img->Save(fileName, &clsidEncoder);
        }
    } else {
        throw std::runtime_error("Could not find a suitable converter");
    }

    if (result != Ok) {
        DWORD lastError = GetLastError();
        CString error = GdiplusStatusToString(result);
        if (result == Gdiplus::Win32Error) {
            error += L"\r\n" + WinUtils::FormatWindowsErrorMessage(lastError) + L"";
        }

        throw IOException("Could not save image, Gdiplus status=" + W2U(error), W2U(fileName));
    }

    return true;
}

SaveImageFormat GetFormatByFileName(CString filename) {
    CString ext = WinUtils::GetFileExt(filename);
    ext.MakeLower();
    if (ext == _T("jpg") || ext == _T("jpeg") || ext == _T("jpe") || ext == _T("jif") || ext == _T("jfif")) {
        return sifJPEG;
    } else if (ext == _T("gif")) {
        return sifGIF;
    } else if (ext == _T("png")) {
        return sifPNG;
    } else if (ext == _T("webp")) {
        return sifWebpLossless;
    }
    return sifJPEG;
}

CRect CenterRect(CRect r1, const CRect& intoR2)
{
    r1.OffsetRect((intoR2.Width() - r1.Width()) / 2, (intoR2.Height() - r1.Height()) / 2);
    return r1;
}


short GetImageOrientation(Image* img) {
    UINT totalBufferSize = 0, numProperties;
    short orient = 0;
    img->GetPropertySize(&totalBufferSize, &numProperties);

    if (totalBufferSize) {
        PropertyItem* all_items = (PropertyItem*)malloc(totalBufferSize);
        if (!all_items) {
            return orient;
        }
        img->GetAllPropertyItems(totalBufferSize, numProperties, all_items);

        for (UINT j = 0; j < numProperties; ++j) {
            if (all_items[j].id == 0x0112) {

                memcpy(&orient, all_items[j].value, sizeof(orient));


                break; // only orientation
            }
        }
        free(all_items);
    }
    return orient;
}

bool RotateAccordingToOrientation(short orient, Image* img, bool removeTag) {
    if (orient == 2) {
        img->RotateFlip(RotateNoneFlipX);
    } else if (orient == 3) {
        img->RotateFlip(Rotate180FlipNone);
    } else if (orient == 4) {
        img->RotateFlip(RotateNoneFlipY);
    } else if (orient == 5) {
        img->RotateFlip(Rotate90FlipX);
    } else if (orient == 6) {
        img->RotateFlip(Rotate270FlipXY);
    } else if (orient == 7) {
        img->RotateFlip(Rotate270FlipX);
    } else if (orient == 8) {
        img->RotateFlip(Rotate90FlipXY);
    }
    // This EXIF data is now invalid and should be removed.
    //img->RemovePropertyItem(PropertyTagOrientation);
    return false;
}

std::unique_ptr<GdiPlusImage> LoadImageFromFileExtended(const CString& fileName) {
    ImageLoader loader;
    auto result = loader.loadFromFile(fileName);
    if (!result) {
        ServiceLocator::instance()->logger()->write(ILogger::logWarning, _T("Image Loader"),
            _T("Cannot load image.") + CString(L"\r\n") + loader.getLastError().c_str(),
            CString(_T("File:")) + _T(" ") + fileName);
    }
    return result;
}

using PropertyItemPtr = std::unique_ptr<PropertyItem, void(*)(PropertyItem*)>;

PropertyItemPtr GetPropertyItemFromImage(Gdiplus::Image* bm, PROPID propId) {
    auto deleter = [](PropertyItem* ptr) {
        delete[] reinterpret_cast<uint8_t*>(ptr);
    };
    UINT itemSize = bm->GetPropertyItemSize(propId);
    if (!itemSize) {
        return PropertyItemPtr(nullptr, deleter);
    }
    PropertyItemPtr item(reinterpret_cast<PropertyItem*>(new uint8_t[itemSize]), deleter);
    if (bm->GetPropertyItem(propId, itemSize, item.get()) != Ok) {
        return PropertyItemPtr(nullptr, deleter);
    }
    return item;
}

UINT VoidToInt(void* data, unsigned int size) {
    switch (size) {
        case 8:
            return *static_cast<UINT*>(data);
        case 4:
            return *static_cast<DWORD*>(data);
        case 2:
            return *static_cast<WORD*>(data);
        default:
            return *static_cast<BYTE*>(data);
    }
}


CComPtr<IStream>  CreateMemStream(const BYTE* pInit, UINT cbInit) {
    CComPtr<IStream> res;
    res.Attach(SHCreateMemStream(pInit, cbInit));
    return res;
}

std::unique_ptr<Gdiplus::Bitmap> BitmapFromMemory(BYTE* data, size_t imageSize) {
    std::unique_ptr<Gdiplus::Bitmap> bitmap;
    IStream* pStream = SHCreateMemStream(data, imageSize);
    if (pStream) {
        bitmap.reset(Gdiplus::Bitmap::FromStream(pStream));
        pStream->Release();
        if (bitmap) {
            if (bitmap->GetLastStatus() == Gdiplus::Ok) {
                return bitmap;
            }
        }
    }

    return nullptr;
}

// Based on original method from http://danbystrom.se/2009/01/05/imagegetthumbnailimage-and-beyond/
std::unique_ptr<Gdiplus::Bitmap> GetThumbnail(Gdiplus::Image* bm, int width, int height, Gdiplus::Size* realSize) {
    using namespace Gdiplus;
    if (realSize) {
        realSize->Width = bm->GetWidth();
        realSize->Height = bm->GetHeight();
    }
    Size sz = AdaptProportionalSize(Size(width, height), Size(bm->GetWidth(), bm->GetHeight()));
    std::unique_ptr<Bitmap> res = std::make_unique<Bitmap>(sz.Width, sz.Height);
    Graphics gr(res.get());

    gr.SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    UINT size = bm->GetPropertyItemSize(PropertyTagThumbnailData);
    if (size) {
        // Loading thumbnail from EXIF data (fast)
        enum ThumbCompression { ThumbCompressionJPEG, ThumbCompressionRGB, ThumbCompressionYCbCr, ThumbCompressionUnknown }
            compression = ThumbCompressionJPEG;

        short orient = 0;
        auto orientationItem = GetPropertyItemFromImage(bm, PropertyTagOrientation);
        if (orientationItem) {
            memcpy(&orient, orientationItem->value, sizeof(orient));
        }
        auto thumbnailFormatItem = GetPropertyItemFromImage(bm, PropertyTagThumbnailFormat);
        if (thumbnailFormatItem) {
            UINT format = VoidToInt(thumbnailFormatItem->value, thumbnailFormatItem->length);
            if (format == 0) {
                compression = ThumbCompressionRGB;
            } else if (format == 1) {
                compression = ThumbCompressionJPEG;
            } else {
                compression = ThumbCompressionUnknown;
            }
        } else {
            auto compressionItem = GetPropertyItemFromImage(bm, PropertyTagThumbnailCompression);
            if (compressionItem) {
                WORD compressionTag = *static_cast<WORD*>(compressionItem->value);
                if (compressionTag == 1) {
                    compression = ThumbCompressionRGB;
                    auto photometricInterpretationItem = GetPropertyItemFromImage(bm, PropertyTagPhotometricInterp);
                    if (photometricInterpretationItem) {
                        UINT photoMetricInterpretationTag = VoidToInt(photometricInterpretationItem->value, photometricInterpretationItem->length);
                        if (photoMetricInterpretationTag == 6) {
                            compression = ThumbCompressionYCbCr;
                        }
                    }

                } else if (compressionTag == 6) {
                    compression = ThumbCompressionJPEG;
                }
            }
        }

        int originalThumbWidth = 0, originalThumbHeight = 0;
        if (compression == ThumbCompressionJPEG || compression == ThumbCompressionRGB) {
            auto thumbDataItem = GetPropertyItemFromImage(bm, PropertyTagThumbnailData);
            if (thumbDataItem) {
                if (compression == ThumbCompressionJPEG) {
                    std::unique_ptr<Bitmap> src(BitmapFromMemory(static_cast<BYTE*>(thumbDataItem->value), thumbDataItem->length));
                    if (src) {
                        RotateAccordingToOrientation(orient, src.get());

//                        int ww = src->GetWidth();
//                        int hh = src->GetHeight();
                        gr.DrawImage(src.get(), 0, 0, sz.Width, sz.Height);
                        return res;
                    }
                } else if (compression == ThumbCompressionRGB) {
                    auto widthItem = GetPropertyItemFromImage(bm, PropertyTagThumbnailImageWidth);
                    if (widthItem) {
                        originalThumbWidth = VoidToInt(widthItem->value, widthItem->length);
                    }
                    auto heightItem = GetPropertyItemFromImage(bm, PropertyTagThumbnailImageHeight);
                    if (heightItem) {
                        originalThumbHeight = VoidToInt(heightItem->value, heightItem->length);
                    }
                    if (originalThumbWidth && originalThumbHeight) {
                        BITMAPINFOHEADER bih;
                        memset(&bih, 0, sizeof(bih));
                        bih.biSize = sizeof(bih);
                        bih.biWidth = originalThumbWidth;
                        bih.biHeight = -originalThumbHeight;
                        bih.biPlanes = 1;
                        bih.biBitCount = 24;

                        BITMAPINFO bi;
                        memset(&bi, 0, sizeof(bi));
                        bi.bmiHeader = bih;

                        BYTE* data = static_cast<BYTE*>(thumbDataItem->value);
                        BYTE temp;
                        // Convert RGB to BGR
                        for (unsigned int offset = 0; offset < thumbDataItem->length; offset += 3) {
                            temp = data[offset];
                            data[offset] = data[offset + 2];
                            data[offset + 2] = temp;
                        }
                        Bitmap src(&bi, thumbDataItem->value);

                        if (src.GetLastStatus() == Ok) {
                            gr.DrawImage(&src, 0, 0, sz.Width, sz.Height);
                            return res;
                        }

                    }

                } else {
                    // other type of compression not implemented
                }
            }
        }
    }
    // Fallback - Load full image and draw it  (slow)
    gr.DrawImage(bm, 0, 0, sz.Width, sz.Height);

    return res;
}

std::unique_ptr<Gdiplus::Bitmap> GetThumbnail(const CString& filename, int width, int height, Gdiplus::Size* realSize, CString* imageFormat) {
    using namespace Gdiplus;
    std::unique_ptr<GdiPlusImage> img = LoadImageFromFileExtended(filename);
    if (!img) {
        return {};
    }

    std::unique_ptr<Bitmap> bm (img->releaseBitmap());

    if (!bm || bm->GetLastStatus() != Ok) {
        return {};
    }
    std::unique_ptr<Gdiplus::Bitmap> res = GetThumbnail(bm.get(), width, height, realSize);
    if (res && imageFormat) {
        *imageFormat = U2W(img->getSrcFormat());
    }
    return res;
}

Gdiplus::Size ProportionalSize(const Gdiplus::Size& originalSize, const Gdiplus::Size& newSize) {
    if (originalSize.Width < 1 || originalSize.Height < 1 || newSize.Width < 1 || newSize.Height < 1) {
        return {};
    }

    double ratio = static_cast<double>(originalSize.Width) / originalSize.Height;

    Size maximizedToWidth { newSize.Width, static_cast<int>(round(newSize.Width / ratio)) };
    Size maximizedToHeight { static_cast<int>(round(newSize.Height* ratio)), newSize.Height };

    if (maximizedToWidth.Width > newSize.Width) {
        return maximizedToHeight;
    } else {
        return maximizedToWidth;
    }
}

Gdiplus::Size AdaptProportionalSize(const Gdiplus::Size& szMax, const Gdiplus::Size& szReal)
{
    if (szMax.Width < 1 || szMax.Height < 1 || szReal.Width < 1 || szReal.Height < 1) {
        return {};
    }

    double sMaxRatio = szMax.Width / static_cast<double>(szMax.Height);
    double sRealRatio = szReal.Width / static_cast<double>(szReal.Height);

    int nWidth;
    int nHeight;

    if (sMaxRatio < sRealRatio) {
        nWidth = min(szMax.Width, szReal.Width);
        nHeight = static_cast<int>(round(nWidth / sRealRatio));
    } else {
        nHeight = min(szMax.Height, szReal.Height);
        nWidth = static_cast<int>(round(nHeight * sRealRatio));
    }

    return {nWidth, nHeight};
}

/*
void DrawStrokedText(Graphics &gr, LPCTSTR Text,RectF Bounds,Font &font,Color &ColorText,Color &ColorStroke,int HorPos,int VertPos, int width)
{
RectF OriginalTextRect;
//    , NewTextRect;
FontFamily ff;
font.GetFamily(&ff);

gr.SetPageUnit(UnitPixel);
gr.MeasureString(Text,-1,&font,PointF(0,0),&OriginalTextRect);



Font NewFont(&ff,48,font.GetStyle(),UnitPixel);
Rect realSize = MeasureDisplayString(gr, Text, RectF(0,0,1000,200), NewFont);

GraphicsPath path;
StringFormat format;

//format.SetAlignment(StringAlignmentCenter);
//format.SetLineAlignment ( StringAlignmentCenter);
StringFormat * f = format.GenericTypographic()->Clone();
f->SetTrimming(StringTrimmingNone);
f->SetFormatFlags( StringFormatFlagsMeasureTrailingSpaces);

path.AddString(Text, -1,&ff, (int)NewFont.GetStyle(), NewFont.GetSize(), Point(0,0), f);
RectF realTextBounds;
path.GetBounds(&realTextBounds);

float k = 2*width*realTextBounds.Height/OriginalTextRect.Height;
Pen pen(ColorStroke,(float)k);
realTextBounds.Inflate(k, k);
pen.SetAlignment(PenAlignmentCenter);
//path.GetBounds(&realTextBounds,0,&pen);
k = 2*width*realTextBounds.Height/OriginalTextRect.Height; // recalculate K
//Matrix matrix(1.0f, 0.0f, 0.0f, 1.0f, -realTextBounds.X,-realTextBounds.Y );
//path.Transform(&matrix);
//path.GetBounds(&realTextBounds,0,&pen);
//gr.SetPageUnit(UnitPixel);


//Font NewFont(&ff,48,font.GetStyle(),UnitPixel);

RectF NewTextRect;

gr.MeasureString(Text,-1,&NewFont,RectF(0,0,5000,1600),f, &NewTextRect);

RectF RectWithPaddings;
StringFormat f2;
gr.MeasureString(Text,-1,&NewFont,RectF(0,0,5000,1600),&f2, &RectWithPaddings);


OriginalTextRect.Height = OriginalTextRect.Height-OriginalTextRect.Y;
float newwidth,newheight;
newheight = OriginalTextRect.Height;
newwidth=OriginalTextRect.Height/realTextBounds.Height*realTextBounds.Width;

SolidBrush br(ColorText);

Bitmap temp((int)(realTextBounds.Width+realTextBounds.X),(int)(realTextBounds.Height+realTextBounds.Y),&gr);
//Bitmap temp((int)NewTextRect.Width,(int)NewTextRect.Height,&gr);

Graphics gr_temp(&temp);

gr_temp.SetPageUnit(UnitPixel);
//    GraphicsPath path;
gr_temp.SetSmoothingMode( SmoothingModeHighQuality);
//path.AddString(Text, -1,&ff, (int)NewFont.GetStyle(), NewFont.GetSize(), Point(0,0), &format);
gr_temp.SetSmoothingMode( SmoothingModeHighQuality);


SolidBrush br2(Color(255,0,0));
float x,y;
//gr_temp.FillRectangle(&br2, (int)realTextBounds.X, (int)realTextBounds.Y,(int)(realTextBounds.Width),(int)(realTextBounds.Height));
//gr_temp.Set( SmoothingModeHighQuality);
gr_temp.SetInterpolationMode(InterpolationModeHighQualityBicubic  );
gr_temp.DrawPath(&pen, &path);
gr_temp.FillPath(&br, &path);

gr.SetSmoothingMode( SmoothingModeHighQuality);
gr.SetInterpolationMode(InterpolationModeHighQualityBicubic  );

if(HorPos == 0)
x = 2;
else if(HorPos == 1)
x = (Bounds.Width-newwidth)/2;
else x=(Bounds.Width-newwidth)-2;

if(VertPos==0)
y=2;
else if(VertPos==1)
y=(Bounds.Height-newheight)/2;
else y=(Bounds.Height-newheight)-2;


Rect destRect ((int)(Bounds.GetLeft()+x), (int)(Bounds.GetTop()+y), (int)(newwidth),(int)(newheight));
//    gr.SolidBrush br3(Color(0,255,0));

//gr.FillRectangle(&br2, 0,0,(int)(realTextBounds.Width+realTextBounds.X),(int)(realTextBounds.Height+realTextBounds.Y));

gr.DrawImage(&temp, destRect,(int)realTextBounds.X, (int)realTextBounds.Y,(int)(realTextBounds.Width),(int)(realTextBounds.Height), UnitPixel);
}*/

bool CopyDataToClipboardInDataUriFormat(ULONGLONG dataSize, std::string mimeType, bool html, std::function<size_t(void*, size_t)> readCallback) {
    ULONGLONG offset = 0;
    size_t leftBytes = static_cast<size_t>(dataSize);
    const ULONG buf_size = 1024 * 32;
    char buffer[buf_size];

    const char* footer = "\" alt=\"\" />";
    int footerLen = strlen(footer);
    const char* head = "<img src=\"";
    int headLen = strlen(head);

    HGLOBAL hglbCopy;
    int cch = static_cast<int>(dataSize * 4 / 3 + 40);
    if (html) {
        cch += footerLen + headLen;
    }
    if (!OpenClipboard(NULL)) {
        LOG(ERROR) << "Failed to open clipboard";
        return FALSE;
    }

    EmptyClipboard();
    size_t bytesToAlloc = (cch + 1) * sizeof(char);
    hglbCopy = GlobalAlloc(GMEM_MOVEABLE, bytesToAlloc);
    if (hglbCopy == NULL) {
        LOG(ERROR) << "Failed to alloc global memory (" << bytesToAlloc << " bytes).";
        CloseClipboard();
        return FALSE;
    }

    base64_state state;
    base64_stream_encode_init(&state, 0);
    char* encodedData = static_cast<char*>(GlobalLock(hglbCopy));
    char* encodedDataCur = encodedData;

    if (html) {

        strncpy(encodedDataCur, head, headLen);
        encodedDataCur += headLen;
    }
    strncpy(encodedDataCur, "data:", 5);
    encodedDataCur += 5;

    strncpy(encodedDataCur, mimeType.c_str(), mimeType.length());
    encodedDataCur += mimeType.length();
    strncpy(encodedDataCur, ";base64,", 8);
    encodedDataCur += 8;
    size_t outlen = 0;
    while (offset < dataSize) {
        size_t readBytes = readCallback(buffer, std::min<size_t>(buf_size, leftBytes));
        if (!readBytes) {
            break;
        }
        leftBytes -= readBytes;
        offset += readBytes;
        outlen = 0;
        base64_stream_encode(&state, buffer, readBytes, encodedDataCur, &outlen);
        encodedDataCur += outlen;
    }
    base64_stream_encode_final(&state, encodedDataCur, &outlen);
    encodedDataCur += outlen;
    if (html) {
        strncpy(encodedDataCur, footer, footerLen);
        encodedDataCur += footerLen;
    }

    *encodedDataCur = 0;

    GlobalUnlock(hglbCopy);
    SetClipboardData(CF_TEXT, hglbCopy);

    if (html) {
        std::string clipboardHtml = WinUtils::TextToClipboardHtmlFormat(encodedData, encodedDataCur - encodedData);
        unsigned htmlClipboardFormatId = RegisterClipboardFormat(_T("HTML Format"));
        size_t clipboardHtmlSize = clipboardHtml.size();
        HGLOBAL hglbHtml = GlobalAlloc(GMEM_MOVEABLE, clipboardHtmlSize + 1);
        char* htmlData = static_cast<char*>(GlobalLock(hglbHtml));

        strncpy(htmlData, clipboardHtml.c_str(), clipboardHtmlSize);
        htmlData[clipboardHtmlSize] = 0;
        GlobalUnlock(hglbHtml);
        SetClipboardData(htmlClipboardFormatId, hglbHtml);
    }
    CloseClipboard();
    return true;
}

bool CopyFileToClipboardInDataUriFormat(const CString& fileName, int Format, int Quality, bool html) {
    std::string fileNameUtf8 = W2U(fileName);
    std::string mimeType = IuCoreUtils::GetFileMimeType(fileNameUtf8);
    FILE* f = IuCoreUtils::FopenUtf8(fileNameUtf8.c_str(), "rb");
    if (!f) {
        LOG(ERROR) << boost::format("Could not save xml to file '%s'.") % fileName << std::endl << "Reason: " << strerror(errno);
        return false;
    }
    int64_t fileSize = IuCoreUtils::GetFileSize(fileNameUtf8);
    if (fileSize > 10 * 1024 * 1024) {
        LOG(ERROR) << fileNameUtf8 << std::endl << "File is too big";
        return false;
    }

    bool res = CopyDataToClipboardInDataUriFormat(fileSize, mimeType, html, [&](void* buffer, size_t size) -> size_t {
        if (feof(f)) {
            return 0;
        }
        return fread(buffer, 1, size, f);
    });
    fclose(f);
    return res;
}

bool CopyBitmapToClipboardInDataUriFormat(Bitmap* bm, SaveImageFormat Format, int Quality, bool html) {
    CComPtr<IStream> stream = CreateMemStream(nullptr, 0);
    CString mimeType;
    bool res = SaveImageToFile(bm, CString(), stream, Format, Quality, &mimeType);
    std::string mimeTypeA = W2U(mimeType);
    STATSTG stats;
    HRESULT hr = stream->Stat(&stats, STATFLAG_NONAME);
    if (SUCCEEDED(hr)) {
        hr = stream->Seek({ 0, 0 }, STREAM_SEEK_SET, nullptr);
        if (SUCCEEDED(hr)) {
            CopyDataToClipboardInDataUriFormat(stats.cbSize.QuadPart, mimeTypeA, html, [&](void* buffer, size_t size) -> size_t {
                unsigned long readBytes;
                hr = stream->Read(buffer, size, &readBytes);
                if (!SUCCEEDED(hr)) {
                    return 0;
                }
                return readBytes;
            });
        }
    }
    return res;

}

ImageInfo GetImageInfo(const wchar_t* fileName) {
    auto img = LoadImageFromFileExtended(fileName);

    ImageInfo res;
    if (!img) {
        return res;
    }
    auto* bm = img->getBitmap();
    if (bm) {
        res.width = bm->GetWidth();
        res.height = bm->GetHeight();
    }
    return res;
}

bool SaveImageFromCliboardDataUriFormat(const CString& clipboardText, CString& fileName) {
    if (clipboardText.Left(5) != _T("data:")) {
        return false;
    }
    int commaPos = clipboardText.Find(_T(","), 5);
    if (commaPos == -1) {
        return false;
    }
    CString mediaType = clipboardText.Mid(5, commaPos - 5);
    int startPos = 0;
    bool base64 = false;
    CString contentType = "text/plain";
    while (startPos != -1) {
        CString token = mediaType.Tokenize(_T(";"), startPos);
        if (!token.IsEmpty() && token.Find(_T("=")) == -1) {
            if (token == _T("base64")) {
                base64 = true;
            } else {
                contentType = token;
            }
        }
    }
    if (!base64 || contentType.Left(6) != _T("image/")) {
        return false;
    }
    std::string encodedImg = W2U(clipboardText.Right(clipboardText.GetLength() - commaPos - 1));
    if (encodedImg.empty()) {
        return false;
    }
    try {
        size_t outLen = 0;
        size_t bufferLen = encodedImg.length() * 3 / 4;
        if (!bufferLen) {
            return false;
        }
        auto decodedImg = std::make_unique<char[]>(bufferLen);
        base64_decode(encodedImg.data(), encodedImg.length(), decodedImg.get(), &outLen, 0);
        if (!outLen) {
            return false;
        }
        CString tempDirectory = AppParams::instance()->tempDirectoryW();
        CString extension = U2W(IuCoreUtils::GetDefaultExtensionForMimeType(W2U(contentType)));
        if (extension.IsEmpty()) {
            extension = _T("dat");
        }
        CString outFilename = WinUtils::GetUniqFileName(tempDirectory + _T("clipboard.") + extension);
        FILE* outFile = IuCoreUtils::FopenUtf8(W2U(outFilename).c_str(), "wb");
        if (!outFile) {
            return false;
        }
        size_t bytesWritten = fwrite(decodedImg.get(), 1, outLen, outFile);
        fclose(outFile);
        if (bytesWritten == outLen) {
            fileName = outFilename;
            return true;
        }
    } catch (std::bad_alloc& ) {
        return false;
    }
    return false;
}

// Allocates storage for entire file 'file_name' and returns contents and size
std::pair<std::unique_ptr<uint8_t[]>, size_t> ExUtilReadFile(const wchar_t* file_name) {

    std::unique_ptr<uint8_t[]> file_data;

    if (file_name == nullptr) {
        return {};
    }

    FILE* in = _wfopen(file_name, L"rb");
    if (in == nullptr) {
        throw IOException("Cannot open input file", IuCoreUtils::WstringToUtf8(file_name));
    }
    fseek(in, 0, SEEK_END);
    size_t file_size = ftell(in);
    fseek(in, 0, SEEK_SET);
    try {
        file_data = std::make_unique<uint8_t[]>(file_size);
    }
    catch (std::exception &) {
        LOG(ERROR) << "Unable to allocate " << file_size << " bytes";
        fclose(in);
        return {};
    }
    if (file_data == nullptr) {
        fclose(in);
        return {};
    }
    int ok = (fread(file_data.get(), 1, file_size, in) == file_size);
    fclose(in);

    if (!ok) {
        throw IOException(str(boost::format("Could not read %d bytes of data from file") % file_size), IuCoreUtils::WstringToUtf8(file_name));
    }

    return std::make_pair(std::move(file_data), file_size);
}

bool IsImageMultiFrame(Gdiplus::Image* img) {
    /*GUID format;
    if (img->GetRawFormat(&format) == Gdiplus::Ok) {
        if (format == ImageFormatTIFF) {
            return false;
        }
    }*/

    UINT count = img->GetFrameDimensionsCount();
    auto pDimensionIDs = std::make_unique<GUID[]>(count);

    // Get the list of frame dimensions from the Image object.
    img->GetFrameDimensionsList(pDimensionIDs.get(), count);

    // Get the number of frames in the first dimension.
    int frameCount = img->GetFrameCount(&pDimensionIDs[0]);
    return frameCount > 1;
}

bool SaveBitmapAsWebp(Gdiplus::Bitmap* img, CString fileName, IStream* stream, bool lossless, int quality) {
    BitmapData bdSrc;
    Rect r(0, 0, img->GetWidth(), img->GetHeight());
    if (img->LockBits(&r, ImageLockModeRead, PixelFormat32bppARGB, &bdSrc) == Ok) {
        defer d([img, &bdSrc] {
            img->UnlockBits(&bdSrc);
        });

        BYTE* bpSrc = static_cast<BYTE*>(bdSrc.Scan0);
        uint8_t* outData;
        size_t outDataSize;

        if (lossless) {
            outDataSize = WebPEncodeLosslessBGRA(bpSrc, img->GetWidth(), img->GetHeight(), img->GetWidth() * 4, &outData);
        } else {
            outDataSize = WebPEncodeBGRA(bpSrc, img->GetWidth(), img->GetHeight(), img->GetWidth() * 4, static_cast<float>(quality), &outData);
        }
        if (!outDataSize || !outData) {
            throw IOException("WebPEncodeRGBA failed");
        }
        defer d2([outData] {
            WebPFree(outData);
        });
        bool result = true;
        if (stream) {
            ULONG pcbWritten;
            if (FAILED(stream->Write(outData, outDataSize, &pcbWritten))) {
                throw IOException("Failed to write file to the stream");
            }
        }
        else {
            std::string filenameUtf8 = W2U(fileName);
            std::ofstream f(std::filesystem::u8path(filenameUtf8), std::ios::out | std::ios::binary);

            if (!f) {
                throw IOException("Failed to create output file: " + std::string(strerror(errno)), filenameUtf8);
            }
            f.write(reinterpret_cast<char*>(outData), outDataSize);

            if (!f.good()) {
                throw IOException("Error occurred at writing time!", filenameUtf8);
            }
        }
        return result;
    } else {
        throw IOException("LockBits call failed");
    }
    return false;
}

std::unique_ptr<Gdiplus::Font> StringToGdiplusFont(LPCTSTR szBuffer) {
    TCHAR szFontName[LF_FACESIZE] = _T("Ms Sans Serif");

    TCHAR szFontSize[MAX_PATH] = _T("");
    TCHAR szFormat[MAX_PATH] = _T("");
    TCHAR szCharset[MAX_PATH] = _T("");
    bool bBold = false;
    bool bItalic = false;
    bool bUnderline = false;
    bool bStrikeOut = false;
    int nFontSize = 10;
    int nCharSet = ANSI_CHARSET;

    WinUtils::ExtractStrFromList(szBuffer, 0, szFontName, sizeof(szFontName) / sizeof(TCHAR));
    if (WinUtils::ExtractStrFromList(szBuffer, 1, szFontSize, sizeof(szFontSize) / sizeof(TCHAR)))
    {
        _stscanf(szFontSize, _T("%d"), &nFontSize);
    }

    WinUtils::ExtractStrFromList(szBuffer, 2, szFormat, sizeof(szFontSize) / sizeof(TCHAR));

    if (_tcschr(szFormat, 'b')) bBold = true;
    if (_tcschr(szFormat, 'u')) bUnderline = true;
    if (_tcschr(szFormat, 'i')) bItalic = true;
    if (_tcschr(szFormat, 's')) bStrikeOut = true;

    if (WinUtils::ExtractStrFromList(szBuffer, 3, szCharset, sizeof(szCharset) / sizeof(TCHAR))) {
        _stscanf(szCharset, _T("%d"), &nCharSet);
    }

    //lFont->lfCharSet = static_cast<BYTE>(nCharSet);
    int style = Gdiplus::FontStyleRegular;
    if (bItalic) {
        style |= Gdiplus::FontStyleItalic;
    }
    if (bStrikeOut) {
        style |= Gdiplus::FontStyleStrikeout;
    }
    if (bBold) {
        style |= Gdiplus::FontStyleBold;
    }
    if (bUnderline) {
        style |= Gdiplus::FontStyleUnderline;
    }
    FontFamily fontFamily(szFontName);
    return std::make_unique<Font>(&fontFamily, static_cast<REAL>(nFontSize), style, Gdiplus::UnitPixel);
}

CString ImageFormatGUIDToString(GUID guid) {
    if (guid == ImageFormatBMP) {
        return "bmp";
    }
    if (guid == ImageFormatEMF) {
        return "emf";
    }
    if (guid == ImageFormatEXIF) {
        return "exif";
    }
    if (guid == ImageFormatGIF) {
        return "gif";
    }
    if (guid == ImageFormatIcon) {
        return "ico";
    }
    if (guid == ImageFormatJPEG) {
        return "jpeg";
    }
    if (guid == ImageFormatMemoryBMP) {
        return "bmp";
    }
    if (guid == ImageFormatPNG) {
        return "png";
    }
    if (guid == ImageFormatTIFF) {
        return "tiff";
    }
    if (guid == ImageFormatWEBP) {
        return "webp";
    }
    if (guid == ImageFormatWMF) {
        return "wmf";
    }
    return "unknown";
}

/*
Gdiplus::Bitmap* GetIconPixelData(HICON hIcon)
{
    ICONINFO iconInfo;
    GetIconInfo(hIcon, &iconInfo);

    BITMAP iconBmp;
    GetObject(iconInfo.hbmColor, sizeof(BITMAP), &iconBmp);

    Gdiplus::Bitmap* bitmap = new Gdiplus::Bitmap(iconBmp.bmWidth, iconBmp.bmHeight, PixelFormat32bppARGB);

    bool hasAlpha = false;
    {
        // We have to read the raw pixels of the bitmap to get proper transparency information
        // (not sure why, all we're doing is copying one bitmap into another)

        Gdiplus::Bitmap colorBitmap(iconInfo.hbmColor, NULL);
        Gdiplus::BitmapData bmpData;
        Gdiplus::Rect bmBounds(0, 0, colorBitmap.GetWidth(), colorBitmap.GetHeight());

        colorBitmap.LockBits(&bmBounds, Gdiplus::ImageLockModeRead, colorBitmap.GetPixelFormat(), &bmpData);
        for (int y = 0; y < colorBitmap.GetHeight(); y++) {
            byte* pixelBytes = (byte*)bmpData.Scan0 + y * bmpData.Stride;

            for (int x = 0; x < colorBitmap.GetHeight(); x++) {
                ARGB* pixel = (ARGB*)(pixelBytes + x * 4);
                bitmap->SetPixel(x, y, Gdiplus::Color(*pixel));
                hasAlpha = hasAlpha || (pixelBytes[3] > 0 && pixelBytes[3] < 255);
            }
        }

        colorBitmap.UnlockBits(&bmpData);
    }

    if (!hasAlpha) {
        // If there's no alpha transparency information, we need to use the mask
        // to turn back on visible pixels

        Gdiplus::Bitmap maskBitmap(iconInfo.hbmMask, NULL);
        Gdiplus::Color cMask, cBitmap;
        for (int y = 0; y < maskBitmap.GetHeight(); y++) {
            for (int x = 0; x < maskBitmap.GetWidth(); x++) {
                maskBitmap.GetPixel(x, y, &cBitmap);
                cBitmap.SetValue(cBitmap.GetValue() | 0xFF000000); // turn alpha to opaque (i.e. 0xFF)
                bitmap->SetPixel(x, y, cBitmap);
            }
        }
    }
    return bitmap;
}*/

std::pair<std::unique_ptr<Gdiplus::Bitmap>,std::unique_ptr<BYTE[]>> GetIconPixelData(HICON hIcon) {
    if (hIcon == NULL){
        return {};
    }
    ICONINFO icInfo = { 0 };

    if (!::GetIconInfo(hIcon, &icInfo)) {
        return {};
    }

    BITMAP bitmap;
    GetObject(icInfo.hbmColor, sizeof(BITMAP), &bitmap);
    std::unique_ptr<Bitmap> pBitmap;
    std::unique_ptr<Bitmap> pWrapBitmap;
    std::unique_ptr<BYTE[]> data;
    if (bitmap.bmBitsPixel != 32) {
        pBitmap.reset(Bitmap::FromHICON(hIcon));
    } else {
        pWrapBitmap.reset(Bitmap::FromHBITMAP(icInfo.hbmColor, NULL));
        BitmapData bitmapData;
        Rect rcImage(0, 0, pWrapBitmap->GetWidth(), pWrapBitmap->GetHeight());

        if (pWrapBitmap->LockBits(&rcImage, ImageLockModeRead, pWrapBitmap->GetPixelFormat(), &bitmapData) == Ok) {
            int absStride = std::abs(bitmapData.Stride);
            size_t size = absStride * bitmapData.Height;

            data = std::make_unique<BYTE[]>(size);

            for (int y = 0; y < bitmapData.Height; y++) {
                BYTE* srcData = static_cast<BYTE*>(bitmapData.Scan0) + y* bitmapData.Stride;
                memcpy(data.get() + y * absStride, srcData, bitmapData.Width * 4);
            }

            pBitmap = std::make_unique<Bitmap>(bitmapData.Width, bitmapData.Height, absStride,
                PixelFormat32bppARGB, data.get());
            pWrapBitmap->UnlockBits(&bitmapData);
        }
    }

    DeleteObject(icInfo.hbmColor);
    DeleteObject(icInfo.hbmMask);

    return std::make_pair(std::move(pBitmap), std::move(data));
}

std::unique_ptr<Gdiplus::Bitmap> CreateAlphaBitmap(Gdiplus::Bitmap* srcBitmap, Gdiplus::PixelFormat targetPixelFormat) {
    if (!srcBitmap) {
        return nullptr;
    }

    auto result = std::make_unique<Gdiplus::Bitmap>(srcBitmap->GetWidth(), srcBitmap->GetHeight(), targetPixelFormat);

    if (!result || result->GetLastStatus() != Gdiplus::Ok) {
        return nullptr;
    }

    Gdiplus::Rect bmpBounds(0, 0, srcBitmap->GetWidth(), srcBitmap->GetHeight());
    Gdiplus::BitmapData srcData, destData;

    if (srcBitmap->LockBits(&bmpBounds, Gdiplus::ImageLockModeRead, srcBitmap->GetPixelFormat(), &srcData) != Gdiplus::Ok) {
        return nullptr;
    }

    if (result->LockBits(&bmpBounds, Gdiplus::ImageLockModeWrite, targetPixelFormat, &destData) != Gdiplus::Ok) {
        srcBitmap->UnlockBits(&srcData);
        return nullptr;
    }

    BYTE* srcPtr = static_cast<BYTE*>(srcData.Scan0);
    BYTE* destPtr = static_cast<BYTE*>(destData.Scan0);


    if (srcData.Stride == destData.Stride && srcData.Stride > 0) {
        size_t totalBytes = abs(srcData.Stride) * srcData.Height;
        memcpy(destPtr, srcPtr, totalBytes);
    } else {
        int srcStride = srcData.Stride;
        int destStride = destData.Stride;
        int rowBytes = srcData.Width * 4;

        for (int y = 0; y < srcData.Height; y++) {
            BYTE* srcRow = srcPtr + (srcStride * y);
            BYTE* destRow = destPtr + (destStride * y);
            memcpy(destRow, srcRow, rowBytes);
        }
    }

    srcBitmap->UnlockBits(&srcData);
    result->UnlockBits(&destData);

    return result;
}

std::unique_ptr<Gdiplus::Bitmap> GetBitmapFromHBitmap(HBITMAP nativeHBitmap) {
    using namespace Gdiplus;
    std::unique_ptr<Gdiplus::Bitmap> bmp(Bitmap::FromHBITMAP(nativeHBitmap, 0));

    if (GetPixelFormatSize(bmp->GetPixelFormat()) != 32) {
        return bmp;
    }
    return CreateAlphaBitmap(bmp.get(), PixelFormat32bppARGB);
}

std::unique_ptr<Gdiplus::Bitmap> ConvertBitmapWithPremultipliedAlphaV2(HBITMAP hBitmap) {
    BITMAP bmp;
    if (GetObject(hBitmap, sizeof(BITMAP), &bmp) == 0) {
        return {};
    }

    auto ppBitmap = std::make_unique<Gdiplus::Bitmap>(bmp.bmWidth, bmp.bmHeight, PixelFormat32bppPARGB);

    Gdiplus::BitmapData bitmapData;
    Gdiplus::Rect rect(0, 0, bmp.bmWidth, bmp.bmHeight);

    if (ppBitmap->LockBits(&rect, Gdiplus::ImageLockModeWrite, PixelFormat32bppPARGB, &bitmapData) == Gdiplus::Ok) {

        HDC hdcSrc = CreateCompatibleDC(nullptr);
        HGDIOBJ hOldBmp = SelectObject(hdcSrc, hBitmap);

        BITMAPINFO bi = { 0 };
        bi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bi.bmiHeader.biWidth = bmp.bmWidth;
        bi.bmiHeader.biHeight = -bmp.bmHeight; 
        bi.bmiHeader.biPlanes = 1;
        bi.bmiHeader.biBitCount = 32;
        bi.bmiHeader.biCompression = BI_RGB;

        GetDIBits(hdcSrc, hBitmap, 0, bmp.bmHeight, bitmapData.Scan0, &bi, DIB_RGB_COLORS);

        SelectObject(hdcSrc, hOldBmp);
        DeleteDC(hdcSrc);

        ppBitmap->UnlockBits(&bitmapData);
        return ppBitmap;
    }
    return {};
}

}
