#ifndef MoveAndResizeTool_h__
#define MoveAndResizeTool_h__

#include "3rdpart/GdiplusH.h"
#include "../DrawingElement.h"
#include "../MovableElement.h"
#include "AbstractDrawingTool.h"

namespace ImageEditor {

class Canvas;

class CropOverlay;

class MoveAndResizeTool: public AbstractDrawingTool {
public:
    explicit MoveAndResizeTool( Canvas* canvas, ElementType type = ElementType::etNone );
    void beginDraw( int x, int y ) override;
    void continueDraw( int x, int y, DWORD flags ) override;
    void endDraw( int x, int y ) override;
    void render( Painter* gr ) override;
protected:
    MovableElement* currentElement_;
    ElementType elementType_;
    MovableElement::Grip draggedBoundary_;
    void createElement();
    bool canActivateElement(const MovableElement* element) const;
    MovableElement::Grip checkElementsBoundaries(int x, int y, MovableElement** elem = 0);
    MovableElement::Grip checkElementBoundaries(MovableElement*, int x, int y);
    static void cleanUp();
    CursorType getCursor(int x, int y) override;
    void mouseDoubleClick(int x, int y) override;

    static CropOverlay* cropOverlay_;
    bool isMoving_;
    bool allowCreatingElements_;
    bool elementJustCreated_;
    POINT originalStartPoint_;
    POINT originalEndPoint_;
    RECT prevPaintBoundingRect_;
    bool allowMovingElements_;
};

}
#endif // MoveAndResizeTool_h__
