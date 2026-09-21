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

#include "Document.h"

#include <cassert>
#include <utility>

#include "3rdpart/GdiplusH.h"
#include "DrawingElement.h"
#include "Core/Logging.h"
#include "Core/Images/Utils.h"

namespace ImageEditor {
using namespace Gdiplus;

Document::Document(int width, int height) {
    hasTransparentPixels_ = false;
    currentImage_ = std::make_shared<Gdiplus::Bitmap>(width, height, PixelFormat32bppARGB);
    init(true);
}

Document::Document(const wchar_t* fileName) {
    hasTransparentPixels_ = false;
    currentImage_ = ImageUtils::LoadImageFromFileWithoutLocking(fileName, &isSrcMultiFrame_);
    init();
    if (currentImage_) {
        checkTransparentPixels();
    }
}

Document::Document(std::shared_ptr<Gdiplus::Bitmap> sourceImage, bool hasTransparentPixels) {
    currentImage_ = std::move(sourceImage);
    hasTransparentPixels_ = hasTransparentPixels;
    init();
}

Document::~Document() {
    for (auto& i : history_) {
        delete[] i.data;
    }
}

void Document::init(bool clear) {
    currentPainter_.reset();
    drawStarted_ = false;
    originalImage_ = nullptr;
    if (currentImage_) {
        currentPainter_ = std::make_unique<Graphics>(currentImage_.get());
        if (clear) {
            currentPainter_->Clear(Color::Transparent);
        }
        changedSegments_ = AffectedSegments(getWidth(), getHeight());
    }
}

void Document::beginDrawing(bool cloneImage) {
    drawStarted_ = true;
    if (cloneImage) {
        originalImage_ = currentImage_->Clone(0, 0, currentImage_->GetWidth(), currentImage_->GetHeight(),
                                              PixelFormat32bppARGB);
    }
    changedSegments_.clear();
}

void Document::addDrawingElement(DrawingElement* element) {
    currentPainter_->SetSmoothingMode(Gdiplus::SmoothingModeAntiAlias);
    currentPainter_->SetInterpolationMode(Gdiplus::InterpolationModeHighQualityBicubic);
    AffectedSegments segments;
    element->getAffectedSegments(&segments);
    changedSegments_ += segments;
    if (!drawStarted_) {
        saveDocumentState();
        changedSegments_.clear();
    }
    element->render(currentPainter_.get());
}

void Document::endDrawing() {
    // save document state
    saveDocumentState();
    drawStarted_ = false;
    changedSegments_.clear();
}

void Document::addAffectedSegments(const AffectedSegments& segments) {
    changedSegments_ += segments;
}

Gdiplus::Bitmap* Document::getBitmap() const {
    return currentImage_.get();
}

void Document::updateBitmap(std::shared_ptr<Gdiplus::Bitmap> bm) {
    saveDocumentState(true);
    currentImage_ = std::move(bm);
    init();
}

void Document::saveDocumentState(bool full) {
    int pixelSize = 4;
    std::deque<RECT> rects;
    Gdiplus::Bitmap* srcImage = (originalImage_) ? originalImage_ : currentImage_.get();
    int srcImageWidth = srcImage->GetWidth();
    int srcImageHeight = srcImage->GetHeight();

    changedSegments_.getRects(rects, srcImageWidth, srcImageHeight); // may contain invalid segments!
    unsigned int pixels = 0;

    pixels = rects.size() * AffectedSegments::kSegmentSize * AffectedSegments::kSegmentSize * pixelSize;

    unsigned int dataSize = pixels * pixelSize;
    unsigned char* imageData = nullptr;

    if (!full) {
        imageData = new unsigned char[dataSize];
        Gdiplus::BitmapData bdSrc;
        Gdiplus::Rect r(0, 0, currentImage_->GetWidth(), currentImage_->GetHeight());

        if (srcImage->LockBits(&r, ImageLockModeRead, PixelFormat32bppARGB, &bdSrc) != Gdiplus::Ok) {
            delete[] imageData;
            return;
        }

        BYTE* bpSrc = static_cast<BYTE*>(bdSrc.Scan0);

        unsigned char* pImageData = imageData;

        {
            AffectedSegments outSegments(srcImageWidth, srcImageHeight);

            for (auto it = rects.begin(); it != rects.end(); ++it) {
                int x = it->left;
                int y = it->top;
                if (x < 0 || y < 0) {
                    continue;
                }
                int rectWidth = std::min<int>(it->right - it->left, srcImageWidth - x);
                int rectHeight = std::min<int>(it->bottom - it->top, srcImageHeight - y);
                if (rectWidth <= 0 || rectHeight <= 0) {
                    // invalid rectangle. Out of bounds;
                    continue;
                }
                outSegments.markRect(x, y, rectWidth, rectHeight);
                for (int j = 0; j < rectHeight; j++) {
                    unsigned int dataOffset = (r.Width * (y + j) + x) * pixelSize;
                    unsigned int rowSize = rectWidth * pixelSize;
                    memcpy(pImageData, bpSrc + dataOffset, rowSize);
                    //memset( pImageData, 255, rowSize );
                    pImageData += rowSize;
                }
            }
        }
        currentImage_->UnlockBits(&bdSrc);
    }

    HistoryItem item;
    item.data = imageData;
    item.segments = changedSegments_;
    item.size = dataSize;
    item.full = full;
    if (full) {
        item.width = srcImageWidth;
        item.height = srcImageHeight;
        item.bmp = currentImage_;
    }
    history_.push_back(item);
    delete originalImage_;
    originalImage_ = 0;
    changedSegments_.clear();
}

void Document::checkTransparentPixels() {
    using namespace Gdiplus;
    BitmapData bitmapData;
    Rect lockRect(0, 0, std::min<int>(10, currentImage_->GetWidth()), std::min<int>(10, currentImage_->GetHeight()));
    if (currentImage_->LockBits(&lockRect, ImageLockModeRead, PixelFormat32bppARGB, &bitmapData) == Ok) {
        auto* source = static_cast<uint8_t*>(bitmapData.Scan0);
        unsigned int stride;
        if (bitmapData.Stride > 0) {
            stride = bitmapData.Stride;
        } else {
            stride = -bitmapData.Stride;
        }
        for (int i = 0; i < lockRect.Height; i++) {
            for (int j = 0; j < lockRect.Width; j++) {
                if (source[i * stride + j * 4 + 3] != 255) {
                    hasTransparentPixels_ = true;
                    currentImage_->UnlockBits(&bitmapData);
                    return;
                }
            }
        }
        currentImage_->UnlockBits(&bitmapData);
    }
}

void Document::render(Painter* gr, Gdiplus::Rect rc) {
    if (!gr || !currentImage_) return;
    gr->DrawImage(currentImage_.get(), rc.X, rc.Y, rc.X, rc.Y, rc.Width, rc.Height, Gdiplus::UnitPixel);
}

bool Document::undo() {
    if (history_.empty()) {
        return false;
    }
    HistoryItem undoItem = history_.back();
    history_.pop_back();
    std::deque<RECT> rects;
    auto image = currentImage_;
    undoItem.segments.getRects(rects, image->GetWidth(), image->GetHeight());

    Gdiplus::BitmapData bdSrc;
    Gdiplus::Rect r(0, 0, image->GetWidth(), image->GetHeight());
    if (image->LockBits(&r, ImageLockModeWrite, PixelFormat32bppARGB, &bdSrc) != Gdiplus::Ok) {
        return false;
    }
    BYTE* bpSrc = static_cast<BYTE*>(bdSrc.Scan0);
    unsigned char* pdata = undoItem.data;
    int pixelSize = 4;

    if (undoItem.full) {
        /*std::shared_ptr<Gdiplus::Bitmap> newBitmap = std::make_shared<Gdiplus::Bitmap>(undoItem.width, undoItem.height);
        memcpy(bpSrc, pdata, undoItem.size);*/
        currentImage_ = undoItem.bmp;
        init();
    } else {
        for (auto it = rects.begin(); it != rects.end(); ++it) {
            int x = it->left;
            int y = it->top;
            int rectWidth = it->right - it->left;
            int rectHeight = it->bottom - it->top;

            for (int j = 0; j < rectHeight; j++) {
                unsigned int dstDataOffset = (r.Width * (y + j) + x) * pixelSize;
                unsigned int rowSize = rectWidth * pixelSize;
                memcpy(bpSrc + dstDataOffset, pdata, rowSize);
                //memset( bpSrc + dstDataOffset, 255, rowSize );
                pdata += rowSize;
            }
        }
    }

    delete[] undoItem.data;
    image->UnlockBits(&bdSrc);
    return true;
}

int Document::getWidth() const {
    return currentImage_->GetWidth();
}

int Document::getHeight() const {
    return currentImage_->GetHeight();
}

bool Document::isNull() const {
    return !currentImage_;
}

bool Document::hasTransparentPixels() const {
    return hasTransparentPixels_;
}

Painter* Document::getGraphicsObject() const {
    return currentPainter_.get();
}

void Document::applyCrop(int cropX, int cropY, int cropWidth, int cropHeight) {
    saveDocumentState(true);
    auto newBitmap = std::make_shared<Gdiplus::Bitmap>(cropWidth, cropHeight);
    Gdiplus::Graphics gr(newBitmap.get());
    gr.Clear(Gdiplus::Color::Transparent);
    gr.DrawImage(currentImage_.get(), 0, 0, cropX, cropY, cropWidth, cropHeight, UnitPixel);
    currentImage_ = newBitmap;
}

bool Document::isSrcMultiFrame() const {
    return isSrcMultiFrame_;
}

bool Document::isInDrawingState() const {
    return drawStarted_;
}

}
