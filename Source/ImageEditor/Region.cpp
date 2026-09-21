/*
     Uptooda - free application for uploading images/files to the Internet

     Copyright 2007-2025 Sergey Svistunov (zenden2k@gmail.com)

     Licensed under the Apache License, Version 2.0 (the "License");
     you may not use this file except in compliance with the License.
     You may obtain a copy of the License at

     http://www.apache.org/licenses/LICENSE-2.0

     Unless required by applicable law or agreed to in writing, software
     distributed under the License is distributed on an "AS IS" BASIS,
     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
     See the License for the specific language governing permissions and
     limitations under the License.
*/

#include "Region.h"

#include <memory>

#include "3rdpart/GdiplusH.h"

namespace ImageEditor {

Region::Region(int x,int y, int w, int h)
{
    rgn_ = std::make_shared<Gdiplus::Region>(Gdiplus::Rect(x,y,w,h));
}

Region::Region(Gdiplus::Region* rgn)
{
    rgn_.reset(rgn);
}

Region Region::intersected(const Region & r) const
{
    Gdiplus::Region* rgn = rgn_->Clone();
    rgn->Intersect(r.toNativeRegion().get());
    return rgn;
}

Region Region::subtracted(const Region & r) const
{
    Gdiplus::Region* rgn = rgn_->Clone();

    rgn->Exclude(r.toNativeRegion().get());
    return rgn;
}

Region Region::united(const Region & r) const
{
    Gdiplus::Region* rgn = rgn_->Clone();

    rgn->Union(r.toNativeRegion().get());
    return rgn;
}

std::shared_ptr<Gdiplus::Region> Region::toNativeRegion() const
{
    return rgn_;
}

}
