#ifndef IMAGEEDITOR_REGION_H
#define IMAGEEDITOR_REGION_H

#include <memory>

#include "3rdpart/GdiplusH.h"

#include "atlheaders.h"
#include "Core/Utils/CoreTypes.h"

namespace ImageEditor 
{
    class Region {
    public:
        Region(int x,int y, int w, int h);
        #ifndef QT_VERSION
        Region(Gdiplus::Region* rgn );
        #endif
        Region intersected(const Region & r) const;
        Region subtracted(const Region & r) const;
        Region united(const Region & r) const;
        std::shared_ptr<Gdiplus::Region> toNativeRegion() const;
    private:
#ifndef QT_VERSION
        std::shared_ptr<Gdiplus::Region> rgn_;
#endif
    };
}
#endif