#ifndef IMAGEEDITOR_MOVABLEELEMENTS_H
#define IMAGEEDITOR_MOVABLEELEMENTS_H

#include <memory>

#include "3rdpart/GdiplusH.h"
#include "DrawingElement.h"
#include "MovableElement.h"
#include "Core/Utils/CoreTypes.h"

namespace ImageEditor {
class InputBox;
class Line: public MovableElement {
    public:
        Line(Canvas* canvas,int startX, int startY, int endX,int endY);
        void render(Painter* gr) override;
        void setEndPoint(POINT endPoint) override;
        void getAffectedSegments(AffectedSegments* segments) override;
        void createGrips() override;
        bool isItemAtPos(int x, int y) override;
        ElementType getType() const override;
        DISALLOW_COPY_AND_ASSIGN(Line);
};

class TextElement: public MovableElement{
    public:
        TextElement( Canvas* canvas, std::shared_ptr<InputBox> inputBox, int startX, int startY, int endX,int endY, bool filled = false);
        void render(Painter* gr) override;
        void getAffectedSegments(AffectedSegments* segments) override;
        void resize(int width, int height) override;
        void setInputBox(std::shared_ptr<InputBox> inputBox);
        void setFont(LOGFONT font,  DWORD changeMask);
        LOGFONT getFont() const;
        std::shared_ptr<InputBox> getInputBox() const;
        ElementType getType() const override;
        void setSelected(bool selected) override;
        void beginEdit();
        void endEdit(bool saveToHist);
        void setRawText(const std::string& rawText);
        void setColor(Gdiplus::Color color) override;
        bool isEmpty() const override;
        void setFillBackground(bool fill);
        bool getFillBackground() const;
protected:
    std::shared_ptr<InputBox> inputBox_;
    LOGFONT font_;
    bool isEditing_;
    bool firstEdit_;
    bool fillBackground_;
    std::string originalRawText_;
    void onTextChanged(LPCTSTR text);
    void onEditCanceled();
    void onEditFinished();
    void onControlResized(int w, int h);
    void setTextColor();
    void onSelectionChanged(int min, int max, LOGFONT font);
    void saveToHistory();
    DISALLOW_COPY_AND_ASSIGN(TextElement);
};

class Crop: public MovableElement {
public:
    Crop(Canvas* canvas, int startX, int startY, int endX,int endY);
    void render(Painter* gr) override;
    void getAffectedSegments(AffectedSegments* segments) override;
    ElementType getType() const override;
    void setPos(int  x, int y) override;
    bool move(int  offsetX, int offsetY, bool checkBounds) override;
    void resize(int width, int height) override;
    RECT getPaintBoundingRect() override;
    DISALLOW_COPY_AND_ASSIGN(Crop);
};

class CropOverlay: public MovableElement {
public:
    CropOverlay(Canvas* canvas, int startX, int startY, int endX,int endY);
    void render(Painter* gr) override;
    DISALLOW_COPY_AND_ASSIGN(CropOverlay);
private:
    std::unique_ptr<Gdiplus::Font> font_;
};

class BlurringRectangle: public MovableElement {
public:
    BlurringRectangle(Canvas* canvas, float blurRadius, int startX, int startY, int endX,int endY, bool pixelate = false, bool invertSelection = false);
    ~BlurringRectangle() override;
    void setBlurRadius(float radius);
    float getBlurRadius() const;
    void render(Painter* gr) override;
    ElementType getType() const override;
    void setInvertSelection(bool invert);
    bool getInvertSelection() const;
    RECT getPaintBoundingRect() override;
protected:
    float blurRadius_;
    bool pixelate_;
    bool invertSelection_;
    DISALLOW_COPY_AND_ASSIGN(BlurringRectangle);
};

class PixelateRectangle : public BlurringRectangle {
public:
    PixelateRectangle(Canvas* canvas, float blurRadius, int startX, int startY, int endX, int endY, bool invertSelection = false);
    ElementType getType() const override;
protected:
    DISALLOW_COPY_AND_ASSIGN(PixelateRectangle);
};

class Rectangle: public MovableElement {
public:
    Rectangle(Canvas* canvas, int startX, int startY, int endX, int endY, bool filled = false, bool drawBorder = true);
    void render(Painter* gr) override;
    void getAffectedSegments(AffectedSegments* segments) override;
    bool isItemAtPos(int x, int y) override;
    RECT getPaintBoundingRect() override;

    ElementType getType() const override;

protected:
    bool filled_;
    DISALLOW_COPY_AND_ASSIGN(Rectangle);
};

class FilledRectangle: public Rectangle {
public:
    FilledRectangle(Canvas* canvas, int startX, int startY, int endX,int endY, bool drawBorder );
    ElementType getType() const override;
    DISALLOW_COPY_AND_ASSIGN(FilledRectangle);
};

class RoundedRectangle: public Rectangle {
public:
    RoundedRectangle(Canvas* canvas, int startX, int startY, int endX,int endY,bool filled = false, bool drawBorder = true);
    void render(Painter* gr) override;
    ElementType getType() const override;
    DISALLOW_COPY_AND_ASSIGN(RoundedRectangle);
};

class FilledRoundedRectangle: public RoundedRectangle {
public:
    FilledRoundedRectangle(Canvas* canvas, int startX, int startY, int endX, int endY, bool drawBorder);
    ElementType getType() const override;
    DISALLOW_COPY_AND_ASSIGN(FilledRoundedRectangle);
};

class Arrow: public Line {
public:
    enum class ArrowMode { Mode1, Mode2 };

    Arrow(Canvas* canvas,int startX, int startY, int endX,int endY, ArrowMode mode = ArrowMode::Mode1);
    void render(Painter* gr) override;
    RECT getPaintBoundingRect() override;
    ElementType getType() const override;
    static void render(Painter* gr, Gdiplus::Color color, int penSize, POINT startPoint, POINT endPoint, ArrowMode mode);
    DISALLOW_COPY_AND_ASSIGN(Arrow);
protected:
    ArrowMode mode_;
};

class Ellipse: public MovableElement {
public:
    explicit Ellipse(Canvas* canvas, bool filled = false, bool drawBorder = true );
    void render(Painter* gr) override;
    bool isItemAtPos(int x, int y) override;
    ElementType getType() const override;
    RECT getPaintBoundingRect() override;

protected:
    bool filled_;
    static bool containsPoint(Gdiplus::Rect ellipse, const Gdiplus::Point& location);
    void createGrips() override;
    DISALLOW_COPY_AND_ASSIGN(Ellipse);
};

class FilledEllipse: public Ellipse {
public:
    explicit FilledEllipse(Canvas* canvas, bool drawBorder);
    ElementType getType() const override;
    DISALLOW_COPY_AND_ASSIGN(FilledEllipse);
};

class Selection: public MovableElement {
public:
    Selection(Canvas* canvas, int startX, int startY, int endX,int endY);
    void render(Painter* gr) override;
    ElementType getType() const override;
    void createGrips() override;
    DISALLOW_COPY_AND_ASSIGN(Selection);
};

class StepNumber : public MovableElement {
public:
    StepNumber(Canvas* canvas, int startX, int startY, int endX, int endY, int number, int fontSize);
    void setFontSize(int size);
    void render(Painter* gr) override;
    RECT getPaintBoundingRect() override;
    void setNumber(int number);
    bool isResizable() const override;

    ElementType getType() const override;
protected:
    int number_;
    int fontSize_;
    int recalcRadius();
    DISALLOW_COPY_AND_ASSIGN(StepNumber);
};

}

#endif
