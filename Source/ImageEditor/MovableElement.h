#ifndef IMAGEEDITOR_MOVABLEELEMENT_H
#define IMAGEEDITOR_MOVABLEELEMENT_H

#include <vector>

#include "3rdpart/GdiplusH.h"
#include "BasicTypes.h"
#include "DrawingElement.h"

namespace ImageEditor {
class Canvas;
class AffectedSegments;

class MovableElement: public DrawingElement {
    public:

        // This is sizing handle dimension when DPI scale is 100%
        enum { kGripSize = 6, kSelectRadius = 5};

        enum class GripPointType {gptNone, gptStartPoint, gptEndPoint};

        struct Grip {
            POINT pt;
            BoundaryType bt;
            GripPointType gpt; 
            Grip() {
                pt = {-1, -1};
                bt = BoundaryType::btNone;
                gpt = GripPointType::gptNone;
            }
        };
        MovableElement(Canvas* canvas_);
        void render(Painter* gr) override;
        void renderGrips(Painter* gr);
        virtual void setSelected(bool selected);
        bool isSelected() const;
        virtual ElementType getType() const;
        static CursorType GetCursorForBoundary(BoundaryType bt);
        friend class MoveAndResizeTool;
        void setDrawDashedRectangle(bool draw);
        int getX();
        int getY();
        virtual RECT getPaintBoundingRect();
        virtual void setPos(int  x, int y);
        virtual bool move(int  offsetX, int offsetY, bool checkBounds);
        virtual bool isItemAtPos(int x, int y);

        void resize(int width, int height) override;
        virtual void createGrips();
        virtual void beginMove();
        virtual void endMove();
        bool isPenSizeUsed() const;
        bool isColorUsed() const;
        bool isBackgroundColorUsed() const;
        virtual bool isResizable() const;
        virtual bool isEmpty() const;

        bool isMoving() const;

        void setDrawBorder(bool enable);
        bool getDrawBorder() const;

    protected:
        bool isSelected_;
        bool drawDashedRectangle_;
        bool drawDashedRectangleWhenSelected_;
        std::vector<Grip> grips_;
        int gripWidth_, gripHeight_;
        
        POINT* getMaxPoint(Axis axis);
        POINT* getMinPoint(Axis axis);
        bool isMoving_;
        bool isPenSizeUsed_;
        bool isColorUsed_;
        bool isBackgroundColorUsed_;
        bool drawBorder_ = false;
};

}

#endif
