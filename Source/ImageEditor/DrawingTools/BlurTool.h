#ifndef BlurTool_h__
#define BlurTool_h__

#include "3rdpart/GdiplusH.h"
#include "../DrawingElement.h"
#include "../MovableElement.h"
#include "AbstractDrawingTool.h"
#include "BrushTool.h"
namespace ImageEditor {

    class Canvas;

#if GDIPVER >= 0x0110 
class BlurTool: public BrushTool  {
public:
    BlurTool( Canvas* canvas );
protected:
    virtual void drawLine(int x0, int y0, int x1, int y1);

    virtual void endDraw(int x, int y);

};
#endif

}
#endif // BlurTool_h__