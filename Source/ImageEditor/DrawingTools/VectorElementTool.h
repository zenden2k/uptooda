#ifndef VectorElementTool_h_
#define VectorElementTool_h_

#include "3rdpart/GdiplusH.h"
#include "../DrawingElement.h"
#include "../MovableElement.h"
#include "MoveAndResizeTool.h"

namespace ImageEditor {

class Canvas;

class VectorElementTool: public MoveAndResizeTool {
public:
    VectorElementTool(Canvas* canvas, ElementType type);
    CursorType getCursor(int x, int y) override;
};

}
#endif // VectorElementTool_h_