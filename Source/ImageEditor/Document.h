#ifndef IMAGEEDITOR_DOCUMENT_H
#define IMAGEEDITOR_DOCUMENT_H

#include <memory>
#include <vector>

#include <Windows.h>
#include "3rdpart/GdiPlusH.h"
#include "BasicTypes.h"
#include "DrawingElement.h"
#include "Core/Utils/CoreTypes.h"

namespace ImageEditor {

class DrawingElement;

class Document {
    public:

        struct HistoryItem {
            unsigned char *data = nullptr;
            unsigned int size = 0;
            AffectedSegments segments;
            bool full = false;
            int width = 0;
            int height = 0;
            std::shared_ptr<Gdiplus::Bitmap> bmp;
        };
        Document(int width, int height);
        explicit Document(const wchar_t* fileName);
        explicit Document(std::shared_ptr<Gdiplus::Bitmap> sourceImage, bool hasTransparentPixels = false);
        virtual ~Document();
        
        Painter* getGraphicsObject() const;
        void beginDrawing( bool cloneImage = true );
        void addDrawingElement(DrawingElement *element);
        void endDrawing();

        void addAffectedSegments(const AffectedSegments& segments);
        void applyCrop(int cropX, int cropY, int cropWidth, int cropHeight);
        Gdiplus::Bitmap* getBitmap() const;
        void updateBitmap(std::shared_ptr<Gdiplus::Bitmap> bm);
        void render(Painter* gr, Gdiplus::Rect rc);
        bool undo();
        int getWidth() const;
        int getHeight() const;
        bool isNull() const;
        bool hasTransparentPixels() const;
        bool isSrcMultiFrame() const;
        bool isInDrawingState() const;
    private:
        std::shared_ptr<Gdiplus::Bitmap> currentImage_;
        Gdiplus::Bitmap* originalImage_{};
        std::unique_ptr<Painter> currentPainter_;
        std::vector<HistoryItem> history_;
        bool drawStarted_ = false;
        AffectedSegments changedSegments_;
        bool hasTransparentPixels_;
        bool isSrcMultiFrame_ = false;
        void init(bool clear = false);
        void saveDocumentState(bool full = false);
        void checkTransparentPixels();
        
        DISALLOW_COPY_AND_ASSIGN(Document);
};

}

#endif
