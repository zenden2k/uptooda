// This class section was inspired by KGySoft.Drawing
// https://github.com/koszeggy/KGySoft.Drawing, which is under the KGy SOFT License.
// Original source code: https://github.com/koszeggy/KGySoft.Drawing/blob/master/KGySoft.Drawing.Core/Drawing/Imaging/_Quantizers/OptimizedPaletteQuantizer.Octree.cs
// The KGy SOFT License is available at https://github.com/koszeggy/KGySoft.Drawing/blob/master/LICENSE

#pragma once
#include <algorithm>
#include <vector>
#include <cassert>
#include <memory>
#include <unordered_map>

#include <windows.h>

#include "3rdpart/GdiPlusH.h"
#include "Core/Utils/CoreUtils.h"

Gdiplus::Color BlendWithBackgroundSrgb(Gdiplus::Color c, Gdiplus::Color backColor);

class MyPalette {
public:
    explicit MyPalette(Gdiplus::ColorPalette* palette, Gdiplus::Color backColor, BYTE alphaThreshold = 128);
    int findNearestColorIndexSrgb(Gdiplus::Color color);
    int getNearestColorIndex(Gdiplus::Color c);
private:
    Gdiplus::ColorPalette* palette_;
    std::unordered_map<Gdiplus::ARGB, int> colorToIndex_;
    Gdiplus::Color backColor_;
    BYTE alphaThreshold_;
    int transparentIndex_;
    bool hasMultiLevelAlpha_ = false;
    bool hasAlpha_ = false;
    bool isGrayscale_ = false;
};

class OctreeColorQuantizer {

public:
    class OctreeNode {
    private:
        OctreeColorQuantizer* parent_ = nullptr;

        unsigned sumRed_ = 0;
        unsigned sumGreen_ = 0;
        unsigned sumBlue_ = 0;
        int pixelCount_ = 0;
        OctreeNode* children_[8] {};
    public:
        OctreeNode(int level, OctreeColorQuantizer* parent);
        ~OctreeNode();
        int deepPixelCount();

        bool isEmpty()
        {
            return pixelCount_ == 0;
        }

        bool addColor(Gdiplus::Color color, int level);

        void mergeNodes(int& leavesCount);

        Gdiplus::Color toColor()
        {
            assert(!isEmpty());
            return pixelCount_ == 1
                ? Gdiplus::Color((byte)sumRed_, (byte)sumGreen_, (byte)sumBlue_)
                : Gdiplus::Color((byte)(sumRed_ / pixelCount_), (byte)(sumGreen_ / pixelCount_), (byte)(sumBlue_ / pixelCount_));
        }

        void populatePalette(Gdiplus::Color* result, int& palIndex, int& remainingColors);
    };

    private:
        int maxColors_ = 0;
        int size_ = 0;
        int levelCount_ = 0;
        bool hasTransparency_ = false;
        std::vector<std::vector<OctreeNode*>> levels_;
        std::unique_ptr<OctreeNode> root_;
        int leavesCount_ = 0;

        int colorCount()
        {
            return leavesCount_ + (hasTransparency_ ? 1 : 0);
        }

    public:
        explicit OctreeColorQuantizer(int requestedColors, BYTE bitLevel, Gdiplus::BitmapData* source);

        void addColor(Gdiplus::Color color) {
            if (color.GetA() == 0) {
                hasTransparency_ = true;
            } else if (root_->addColor(color, 0)) {
                leavesCount_++;
            }
        }

        unique_c_ptr<Gdiplus::ColorPalette> generatePalette();

        void reduceTree();
};
