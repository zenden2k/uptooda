#include "ImageConverterPrivate_gdiplus.h"

#include <cassert>
#include <memory>

#include "Func/MyUtils.h"
#include "Core/Utils/StringUtils.h"
#include "Core/Utils/CoreUtils.h"
#include "Core/Images/Utils.h"
#include "Core/CommonDefs.h"
#include "Func/IuCommonFunctions.h"
#include "Func/WinUtils.h"
#include "Core/Images/GdiPlusImage.h"

#ifndef MYRGB
    #define MYRGB(a,color) Color(a,GetRValue(color),GetGValue(color),GetBValue(color))
#endif

using namespace Gdiplus;
bool ImageConverterPrivate::convert(const std::string& sourceFile)
{
    sourceFile_ = sourceFile;
    ImageUtils::SaveImageFormat fileformat;
    double width, height, imgwidth, imgheight, newwidth, newheight;
    CString sourceFileW = U2W(sourceFile);
    CString imageFile = sourceFileW;
    auto srcImg = ImageUtils::LoadImageFromFileExtended(sourceFileW);

    if (!srcImg) {
        LOG(ERROR) << "ImageConverter: unable to load source file " << sourceFileW;
        return false;
    }
    Bitmap* bm = srcImg->getBitmap();

    if (!bm) {
        LOG(ERROR) << "ImageConverter: unable to load source file " << sourceFileW;
        return false;
    }

    Bitmap* thumbSource = bm;
    std::unique_ptr<Bitmap> BackBuffer;
    imgwidth = float(bm->GetWidth());
    imgheight = float(bm->GetHeight());
    double NewWidth = atof(m_imageConvertingParams.strNewWidth.c_str());
    double NewHeight = atof(m_imageConvertingParams.strNewHeight.c_str());
    if (IuStringUtils::Tail(m_imageConvertingParams.strNewWidth, 1) == "%") {
        NewWidth = NewWidth * imgwidth / 100;
    }

    if ( IuStringUtils::Tail(m_imageConvertingParams.strNewHeight, 1)==  "%" ) {
        NewHeight = NewHeight * imgheight / 100;
    }

    width = float(NewWidth);
    height = float(NewHeight);

    if (m_imageConvertingParams.Format < 1 || !processingEnabled_)
        fileformat = ImageUtils::GetFormatByFileName(sourceFileW);
    else
        fileformat = static_cast<ImageUtils::SaveImageFormat>(m_imageConvertingParams.Format - 1);

    if (m_imageConvertingParams.SmartConverting && fileformat == ImageUtils::GetFormatByFileName(sourceFileW) && imgwidth < width && imgheight < height) {
        processingEnabled_ = false;
    }

    if (m_imageConvertingParams.SkipAnimated && srcImg->isSrcMultiFrame()) {
        processingEnabled_ = false;
    }

    newwidth = imgwidth;
    newheight = imgheight;

    // Если включена опция "Оставить без изменений", просто копируем имя исходного файла
    if (!processingEnabled_)
        resultFileName_ = sourceFile;
    else
    {
        UINT propertyItemsSize = 0;
        UINT propertyItemsCount = 0;
        PropertyItem* pPropBuffer = NULL;
        if (m_imageConvertingParams.PreserveExifInformation) {
            bm->GetPropertySize(&propertyItemsSize, &propertyItemsCount);
            if (propertyItemsSize) {
                pPropBuffer = static_cast<PropertyItem*>(malloc(propertyItemsSize));
                bm->GetAllPropertyItems(propertyItemsSize, propertyItemsCount, pPropBuffer);
            }
        }
        if (m_imageConvertingParams.ResizeMode == ImageConvertingParams::irmFit)
        {
            if (width && height && (imgwidth > width || imgheight > height))
            {
                double S = min(width / imgwidth, height / imgheight);
                newwidth = S * imgwidth;
                newheight = S  * imgheight;
            }
            else
                if (width && imgwidth > width)
                {
                    newwidth = width;
                    newheight = newwidth / imgwidth * imgheight;
                }
                else
                    if (height && imgheight > height)
                    {
                        newheight = height;
                        newwidth = newheight / imgheight * imgwidth;
                    }
        }
        else
        {
            if (imgwidth > width || imgheight > height)
            {
                if (width > 0)
                    newwidth = width;
                if (height > 0)
                    newheight = height;
            }
            else
            {
                newwidth = imgwidth;
                newheight = imgheight;
            }
        }
        BackBuffer.reset(new Bitmap(static_cast<int>(round(newwidth)), static_cast<int>(round(newheight))));

        Graphics gr(BackBuffer.get());
        gr.Clear(Gdiplus::Color::Transparent);
        if (fileformat == ImageUtils::sifJPEG || fileformat == ImageUtils::sifGIF) {
            gr.Clear(Color(255, 255, 255, 255));
        }
       

        gr.SetInterpolationMode(InterpolationModeHighQualityBicubic);

        gr.SetPixelOffsetMode(PixelOffsetModeHalf);

        ImageAttributes attr;
        attr.SetWrapMode(WrapModeTileFlipXY);
        if (m_imageConvertingParams.ResizeMode == ImageConvertingParams::irmFit || m_imageConvertingParams.ResizeMode ==
            ImageConvertingParams::irmStretch)
        {
            if ((!width && !height) || ((int)newwidth == (int)imgwidth && (int)newheight == (int)imgheight))
                gr.DrawImage(/*backBuffer*/ bm, (int)0, (int)0, (int)newwidth, (int)newheight);
            else
                gr.DrawImage(bm,
                RectF(0.0, 0.0, float(newwidth), float(newheight)),
                0,
                0,
                float(bm->GetWidth()),
                float(bm->GetHeight()),
                UnitPixel,
                &attr);
            // gr.DrawImage(/*backBuffer*/&bm, (int)-1, (int)-1, (int)newwidth+2,(int)newheight+2);
        }
        else
            if (m_imageConvertingParams.ResizeMode == ImageConvertingParams::irmCrop)
            {
                int newVisibleWidth = 0;
                int newVisibleHeight = 0;
                double k = 1;
                if (newwidth > newheight)
                {
                    newVisibleWidth = static_cast<int>(min(newwidth, imgwidth));
                    k = newVisibleWidth / imgwidth;
                    newVisibleHeight = static_cast<int>(newVisibleWidth / imgwidth * imgheight);
                }
                else
                {
                    newVisibleHeight = static_cast<int>(min(newheight, imgheight));
                    k = newVisibleHeight / imgheight;
                    newVisibleWidth = static_cast<int>(newVisibleHeight / imgheight * imgwidth);
                }
                CRect r(0, 0, newVisibleWidth, newVisibleHeight);
                CRect destRect = ImageUtils::CenterRect(r, CRect(0, 0, static_cast<int>(newwidth), static_cast<int>(newheight)));
                CRect croppedRect;
                croppedRect.IntersectRect(CRect(0, 0, int(newwidth), int(newheight)), destRect);
                CRect sourceRect = croppedRect;
                sourceRect.OffsetRect(/*(destRect.left < 0)?*/ -destRect.left, /*(destRect.top < 0)?*/ -destRect.top);
                sourceRect.left = int(sourceRect.left / k);
                sourceRect.top = int(sourceRect.top / k);
                sourceRect.right = int(sourceRect.right / k);
                sourceRect.bottom = int(sourceRect.bottom / k);
                // = sourceRect.MulDiv(1, k);
                gr.DrawImage(bm,
                    RectF(float(croppedRect.left), float(croppedRect.top), float(croppedRect.Width()),
                    float(croppedRect.Height()))
                    , float(sourceRect.left), float(sourceRect.top), float(sourceRect.Width()),
                    float(sourceRect.Height()), UnitPixel,
                    &attr);
            }
        RectF bounds(0, 0, float(newwidth), float(newheight));

        // Добавляем текст на картинку (если опция включена)
        if (m_imageConvertingParams.AddText)
        {
            SolidBrush brush(Color(GetRValue(m_imageConvertingParams.TextColor), GetGValue(
                m_imageConvertingParams.TextColor), GetBValue(m_imageConvertingParams.TextColor)));

            int HAlign[6] = { 0, 1, 2, 0, 1, 2 };
            int VAlign[6] = { 0, 0, 0, 2, 2, 2 };

            //m_imageConvertingParams.Font.lfQuality = m_imageConvertingParams.Font.lfQuality | ANTIALIASED_QUALITY;
            auto font = ImageUtils::StringToGdiplusFont(U2W(m_imageConvertingParams.Font));

            SolidBrush brush2(Color(70, 0, 0, 0));
            RectF bounds2(1, 1, float(newwidth), float(newheight) + 1);
            ImageUtils::DrawStrokedText(gr, U2W(m_imageConvertingParams.Text), bounds2, *font.get(), MYRGB(255,
                m_imageConvertingParams.TextColor),
                MYRGB(180,
                m_imageConvertingParams.StrokeColor), HAlign[m_imageConvertingParams.TextPosition],
                VAlign[m_imageConvertingParams.TextPosition], 1);
        }

        if (m_imageConvertingParams.AddLogo)
        {
            Bitmap logo(U2W(m_imageConvertingParams.LogoFileName));
            if (logo.GetLastStatus() == Ok)
            {
                int x, y;
                int logowidth, logoheight;
                logowidth = logo.GetWidth();
                logoheight = logo.GetHeight();
                if (m_imageConvertingParams.LogoPosition < 3)
                    y = 0;
                else
                    y = int(newheight - logoheight);
                if (m_imageConvertingParams.LogoPosition == 0 || m_imageConvertingParams.LogoPosition == 3)
                    x = 0;
                if (m_imageConvertingParams.LogoPosition == 2 || m_imageConvertingParams.LogoPosition == 5)
                    x = int(newwidth - logowidth);
                if (m_imageConvertingParams.LogoPosition == 1 || m_imageConvertingParams.LogoPosition == 4)
                    x = int((newwidth - logowidth) / 2);

                gr.DrawImage(&logo, x, y, logowidth, logoheight);
            }
        }
        thumbSource = BackBuffer.get();
        if (m_imageConvertingParams.PreserveExifInformation) {
            for (UINT k = 0; k < propertyItemsCount; k++) {
                if (pPropBuffer[k].id != PropertyTagOrientation) { // Do not preserve orientation
                    BackBuffer->SetPropertyItem(pPropBuffer + k);
                }
            }
        }
        free(pPropBuffer);
        CString resultFileName;
        try {
            ImageUtils::MySaveImage(
                BackBuffer.get(),
                IuCommonFunctions::GenerateFileName(L"img%md5.jpg", 1, CPoint(), time(nullptr), _T("")), resultFileName,
                fileformat, m_imageConvertingParams.Quality);
            resultFileName_ = W2U(resultFileName);
        }
        catch (const std::exception& ex) {
            LOG(ERROR) << ex.what();
        }
     //   imageFile = resultFileName;

    }

    if (!processingEnabled_)
    {
        CString Ext = WinUtils::GetFileExt(sourceFileW);
        if (Ext == _T("png"))
            fileformat = ImageUtils::sifPNG;
        else
            fileformat = ImageUtils::sifJPEG;
    }
    if (generateThumb_)
    {
        // Генерирование превьюшки с шаблоном в отдельной функции
        ImageUtils::SaveImageFormat thumbFormat = fileformat;
        if (m_thumbCreatingParams.Format == ThumbCreatingParams::tfJPEG) {
            thumbFormat = ImageUtils::sifJPEG;
        } else if (m_thumbCreatingParams.Format == ThumbCreatingParams::tfPNG) {
            thumbFormat = ImageUtils::sifPNG;
        } else if (m_thumbCreatingParams.Format == ThumbCreatingParams::tfGIF) {
            thumbFormat = ImageUtils::sifGIF;
        } else if (m_thumbCreatingParams.Format == ThumbCreatingParams::tfWebP) {
            thumbFormat = ImageUtils::sifWebp;
        } else if (m_thumbCreatingParams.Format == ThumbCreatingParams::tfWebPLossless) {
            thumbFormat = ImageUtils::sifWebpLossless;
        }
        createThumb(thumbSource, imageFile, thumbFormat);
    }
    return true;
}

bool ImageConverterPrivate::createThumb(Gdiplus::Bitmap* bm, const CString& imageFile, ImageUtils::SaveImageFormat fileformat)
{
    bool result = false;
    int64_t FileSize = IuCoreUtils::GetFileSize(W2U(imageFile));

    // Saving thumbnail (without template)
    GdiPlusImage src(bm, false);
    std::unique_ptr<AbstractImage> thumb = createThumbnail(&src, FileSize, fileformat);
    auto res = static_cast<GdiPlusImage*>(thumb.get());
    if (res) {
        CString thumbFileName;
        try {
            result = ImageUtils::MySaveImage(res->getBitmap(), IuCommonFunctions::GenerateFileName(L"thumb_%md5.jpg", 1,
                CPoint(), time(nullptr), _T("")), thumbFileName, fileformat, m_thumbCreatingParams.Quality);
            thumbFileName_ = W2U(thumbFileName);
        } catch (const std::exception& ex) {
            LOG(ERROR) << ex.what();
        }
    }

    return result;
}

std::unique_ptr<AbstractImage> ImageConverterPrivate::createThumbnail(AbstractImage* abstractImg, int64_t fileSize, int fileformat)
{
    GdiPlusImage* image = dynamic_cast<GdiPlusImage*>(abstractImg);
    assert(image);
    assert(thumbnailTemplate_);

    if (!image) {
        return {};
    }
    const Thumbnail::ThumbnailData* data = thumbnailTemplate_->getData();
    CWindowDC dc(nullptr);
    int newwidth = image->getWidth();
    int newheight = image->getHeight();

    CString sizeString = U2W(IuCoreUtils::FileSizeToString(fileSize));
    CString ThumbnailText = U2W(m_thumbCreatingParams.Text); // Text that will be drawn on thumbnail

    ThumbnailText.Replace(_T("%width%"), WinUtils::IntToStr(newwidth)); // Replacing variables names with their values
    ThumbnailText.Replace(_T("%height%"), WinUtils::IntToStr(newheight));
    ThumbnailText.Replace(_T("%size%"), sizeString);

    int thumbwidth = m_thumbCreatingParams.Width;
    int thumbheight = m_thumbCreatingParams.Height;

    Graphics g1(dc);
    CString filePath = U2W(thumbnailTemplate_->getSpriteFileName());
    Image templ(filePath);

    std::unique_ptr<Gdiplus::Font> font;
    
    if (thumbnailTemplate_->existsParam("Font")) {
        std::string fontStr = thumbnailTemplate_->getParamString("Font");
        CString wide_text = U2W(fontStr);
        font = ImageUtils::StringToGdiplusFont(wide_text);
        //WinUtils::StringToFont(wide_text, &lf);
    } else {
        //WinUtils::StringToFont(_T("Tahoma,7,b,204"), &lf);
        font = ImageUtils::StringToGdiplusFont(_T("Tahoma,11,b,204"));
    }

    RectF TextRect;

    /*StringFormat * f = StringFormat::GenericTypographic()->Clone();
    f->SetTrimming(StringTrimmingNone);
    f->SetFormatFlags( StringFormatFlagsMeasureTrailingSpaces);*/

    StringFormat format;
    FontFamily ff;
    font->GetFamily(&ff);
    g1.SetPageUnit(UnitWorld);
    g1.MeasureString(_T("test"), -1, font.get(), PointF(0, 0), &format, &TextRect);
    //        delete f;
    m_Vars["TextWidth"] = std::to_string(static_cast<int>(TextRect.Width));
    m_Vars["TextHeight"] = std::to_string(static_cast<int>(TextRect.Height));
    m_Vars["UserText"] = W2U(ThumbnailText);
    std::string textTempl = thumbnailTemplate_->getParamString("Text");
    if (textTempl.empty())
        textTempl = "$(UserText)";
    CString textToDraw = U2W(ReplaceVars(textTempl));

    for (std::map<std::string, std::string>::const_iterator it = data->colors_.begin(); it != data->colors_.end(); ++it)
    {
        m_Vars[it->first] = it->second;
    }

    m_Vars["DrawText"] = std::to_string(m_Vars["DrawText"] == "1" && m_thumbCreatingParams.AddImageSize);

    if (m_thumbCreatingParams.ResizeMode == ThumbCreatingParams::trByWidth)
    {
        thumbwidth -= EvaluateExpression(thumbnailTemplate_->getWidthAddition());
        if (thumbwidth < 10)
            thumbwidth = 10;
        thumbheight = int(round((float)thumbwidth / (float)newwidth * newheight));
    }
    else if (m_thumbCreatingParams.ResizeMode == ThumbCreatingParams::trByHeight)
    {
        thumbheight -= EvaluateExpression(thumbnailTemplate_->getHeightAddition());
        if (thumbheight < 10)
            thumbheight = 10;
        thumbwidth = int(round((float)thumbheight / (float)newheight * newwidth));
    } else if (m_thumbCreatingParams.ResizeMode == ThumbCreatingParams::trByBoth) {
        thumbwidth -= EvaluateExpression(thumbnailTemplate_->getWidthAddition());
        thumbheight -= EvaluateExpression(thumbnailTemplate_->getHeightAddition());
    }

    int RealThumbWidth = thumbwidth + EvaluateExpression(thumbnailTemplate_->getWidthAddition());
    int RealThumbHeight = thumbheight + EvaluateExpression(thumbnailTemplate_->getHeightAddition());

    m_Vars["Width"] = std::to_string(RealThumbWidth);
    m_Vars["Height"] = std::to_string(RealThumbHeight);
    auto ThumbBuffer = std::make_unique<Bitmap>(RealThumbWidth, RealThumbHeight, &g1);
    Graphics thumbgr(ThumbBuffer.get());
    thumbgr.Clear(Gdiplus::Color::Transparent);
    thumbgr.SetPageUnit(UnitPixel);

    if (fileformat == 0 || fileformat == 2)
        thumbgr.Clear(MYRGB(255, m_thumbCreatingParams.BackgroundColor));
    // thumbgr.Clear(Color(255,255,255,255));
    RectF thu((m_thumbCreatingParams.DrawFrame ? 1.0f : 0.f), (m_thumbCreatingParams.DrawFrame ? 1.0f : 0.f),
        static_cast<float>(thumbwidth),
        static_cast<float>(thumbheight));
    thumbgr.SetInterpolationMode(InterpolationModeHighQualityBicubic);
    // thumbgr.SetPixelOffsetMode(PixelOffsetModeHighQuality );
    // thumbgr.SetSmoothingMode( SmoothingModeHighQuality);
    // //    thumbgr.SetSmoothingMode(SmoothingModeAntiAlias);
    thumbgr.SetSmoothingMode(SmoothingModeNone);
    // thumbgr.SetPixelOffsetMode(PixelOffsetModeHighQuality );
    // thumbgr.SetCompositingQuality(CompositingQualityHighQuality);

    Bitmap MaskBuffer(RealThumbWidth, RealThumbHeight, PixelFormat32bppARGB);
    Graphics maskgr(&MaskBuffer);

    for (size_t i = 0; i < data->drawing_operations_.size(); i++)
    {
        bool performOperation = true;
        if (!data->drawing_operations_[i].condition.empty())
        {
            performOperation = EvaluateExpression(data->drawing_operations_[i].condition) != 0;
        }
        if (!performOperation)
            continue;
        Graphics* gr = &thumbgr;
        std::string type = data->drawing_operations_[i].type;
        CRect rc;
        EvaluateRect(data->drawing_operations_[i].rect, &rc);
        if (data->drawing_operations_[i].destination == "mask")
        {
            gr = &maskgr;
        }
        if (type == "text")
        {
            std::string colorsStr = data->drawing_operations_[i].text_colors;
            std::vector<std::string> tokens;
            IuStringUtils::Split(colorsStr, " ", tokens, 2);
            unsigned int color1 = 0xB3FFFFFF;
            unsigned int strokeColor = 0x5A000000;
            if (tokens.size() > 0)
                color1 = EvaluateColor(tokens[0]);
            if (tokens.size() > 1)
                strokeColor = EvaluateColor(tokens[1]);
            RectF TextBounds((float)rc.left, (float)rc.top, (float)rc.right, (float)rc.bottom);
            thumbgr.SetPixelOffsetMode(PixelOffsetModeDefault);
            ImageUtils::DrawStrokedText(*gr, /* Buffer*/ textToDraw, TextBounds, *font.get(), Color(color1), Color(
                strokeColor) /*params.StrokeColor*/, 1, 1, 1);
            thumbgr.SetPixelOffsetMode(PixelOffsetModeNone);
        }
        else
            if (type == "fillrect")
            {
                std::unique_ptr<Gdiplus::Brush> fillBr = CreateBrushFromString(data->drawing_operations_[i].brush, rc);
                gr->FillRectangle(fillBr.get(), rc.left, rc.top, rc.right, rc.bottom);
            }
            else
                if (type == "drawrect")
                {
                    GraphicsPath path;
                    // path.AddRectangle(
                    std::vector<std::string> tokens;
                    IuStringUtils::Split(data->drawing_operations_[i].pen, " ", tokens, 2);
                    if (tokens.size() > 1)
                    {
                        unsigned int color = EvaluateColor(tokens[0]);
                        int size = EvaluateExpression(tokens[1]);
                        Gdiplus::Pen p(color, float(size));
                        if (size == 1)
                        {
                            rc.right--;
                            rc.bottom--;
                        }
                        p.SetAlignment(PenAlignmentInset);
                        gr->DrawRectangle(&p, rc.left, rc.top, rc.right, rc.bottom);
                    }
                }
                else
                    if (type == "blt")
                    {
                        RECT sourceRect;
                        EvaluateRect(data->drawing_operations_[i].source_rect, &sourceRect);
                        Rect t(int(rc.left), int(rc.top), int(rc.right), int(rc.bottom));

                        if (data->drawing_operations_[i].source == "image")
                        {
                            SolidBrush whiteBr(Color(160, 130, 100));
                            // thumbgr.FillRectangle(&whiteBr, (int)t.X, (int) t.Y/*(int)item->DrawFrame?1:0-1*/,thumbwidth,thumbheight);
                            // thumbgr.SetInterpolationMode(InterpolationModeNearestNeighbor  );
                            ImageAttributes attr;
                            attr.SetWrapMode(WrapModeTileFlipXY);
                            Rect dest(t.X, t.Y, thumbwidth, thumbheight);

                            Bitmap tempImage(RealThumbWidth, RealThumbHeight, PixelFormat32bppARGB);
                            Graphics tempGr(&tempImage);
                            tempGr.SetInterpolationMode(InterpolationModeHighQualityBicubic);
                            tempGr.SetSmoothingMode(SmoothingModeHighQuality);
                            tempGr.SetPixelOffsetMode(PixelOffsetModeHighQuality);

                            if (m_thumbCreatingParams.ResizeMode == ThumbCreatingParams::trByBoth) {
                                CRect srcRect, destRect;
                                CRect dRect(t.X, t.Y, t.X + thumbwidth, t.Y + thumbheight);
                                calcCropSize(image->getWidth(), image->getHeight(), dRect, destRect, srcRect);
                                // = sourceRect.MulDiv(1, k);
                                SolidBrush br(Color(255, 255, 255));
                                tempGr.FillRectangle(&br, dRect.left, dRect.top, dRect.Width(), dRect.Height());
                                tempGr.DrawImage(image->getBitmap(),
                                    RectF(float(destRect.left), float(destRect.top), float(destRect.Width()),
                                        float(destRect.Height()))
                                    , float(srcRect.left), float(srcRect.top), float(srcRect.Width()),
                                    float(srcRect.Height()), UnitPixel,
                                    &attr);
                            } else {
                                tempGr.DrawImage(image->getBitmap(), dest, (INT)0, (int)0, (int)image->getWidth(),
                                    (int)image->getHeight(), UnitPixel, &attr);
                            }


                            ImageUtils::ChangeAlphaChannel(MaskBuffer, tempImage, 3, 3);
                            gr->DrawImage(&tempImage, 0, 0);
                            tempGr.SetSmoothingMode(SmoothingModeNone);
                        }
                        else
                        {
                            ImageAttributes attr;
                            attr.SetWrapMode(WrapModeClamp);    // we need this to avoid some glitches on the edges of interpolated image
                            thumbgr.SetInterpolationMode(InterpolationModeHighQualityBicubic);
                            thumbgr.SetSmoothingMode(SmoothingModeHighQuality);
                            gr->DrawImage(&templ, t, static_cast<int>(sourceRect.left), static_cast<int>(sourceRect.top), static_cast<int>(sourceRect.right),
                                static_cast<int>(sourceRect.bottom), UnitPixel,
                                &attr);
                            thumbgr.SetSmoothingMode(SmoothingModeNone);
                        }
                    }
    }

    return std::make_unique<GdiPlusImage>(ThumbBuffer.release());
}

bool ImageConverterPrivate::EvaluateRect(const std::string& rectStr, RECT* out)
{
    std::vector<std::string> values;
    IuStringUtils::Split(rectStr, ";", values, 4);
    if (values.size() != 4)
        return false;
    int* target = reinterpret_cast<int*>(out);
    for (size_t i = 0; i < values.size(); i++)
    {
        int coord = EvaluateExpression(IuStringUtils::Trim(values[i]));
        *target = coord;
        target++;
    }
    return true;
}

std::unique_ptr<Gdiplus::Brush> ImageConverterPrivate::CreateBrushFromString(const std::string& brStr, const RECT& rect) {
    std::vector<std::string> tokens;
    IuStringUtils::Split(brStr, ":", tokens, 10);
    if (tokens[0] == "solid") {
        uint32_t color = 0;
        if (tokens.size() >= 2) {
            color = EvaluateColor(tokens[1]);
        }
        return std::make_unique<SolidBrush>(Color(color));
    }
    else if (tokens[0] == "gradient" && tokens.size() > 1) {
        std::vector<std::string> params;
        IuStringUtils::Split(tokens[1], " ", params, 10);
        if (params.size() >= 3) {
            unsigned int color1 = EvaluateColor(params[0]);
            unsigned int color2 = EvaluateColor(params[1]);
            int type = -1;
            std::string gradType = params[2];
            if (gradType == "vert") {
                type = LinearGradientModeVertical;
            }
            else if (gradType == "hor") {
                type = LinearGradientModeHorizontal;
            }
            else if (gradType == "diag1") {
                type = LinearGradientModeForwardDiagonal;
            }
            else if (gradType == "diag2") {
                type = LinearGradientModeBackwardDiagonal;
            }
            // return new LinearGradientBrush(Rect(0,0, /*rect.left+*/rect.right , /*rect.top+*/rect.bottom ), Color(color1), Color(color2), LinearGradientMode(type));
            return std::make_unique<LinearGradientBrush>(RectF(float(rect.left), float(-0.5 + rect.top), float(rect.right),
                                                 /*rect.top+*/ float(rect.bottom)), Color(color1), Color(
                                               color2), LinearGradientMode(type));
        }
    }
    return std::make_unique<SolidBrush>(0);
}


bool ImageConverterPrivate::supposedOutputFormat(SupposedFormat& file){
    if (!processingEnabled_) {
        return false;
    }

    CString resultFileName;
    ImageUtils::SaveImageFormat fileformat;
    if (m_imageConvertingParams.Format < 1 || !processingEnabled_)
        fileformat = ImageUtils::GetFormatByFileName(U2W(file.fileName));
    else
        fileformat = static_cast<ImageUtils::SaveImageFormat>(m_imageConvertingParams.Format - 1);

    // We don't actually save anything, we just ask for the output file name.
    if (ImageUtils::MySaveImage(nullptr,
                                IuCommonFunctions::GenerateFileName(L"img%md5.jpg", 1, CPoint(), time(nullptr), _T("")),
                                resultFileName, fileformat, m_imageConvertingParams.Quality)) {
        file.fileName = W2U(resultFileName);
        file.mimeType = IuCoreUtils::GetFileMimeTypeByName(file.fileName);
        file.fileSize = 0;
        return true;
    }
    return false;
}

void ImageConverterPrivate::calcCropSize(int srcWidth, int srcHeight, CRect targetRect, CRect& destRect, CRect& srcRect) {
    float imgwidth = static_cast<float>(srcWidth);
    float imgheight = static_cast<float>(srcHeight);

    int thumbwidth = targetRect.Width();
    int thumbheight = targetRect.Height();
    
    float ratio = std::min<float>((float)thumbwidth / imgwidth, (float)thumbheight / imgheight);

    CSize sizeDrawingImage;
    sizeDrawingImage.cx = static_cast<LONG>(round(imgwidth * ratio));
    sizeDrawingImage.cy = static_cast<LONG>(round(imgheight * ratio));

    CRect rectDrawing(CPoint(0, 0), sizeDrawingImage);
    rectDrawing.OffsetRect(targetRect.left + (thumbwidth - sizeDrawingImage.cx) / 2, targetRect.top + (thumbheight - sizeDrawingImage.cy) / 2);

    destRect = rectDrawing;
    srcRect = CRect(0,0, srcWidth, srcHeight);
}
