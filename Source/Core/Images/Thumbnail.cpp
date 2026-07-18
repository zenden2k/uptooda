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

#include "Thumbnail.h"

#include <cstdlib>

#include "Core/Utils/CoreUtils.h"
#include "Core/Utils/SimpleXml.h"
#include "Core/Utils/StringUtils.h"

bool Thumbnail::loadFromFile(const std::string& filename)
{
    SimpleXml xml;
    if (!xml.LoadFromFile(filename))
        return false;
    SimpleXmlNode root = xml.getRoot("Thumbnail", false);
    if (root.IsNull())
        return false;
    data_.sprite_source_file = root["Definitions"]["Sprite"].Attribute("Source");

    if (!data_.sprite_source_file.empty() && IuCoreUtils::ExtractFilePath(data_.sprite_source_file).empty())
    {
        data_.sprite_source_file = IuCoreUtils::ExtractFilePath(filename) + "/" + data_.sprite_source_file;
    }
    SimpleXmlNode colorsNode = root["Definitions"]["Params"];
    std::vector<SimpleXmlNode> colorNodes;
    colorsNode.GetChilds("Param", colorNodes);
    for (size_t i = 0; i < colorNodes.size(); i++)
    {
        std::string colorName;
        colorName = colorNodes[i].Attribute("Name");
        data_.colors_[colorName] =  colorNodes[i].Attribute("Value");
    }
    SimpleXmlNode drawingNode = root["Drawing"];
    if (!drawingNode.IsNull())
    {
        data_.width_addition = drawingNode.Attribute("AddWidth");
        data_.height_addition = drawingNode.Attribute("AddHeight");
        std::vector<SimpleXmlNode> drawOperations;
        drawingNode.GetChilds("Operation", drawOperations);
        for (size_t i = 0; i < drawOperations.size(); i++)
        {
            ThumbnailDrawOperation op;
            op.brush = drawOperations[i].Attribute("Brush");
            op.rect = drawOperations[i].Attribute("Rect");
            op.source = drawOperations[i].Attribute("Source");
            op.destination = drawOperations[i].Attribute("Destination");
            op.source_rect = drawOperations[i].Attribute("SourceRect");
            op.type = drawOperations[i].Attribute("Type");
            op.pen = drawOperations[i].Attribute("Pen");
            op.condition = drawOperations[i].Attribute("Condition");
            op.text_colors = drawOperations[i].Attribute("TextColors");
            data_.drawing_operations_.push_back(op);
        }
    }
    file_name_ = filename;
    return true;
}

void Thumbnail::createNew()
{
    data_.colors_["FrameColor"] = "#004A6F";
    data_.colors_["StrokeColor"] = "0";
    data_.colors_["GradientColor1"] = "0";
    data_.colors_["GradientColor2"] = "0";
    data_.colors_["TextColor"] = "0";
}

bool Thumbnail::saveToFile(const std::string& filename)
{
    SimpleXml xml;
    std::string fileToSave = filename;
    if (filename.empty())
    {
        fileToSave = file_name_;
    }
    xml.LoadFromFile(fileToSave);
    SimpleXmlNode root = xml.getRoot("Thumbnail", false);
    if (root.IsNull())
        return false;
    if ( !data_.sprite_source_file.empty() ) {
        root.GetChild("Definitions").GetChild("Sprite").SetAttribute("Source", IuCoreUtils::ExtractFileName(data_.sprite_source_file));
    }
    SimpleXmlNode colorsNode = root.GetChild("Definitions").GetChild("Params");
    colorsNode.DeleteChilds();
    for (auto it = data_.colors_.begin(); it != data_.colors_.end(); ++it)
    {
        SimpleXmlNode colorNode = colorsNode.CreateChild("Param");
        colorNode.SetAttribute("Name", it->first);
        colorNode.SetAttribute("Value", it->second);
    }
    return xml.SaveToFile(fileToSave);
}

std::string Thumbnail::getSpriteFileName() const
{
    return data_.sprite_source_file;
}

std::string Thumbnail::getWidthAddition() const
{
    return data_.width_addition;
}

std::string Thumbnail::getHeightAddition() const
{
    return data_.height_addition;
}

void Thumbnail::setSpriteFileName(const std::string& name)
{
    data_.sprite_source_file = name;
}

const Thumbnail::ThumbnailData* Thumbnail::getData() const
{
    return &data_;
}

unsigned int Thumbnail::getColor(const std::string& name) const
{
    auto it = data_.colors_.find(name);
    if (it != data_.colors_.end()) {
        std::string res = it->second;
        res = IuStringUtils::Replace(res, "#", "0x");
        return strtoul(res.c_str(), 0, 0);
    }
    return 0;
}

unsigned int Thumbnail::getParam(const std::string& name) const
{
    auto it = data_.colors_.find(name);
    if (it != data_.colors_.end()) {
        return atoi(it->second.c_str());
    }
    return 0;
}

void Thumbnail::setColor(const std::string& name, unsigned int value)
{
    char buf[20];
    sprintf(buf, "#%06x", value);
    data_.colors_[name] = buf;
}

void Thumbnail::setParam(const std::string& name, unsigned int value)
{
    data_.colors_[name] = std::to_string(value);
}

void Thumbnail::setParamString(const std::string& name, const std::string& value)
{
    data_.colors_[name] = value;
}

std::string Thumbnail::getParamString(const std::string& name) const
{
    auto it = data_.colors_.find(name);
    return it == data_.colors_.end() ? std::string() : it->second;
}

bool Thumbnail::existsParam(const std::string& name) const
{
    return data_.colors_.count(name) > 0;
}
