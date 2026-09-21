#include "OctreeColorQuantizer.h"

int ToBitsPerPixel(int colorCount) {
    if (colorCount == 1) {
        return 1;
    }

    // Bits per pixel is actually ceiling of log2(maxColors)
    int bpp = 0;
    for (int n = colorCount - 1; n > 0; n >>= 1) {
        bpp++;
    }

    return bpp;
}

BYTE GetBrightness(Gdiplus::Color c) {
    const float RLumSrgb = 0.299f;
    const float GLumSrgb = 0.587f;
    const float BLumSrgb = 0.114f;

     if (c.GetR() == c.GetG() && c.GetR() == c.GetB())
        return c.GetR();

    return c.GetR() * RLumSrgb + c.GetG() * GLumSrgb + c.GetB() * BLumSrgb;
}

Gdiplus::Color BlendWithBackgroundSrgb(Gdiplus::Color c, Gdiplus::Color backColor) {
    assert(c.GetA() != 255);
    assert(backColor.GetA() == 255);

    // The blending is applied only to the color and not the resulting alpha, which will always be opaque
    if (c.GetA() == 0)
        return backColor;
    int inverseAlpha = 255 - c.GetA();

    // The non-accelerated version. Division, eg. r:(byte)((c.R * c.A + backColor.R * inverseAlpha) / 255) would be more accurate
    // but unlike for premultiplication we use the faster bit shifting because there is no inverse operation for blending.
    return Gdiplus::Color(255,
        (BYTE)((c.GetR() * c.GetA() + backColor.GetR() * inverseAlpha) >> 8),
        (BYTE)((c.GetG() * c.GetA() + backColor.GetG() * inverseAlpha) >> 8),
        (BYTE)((c.GetB() * c.GetA() + backColor.GetB() * inverseAlpha) >> 8));
}

MyPalette::MyPalette(Gdiplus::ColorPalette* palette, Gdiplus::Color backColor, BYTE alphaThreshold /*= 128*/)
    : palette_(palette)
    , backColor_(backColor)
    , alphaThreshold_(alphaThreshold)
{
    transparentIndex_ = -1;
    isGrayscale_ = true;
    for (int i = 0; i < palette->Count; i++) {
        Gdiplus::Color c = palette->Entries[i];
        if (!colorToIndex_.count(c.GetValue()) && !(alphaThreshold_ == 0 && c.GetA() == 0 && !hasMultiLevelAlpha_)) {
            colorToIndex_[c.GetValue()] = i;
        }

        if (c.GetA() != 255) {
            hasAlpha_ = true;
            if (!hasMultiLevelAlpha_) {
                hasMultiLevelAlpha_ = c.GetA() > 0;
            }

            if (c.GetA() == 0) {
                if (transparentIndex_ < 0)
                    transparentIndex_ = i;
                continue;
            }
        }

        if (isGrayscale_) {
            isGrayscale_ = c.GetR() == c.GetG() && c.GetR() == c.GetB();
        }
    }
}

int MyPalette::findNearestColorIndexSrgb(Gdiplus::Color color) {
    assert(!hasMultiLevelAlpha_);
    assert(color.GetA() >= alphaThreshold_ || !(transparentIndex_ >= 0));

    int minDiff = INT_MAX;
    int resultIndex = 0;

    if (color.GetA() != 255) {
        // blending the color with background and checking if there is an exact match now
        color = BlendWithBackgroundSrgb(color, backColor_);
        auto it = colorToIndex_.find(color.GetValue());
        if (it != colorToIndex_.end())
            return it->second;
    }

    // The two similar lookups could be merged, but it is faster to separate them even if some parts are duplicated
    int len = palette_->Count;
    if (isGrayscale_) {
        BYTE brightness = GetBrightness(color);
        for (int i = 0; i < len; i++) {
            Gdiplus::Color current = palette_->Entries[i];

            // Palette color with alpha
            if (current.GetA() != 255) {
                // Skipping fully transparent palette colors because they were handled in GetNearestColorIndex
                continue;
            }

            // If the palette is grayscale, then distance is measured by perceived brightness.
            int diff = std::abs(GetBrightness(current) - brightness);
            if (diff >= minDiff)
                continue;

            // new closest match
            if (diff == 0)
                return i;
            minDiff = diff;
            resultIndex = i;
        }
    } else {

        for (int i = 0; i < len; i++) {
            Gdiplus::Color current = palette_->Entries[i];

            // Palette color with alpha
            if (current.GetA() != 255) {
                // Skipping fully transparent palette colors because they were handled in GetNearestColorIndex
                //Debug.Assert(current.A == 0, $ "If palette has partially transparent entries the {nameof(FindNearestColorIndexAlphaSrgb)} method should be used");
                continue;
            }

            // If the palette is not grayscale, then distance is measured by Manhattan distance based on RGB components.
            // Euclidean distance squared would provide a slightly different result in some cases but there is no good accelerated
            // version for it using integers (DotProduct is available for floating point arguments only)
            int diff;

            {
                diff = std::abs(current.GetR() - color.GetR()) + std::abs(current.GetG() - color.GetG()) + std::abs(current.GetB() - color.GetB());
            }

            assert(diff != 0);
            if (diff >= minDiff)
                continue;

            // new closest match
            minDiff = diff;
            resultIndex = i;
        }
    }

    return resultIndex;
}

int MyPalette::getNearestColorIndex(Gdiplus::Color c) {
    // mapping alpha to full transparency
    if (c.GetA() < alphaThreshold_ && transparentIndex_ >= 0)
        return transparentIndex_;

    // exact match: from the palette
    auto it = colorToIndex_.find(c.GetValue());
    if (it != colorToIndex_.end()) {
        return it->second;
    }

    return findNearestColorIndexSrgb(c);
}

OctreeColorQuantizer::OctreeNode::~OctreeNode()
{
    for (auto* child: children_) {
        delete child;
    }
}

int OctreeColorQuantizer::OctreeNode::deepPixelCount()
{
    int result = pixelCount_;
    /* if (children == null)
        return result;*/

    // Adding also the direct children because reducing the tree starts at level levelCount - 2.
    // And due to reducing no more than two levels can have non-empty nodes.
    for (int index = 0; index < 8; index++) {
        OctreeNode* node = children_[index];
        if (node != nullptr)
            result += node->pixelCount_;
    }

    return result;
}

OctreeColorQuantizer::OctreeNode::OctreeNode(int level, OctreeColorQuantizer* parent) {
    this->parent_ = parent;
    assert(level < parent->levelCount_);

    if (level >= 0) {
        parent->levels_[level].push_back(this);
    }
}

bool OctreeColorQuantizer::OctreeNode::addColor(Gdiplus::Color color, int level) {
    // In the populating phase all colors are summed up in leaves at deepest level.
    if (level == parent_->levelCount_) {
        sumRed_ += color.GetR();
        sumGreen_ += color.GetG();
        sumBlue_ += color.GetB();
        pixelCount_++;

        // returning whether a new leaf has been added
        return pixelCount_ == 1;
    }

    assert(level < parent_->levelCount_);

    // Generating a 0..7 index based on the color components and adding new branches on demand.
    int mask = 128 >> level;
    int branchIndex = ((color.GetR() & mask) == mask ? 4 : 0)
        | ((color.GetG() & mask) == mask ? 2 : 0)
        | ((color.GetB() & mask) == mask ? 1 : 0);
    OctreeNode* child = children_[branchIndex];

    if (!child) {
        child = new OctreeNode(level, parent_);
    }
    children_[branchIndex] = child;
    return child->addColor(color, level + 1);
}

void OctreeColorQuantizer::OctreeNode::mergeNodes(int& leavesCount) {
    auto CompareByBrightness = [](OctreeNode* a, OctreeNode* b) -> bool {
        if (a == nullptr || b == nullptr)
            return a == b ? false : a == nullptr ? true
                                                 : false;

        auto ca = a->toColor();
        auto cb = b->toColor();
        return GetBrightness(ca) < GetBrightness(cb);
    };

    auto CompareByWeightedBrightness = [this](OctreeNode* a, OctreeNode* b) -> bool {
        if (a == nullptr || b == nullptr)
            return a == b ? false : a == nullptr ? true
                                                 : false;

        auto ca = a->toColor();
        auto cb = b->toColor();
        return GetBrightness(ca) * (a->deepPixelCount() / (float)parent_->size_) < GetBrightness(cb) * (b->deepPixelCount() / (float)parent_->size_);
    };

    /* if (children == null)
                return;*/

    // If there are fewer than 8 removals left we sort them to merge the least relevant ones first.
    // For 2 colors (and 3 + TR) the distance is measured purely by brightness to avoid returning very similar colors.
    // Note: reordering children is not a problem because we don't add more colors in merging phase.
    if (parent_->colorCount() - parent_->maxColors_ < 8) {
        auto cc = parent_->maxColors_ - (parent_->hasTransparency_ ? 1 : 0);
        if (cc <= 2)
            std::sort(std::begin(children_), std::end(children_), CompareByBrightness);
        else
            std::sort(std::begin(children_), std::end(children_), CompareByWeightedBrightness);
    }
    for (int i = 0; i < 8; i++) {
        OctreeNode* node = children_[i];
        if (node == nullptr)
            continue;

        assert(!node->isEmpty());

        // Decreasing only if this node is not becoming a "leaf" while cutting a branch down.
        if (!isEmpty()) {
            leavesCount--;
        }

        sumRed_ += node->sumRed_;
        sumGreen_ += node->sumGreen_;
        sumBlue_ += node->sumBlue_;
        pixelCount_ += node->pixelCount_;

        children_[i] = nullptr;
        delete node;

        // As we can return before merging all children,
        // leavesCount may include "not-quite leaf" elements in the end.
        if (parent_->colorCount() == parent_->maxColors_)
            return;
    }
}

void OctreeColorQuantizer::OctreeNode::populatePalette(Gdiplus::Color* result, int& palIndex, int& remainingColors) {
    // if a non-empty node is found, adding it to the resulting palette
    if (!isEmpty()) {
        result[palIndex] = toColor();
        palIndex += 1;
        remainingColors -= 1;
        if (remainingColors == 0)
            return;
    }

    /* if (children == null || context.IsCancellationRequested)
        return;*/

    for (OctreeNode* child : children_) {
        if (child == nullptr)
            continue;
        child->populatePalette(result, palIndex, remainingColors);
        if (remainingColors == 0)
            return;
    }
}

OctreeColorQuantizer::OctreeColorQuantizer(int requestedColors, BYTE bitLevel, Gdiplus::BitmapData* source) {
    maxColors_ = requestedColors;
    size_ = source->Width * source->Height;

    levelCount_ = bitLevel ? bitLevel : std::min<>(8, ToBitsPerPixel(requestedColors));
    levels_.resize(levelCount_);

    root_ = std::make_unique<OctreeNode>(-1, this);
}

unique_c_ptr<Gdiplus::ColorPalette> OctreeColorQuantizer::generatePalette() {
    if (colorCount() > maxColors_)
        reduceTree();

    assert(colorCount() <= maxColors_);
    //BYTE* result = new BYTE[sizeof(Gdiplus::ColorPalette) + leavesCount];
    auto pal = make_unique_malloc<Gdiplus::ColorPalette>(sizeof(Gdiplus::ColorPalette) + leavesCount_ * sizeof(Gdiplus::ARGB));
    pal->Count = leavesCount_;
    pal->Flags = Gdiplus::PaletteFlagsHasAlpha;

    if (leavesCount_ > 0) {
        int palIndex = 0;
        root_->populatePalette(reinterpret_cast<Gdiplus::Color*>(pal->Entries), palIndex, leavesCount_);
        assert(leavesCount_ == 0);
    }

    // If transparent color is needed, then it will be automatically the last color in the result
    return pal;
}

void OctreeColorQuantizer::reduceTree() {
    // Scanning all levels towards root. Leaves are skipped (hence -2) because they are not reducible.
    for (int level = levelCount_ - 2; level >= 0; level--) {
        if (levels_[level].empty())
            continue;

        // Sorting nodes of the current level (the least significant ones first)
        // while merging them into their parents until we go under MaxColors
        auto& nodes = levels_[level];
        std::sort(nodes.begin(), nodes.end(), [](auto* a, auto* b) { return a->deepPixelCount() < b->deepPixelCount(); });
        //nodes.Sort((a, b) = > a.DeepPixelCount - b.DeepPixelCount);

        for (OctreeNode* node : nodes) {
            // As merging is stopped when we reach MaxColors.
            // leavesCount may include some half-merged non-leaf nodes as well.
            node->mergeNodes(leavesCount_);
            if (colorCount() <= maxColors_)
                return;
        }
    }

    // If we are here, we need to reduce also the root node (less than 8 colors or 8 colors + transparency)
    root_->mergeNodes(leavesCount_);
    assert(colorCount() == maxColors_);
}
