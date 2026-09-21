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

#ifndef IU_CORE_IMAGES_THUMBNAIL_H
#define IU_CORE_IMAGES_THUMBNAIL_H

#include <string>
#include <map>
#include <vector>
#include "Core/Utils/CoreTypes.h"

class Thumbnail
{
    public:
        enum PresetColorRole { kFrameColor, kStrokeColor, kGradientColor1, kTextColor };

        struct ThumbnailDrawOperation
        {
            std::string type;
            std::string rect;
            std::string brush;
            std::string source_rect;
            std::string source;
            std::string destination;
            std::string pen;
            std::string condition;
            std::string text_colors;
        };

        struct ThumbnailData
        {
            std::map<std::string, std::string> colors_;
            std::vector<ThumbnailDrawOperation> drawing_operations_;
            std::string width_addition;
            std::string height_addition;
            std::string sprite_source_file;
        };

        Thumbnail() = default;

        bool loadFromFile(const std::string& filename);
        void createNew();
        bool saveToFile(const std::string& filename = "" );

        /* accessor functions */
        std::string getSpriteFileName() const;
        std::string getWidthAddition() const;
        std::string getHeightAddition() const;

        void setSpriteFileName(const std::string& name);

        bool existsParam(const std::string& name) const;
        unsigned int getColor(const std::string& name) const;
        unsigned int getParam(const std::string& name) const;

        void setColor(const std::string& name, unsigned int value);
        void setParam(const std::string& name, unsigned int value);
        void setParamString(const std::string& name, const std::string& value);
        std::string getParamString(const std::string& name) const;
        const ThumbnailData* getData() const;
    private:
        ThumbnailData data_;
        std::string file_name_;
        DISALLOW_COPY_AND_ASSIGN(Thumbnail);
};
#endif
