#ifndef CropTool_h_
#define CropTool_h_

#include "3rdpart/GdiplusH.h"
#include "../DrawingElement.h"
#include "MoveAndResizeTool.h"

namespace ImageEditor {

class Canvas;

class CropTool : public MoveAndResizeTool {
public:
    explicit CropTool(Canvas* canvas);
    void beginDraw(int x, int y) override;
    void continueDraw(int x, int y, DWORD flags) override;
    void endDraw(int x, int y) override;
    bool applyOperation() override;
    void cancelOperation() override;
};

}

#endif // CropTool_h_