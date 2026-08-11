#ifndef PenTool_h_
#define PenTool_h_

#include "3rdpart/GdiplusH.h"
#include "../DrawingElement.h"
#include "../MovableElement.h"
#include "AbstractDrawingTool.h"

namespace ImageEditor {
class Canvas;

class PenTool : public AbstractDrawingTool {
public:
    explicit PenTool(Canvas* canvas);
    void beginDraw(int x, int y) override;
    void continueDraw(int x, int y, DWORD flags) override;
    void endDraw(int x, int y) override;
    void render(Painter* gr) override;
    CursorType getCursor(int x, int y) override;

private:
    POINT oldPoint_{};
};
}

#endif // PenTool_h_
