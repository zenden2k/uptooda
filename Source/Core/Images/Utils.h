#ifndef IU_CORE_IMAGES_UTILS_H
#define IU_CORE_IMAGES_UTILS_H

#include <memory>
#include <string>
#include "atlheaders.h"
#include "3rdpart/GdiplusH.h"
#include "Core/Images/GdiPlusImage.h"

namespace ImageUtils {

enum SaveImageFormat {
    sifUknown = -1,
    sifJPEG = 0,
    sifPNG,
    sifGIF,
    sifWebp,
    sifWebpLossless,
    sifDetectByExtension,
};

struct ImageInfo {
    int width, height;

    ImageInfo()
    {
        width = 0;
        height = 0;
    }
};

int GetEncoderClsid(const WCHAR* format, CLSID* pClsid);

std::unique_ptr<Gdiplus::Image> ImageFromResource(HINSTANCE hInstance, LPCTSTR szResName, LPCTSTR szResType);
std::unique_ptr<Gdiplus::Bitmap> BitmapFromResource(HINSTANCE hInstance, LPCTSTR szResName, LPCTSTR szResType);
void PrintRichEdit(HWND hwnd, Gdiplus::Graphics* graphics, Gdiplus::Bitmap* background, Gdiplus::Rect layoutArea);
void DrawRoundedRectangle(Gdiplus::Graphics* gr, Gdiplus::Rect r, int d, Gdiplus::Pen* p, Gdiplus::Brush* br);
std::unique_ptr<Gdiplus::Bitmap> IconToBitmap(HICON ico);
void ApplyGaussianBlur(Gdiplus::Bitmap* bm, int x, int y, int w, int h, float radius, Gdiplus::Rect exclude = {});
void ApplyPixelateEffect(Gdiplus::Bitmap* bm, int xPos, int yPos, int w, int h, int blockSize, Gdiplus::Rect exclude = {});
std::unique_ptr<Gdiplus::Bitmap> LoadImageFromFileWithoutLocking(const WCHAR* fileName, bool* isAnimated);
Gdiplus::Color StringToColor(const std::string& str);
CComPtr<IStream> CreateMemStream(const BYTE* pInit, UINT cbInit);
void RemoveAlphaFromBitmap(Gdiplus::Bitmap& source, Gdiplus::Color color);

/**
 * @throws IOException, runtime_error
 */
bool MySaveImage(Gdiplus::Bitmap* img, const CString& szFilename, CString& szBuffer, SaveImageFormat Format,
    int Quality,
    LPCTSTR Folder = 0);

/**
 * @throws IOException, std::runtime_error
 */
bool SaveImageToFile(Gdiplus::Bitmap* img, const CString& fileName, IStream* stream, SaveImageFormat Format,
    int Quality, CString* mimeType = nullptr);
SaveImageFormat GetFormatByFileName(CString filename);
void DrawGradient(Gdiplus::Graphics& gr, Gdiplus::Rect rect, Gdiplus::Color& Color1, Gdiplus::Color& Color2);
void DrawStrokedText(Gdiplus::Graphics& gr, LPCTSTR Text, Gdiplus::RectF Bounds, const Gdiplus::Font& font,
    const Gdiplus::Color& ColorText, const Gdiplus::Color& ColorStroke, int HorPos = 0,
    int VertPos = 0,
    int width = 1);
void ChangeAlphaChannel(Gdiplus::Bitmap& source, Gdiplus::Bitmap& dest, int sourceChannel, int destChannel);
Gdiplus::Rect MeasureDisplayString(Gdiplus::Graphics& graphics, CString text, Gdiplus::RectF boundingRect,
    Gdiplus::Font& font);
CRect CenterRect(CRect r1, const CRect& intoR2);
std::unique_ptr<Gdiplus::Bitmap> GetThumbnail(Gdiplus::Image* bm, int width, int height, Gdiplus::Size* realSize = 0);
std::unique_ptr<Gdiplus::Bitmap> GetThumbnail(const CString& filename, int width, int height,
    Gdiplus::Size* realSize = 0, CString* imageFormat = nullptr);
Gdiplus::Size AdaptProportionalSize(const Gdiplus::Size& szMax, const Gdiplus::Size& szReal);
Gdiplus::Size ProportionalSize(const Gdiplus::Size& originalSize, const Gdiplus::Size& newSize);
std::unique_ptr<Gdiplus::Bitmap> BitmapFromMemory(BYTE* data, size_t size);

/**
 * @throws IOException, runtime_error
 */
bool CopyBitmapToClipboardInDataUriFormat(Gdiplus::Bitmap* bm, SaveImageFormat Format, int Quality, bool html = false);
bool CopyFileToClipboardInDataUriFormat(const CString& fileName, int Format, int Quality, bool html);
bool SaveImageFromClipboardDataUriFormat(const CString& clipboardText, CString& fileName);
// Load image from file with webp support
std::unique_ptr<GdiPlusImage> LoadImageFromFileExtended(const CString& fileName);
CString GdiplusStatusToString(Gdiplus::Status statusID);

/**
 * @throws IOException
 */
std::pair<std::unique_ptr<uint8_t[]>, size_t> ExUtilReadFile(const wchar_t* file_name);
short GetImageOrientation(Gdiplus::Image* img);
bool RotateAccordingToOrientation(short orient, Gdiplus::Image* img, bool removeTag = false);
ImageInfo GetImageInfo(const wchar_t* fileName);

CString GdiplusStatusToString(Gdiplus::Status statusID);
bool IsImageMultiFrame(Gdiplus::Image* img);

/**
 * @throws IOException
 */
bool SaveBitmapAsWebp(Gdiplus::Bitmap* img, CString fileName, IStream* stream, bool lossless, int quality);

std::unique_ptr<Gdiplus::Font> StringToGdiplusFont(LPCTSTR szBuffer);
CString ImageFormatGUIDToString(GUID guid);

std::pair<std::unique_ptr<Gdiplus::Bitmap>, std::unique_ptr<BYTE[]>> GetIconPixelData(HICON hIcon);

std::unique_ptr<Gdiplus::Bitmap> ConvertBitmapWithPremultipliedAlphaV2(HBITMAP hBitmap);

std::unique_ptr<Gdiplus::Bitmap> GetBitmapFromHBitmap(HBITMAP nativeHBitmap);

}
#endif
