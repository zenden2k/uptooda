#ifndef IMAGEEDITOR_SPEECHBALOON_H
#define IMAGEEDITOR_SPEECHBALOON_H

#include "MovableElements.h"

namespace ImageEditor {

class SpeechBaloon : public TextElement {
public:
    SpeechBaloon(Canvas* canvas, std::shared_ptr<InputBox> inputBox, int startX, int startY, int endX, int endY);

    void render(Painter* gr) override;
    void resize(int width, int height) override;
    ElementType getType() const override;
    Gdiplus::Rect getInputBoxRect() override;

protected:
    void onControlResized(int width, int height) override;

private:
    enum class TailSide { LEFT, TOP, RIGHT, BOTTOM };

    struct Geometry {
        Gdiplus::Rect BubbleRect;
        Gdiplus::Point TailTip;
        Gdiplus::Point TailBaseStart;
        Gdiplus::Point TailBaseEnd;
        TailSide Side;
    };

    Geometry getGeometry(int inset = 0);
    static void addSpeechBaloonPath(Gdiplus::GraphicsPath& path, const Geometry& geometry, int radius);

    int inputWidth_ { };
    int inputHeight_ { };

    DISALLOW_COPY_AND_ASSIGN(SpeechBaloon);
};

}

#endif
