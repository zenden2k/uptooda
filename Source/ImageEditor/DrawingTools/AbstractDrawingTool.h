#ifndef AbstractDrawingTool_h_
#define AbstractDrawingTool_h_

#include "3rdpart/GdiplusH.h"
#include "../DrawingElement.h"
#include "../MovableElement.h"

namespace ImageEditor {

class Canvas;

class AbstractDrawingTool {
public:
    AbstractDrawingTool(Canvas* canvas);
    virtual ~AbstractDrawingTool() = default;
    virtual void beginDraw(int x, int y);
    virtual void continueDraw(int x, int y, DWORD flags) = 0;
    virtual void endDraw(int x, int y);
    virtual void render(Painter* gr) = 0;
    virtual void mouseDoubleClick(int x, int y);
    virtual CursorType getCursor(int x, int y);
    virtual void rightButtonClick(int x, int y);
    virtual void setPenSize(int size);
    virtual void setRoundingRadius(int radius);
    virtual bool applyOperation();
    virtual void cancelOperation();
    void setForegroundColor(Gdiplus::Color color);
    void setBackgroundColor(Gdiplus::Color color);

protected:
    Canvas* canvas_;
    POINT startPoint_;
    POINT endPoint_;
    int penSize_;
    int roundingRadius_;
    Gdiplus::Color foregroundColor_;
    Gdiplus::Color backgroundColor_;
};

}
#endif // AbstractDrawingTool_h_
