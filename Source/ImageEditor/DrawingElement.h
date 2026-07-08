#ifndef IMAGEEDITOR_DRAWINGELEMENT_H
#define IMAGEEDITOR_DRAWINGELEMENT_H

#include <map>
#include <deque>
#include "3rdpart/GdiplusH.h"
#include "BasicTypes.h"
#include "Core/Utils/CoreTypes.h"

namespace ImageEditor {

class Canvas;

class AffectedSegments;

class DrawingElement {
    public:
        DrawingElement();
        virtual ~DrawingElement() = default;
        virtual void render(Painter* gr) = 0;
        virtual void resize(int width, int height);
        virtual void setStartPoint(POINT startPoint);
        virtual void setEndPoint(POINT endPoint);
        POINT getStartPoint() const;
        POINT getEndPoint() const;
        virtual void setColor( Gdiplus::Color color );
        virtual void setBackgroundColor( Gdiplus::Color color );
        Gdiplus::Color getColor() const;
        Gdiplus::Color getBackgroundColor() const;

        void setCanvas( Canvas* canvas );
        virtual void getAffectedSegments( AffectedSegments* segments );
        void setPenSize( int penSize );
        void setRoundingRadius(int radius);
        int getWidth() const;
        int getHeight() const;
    protected:
        POINT startPoint_{};
        POINT endPoint_{};
        Gdiplus::Color color_;
        Gdiplus::Color backgroundColor_;
        int penSize_;
        Canvas* canvas_;
        int roundingRadius_;
        DISALLOW_COPY_AND_ASSIGN(DrawingElement);
};

class AffectedSegments {
    public:
        AffectedSegments();
        AffectedSegments(int maxWidth, int maxHeight);
        enum { kSegmentSize = 50 };
        void markPoint(int x, int y);
        void markRect(int x, int y, int width, int height);
        void markRect(RECT rc);
        HRGN createRegionFromSegments();
        void clear();
        void getRects( std::deque<RECT>& rects, int maxWidth = 0, int maxHeight = 0) const;
        AffectedSegments& operator+= ( const AffectedSegments& segments_);
    private:
        std::map<unsigned int, bool> segments_;
        int maxWidth_;
        int maxHeight_;
};

}

#endif