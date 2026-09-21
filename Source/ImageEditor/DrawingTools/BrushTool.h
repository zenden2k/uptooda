#ifndef BrushTool_h__
#define BrushTool_h__

#include "3rdpart/GdiplusH.h"
#include "../DrawingElement.h"
#include "../MovableElement.h"
#include "AbstractDrawingTool.h"

namespace ImageEditor {

class Canvas;

class BrushTool: public AbstractDrawingTool  {
public:
    explicit BrushTool( Canvas* canvas );
    void beginDraw( int x, int y ) override;
    void continueDraw( int x, int y, DWORD flags) override;
    void endDraw( int x, int y ) override;
    void render( Painter* gr ) override;
    CursorType getCursor(int x, int y) override;
protected:
    POINT oldPoint_;
    AffectedSegments segments_;
    Gdiplus::Region affectedRegion_;
    virtual void drawLine(int x0, int y0, int x1, int y1);
};

}

#endif // BrushTool_h__
