#ifndef IMAGEEDITOR_INPUTBOX_H
#define IMAGEEDITOR_INPUTBOX_H

#include <boost/signals2.hpp>

#include "3rdpart/GdiplusH.h"
#include "MovableElement.h"

namespace ImageEditor {
class InputBox {
    public:
        virtual ~InputBox(){};
        virtual void show(bool show) = 0;
        virtual void resize(int x, int y, int w,int h, std::vector<MovableElement::Grip> grips ) = 0;
        virtual void render(Gdiplus::Graphics* graphics, Gdiplus::Bitmap* background, Gdiplus::Rect layoutArea)=0;
        virtual bool isVisible() = 0;
        virtual void invalidate() = 0;
        virtual void setTextColor(Gdiplus::Color color) =0;
        virtual void setFont(LOGFONT font, DWORD changeMask) = 0;
        virtual void setRawText(const std::string& text) = 0;
        virtual std::string getRawText() = 0;
        virtual bool isEmpty() = 0;
        boost::signals2::signal<void(LPCTSTR)> onTextChanged;
        boost::signals2::signal<void(int, int)> onSizeChanged;
        boost::signals2::signal<void()> onEditCanceled;
        boost::signals2::signal<void()> onEditFinished;
        boost::signals2::signal<void(int,int)> onResized;
        boost::signals2::signal<void(int,int,LOGFONT)> onSelectionChanged;
};
}

#endif