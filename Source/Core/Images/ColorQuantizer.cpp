#include "ColorQuantizer.h"

#include "OctreeColorQuantizer.h"

using namespace Gdiplus;

std::unique_ptr<Bitmap> ColorQuantizer::getQuantized(Bitmap* sourceBitmap, Gdiplus::Color backgroundColor, UINT nMaxColors /* = 256 */)
{
    BYTE alphaThreshold = 0;
    UINT width = sourceBitmap->GetWidth();
    UINT height = sourceBitmap->GetHeight();
    Rect rc(0, 0, width, height);
    std::unique_ptr<Bitmap> destBitmap;
    BitmapData dataSource;

    if (sourceBitmap->LockBits(&rc, ImageLockModeRead, PixelFormat32bppARGB, &dataSource) == Ok) {
        BYTE* pScan0Source = static_cast<BYTE*>(dataSource.Scan0);
        UINT strideSource;

        if (dataSource.Stride > 0) {
            strideSource = dataSource.Stride;
        } else {
            pScan0Source += height * dataSource.Stride;
            strideSource = -dataSource.Stride;
        }

        BYTE* pRowSource = pScan0Source;
        OctreeColorQuantizer quantizer(nMaxColors, 8, &dataSource);
        destBitmap = std::make_unique<Bitmap>(width, height, PixelFormat8bppIndexed);

        for (UINT y = 0; y < height; y++) {
            BYTE* pPixelSource = pRowSource;

            for (UINT x = 0; x < width; x++) {
                BYTE b = *pPixelSource++;
                BYTE g = *pPixelSource++;
                BYTE r = *pPixelSource++;
                BYTE a = *pPixelSource++;
                Gdiplus::Color c(a, r, g, b);
                if (c.GetA() != 255) {
                    c = c.GetA() < alphaThreshold ? Color() : BlendWithBackgroundSrgb(c, backgroundColor);
                }

                quantizer.addColor(c);
            }

            pRowSource += strideSource;
        }

        auto pallete = quantizer.generatePalette();
        MyPalette mypal(pallete.get(), backgroundColor, alphaThreshold);

        BitmapData dataDest;
        destBitmap->SetPalette(pallete.get());
        if (destBitmap->LockBits(&rc, ImageLockModeWrite, PixelFormat8bppIndexed, &dataDest) == Ok) {
            BYTE* pRowSource = pScan0Source;

            BYTE* pRowDest = static_cast<BYTE*>(dataDest.Scan0);
            UINT strideDest;

            if (dataDest.Stride > 0) {
                strideDest = dataDest.Stride;
            } else {
                pRowDest += height * dataDest.Stride;
                strideDest = -dataDest.Stride;
            }

            for (UINT y = 0; y < height; y++) {
                BYTE* pPixelSource = pRowSource;
                BYTE* pPixelDest = pRowDest;

                for (UINT x = 0; x < width; x++) {
                    BYTE b = *pPixelSource++;
                    BYTE g = *pPixelSource++;
                    BYTE r = *pPixelSource++;
                    BYTE a = *pPixelSource++;

                    BYTE index = static_cast<BYTE>(mypal.getNearestColorIndex(Gdiplus::Color(a, r, g, b)));

                    *pPixelDest++ = index;
                }

                pRowSource += strideSource;
                pRowDest += strideDest;
            }
            destBitmap->UnlockBits(&dataDest);
        }
        sourceBitmap->UnlockBits(&dataSource);
    }
    return destBitmap;
}
