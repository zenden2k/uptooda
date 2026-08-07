#ifndef TextTool_h_
#define TextTool_h_

#include "3rdpart/GdiplusH.h"
#include "../DrawingElement.h"
#include "../MovableElement.h"
#include "MoveAndResizeTool.h"

namespace ImageEditor {
class Canvas;

class TextTool : public MoveAndResizeTool {
public:
    explicit TextTool(Canvas* canvas);
    void beginDraw(int x, int y) override;
    void continueDraw(int x, int y, DWORD flags) override;
    void endDraw(int x, int y) override;
    void render(Painter* gr) override;
    CursorType getCursor(int x, int y) override;

private:
};
}
#endif // TextTool_h_
