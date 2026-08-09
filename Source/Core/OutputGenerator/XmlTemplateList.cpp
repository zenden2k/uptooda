#include "XmlTemplateList.h"

#include "Core/Utils/IOException.h"
#include "Core/Utils/SimpleXml.h"

namespace Uptooda::Core::OutputGenerator {

void XmlTemplateList::loadFromFile(const std::string& fileName) {
    SimpleXml XML;
    if (!IuCoreUtils::FileExists(fileName)) {
        throw IOException("File not found.");
    }

    if (!XML.LoadFromFile(fileName)) {
        throw std::runtime_error("XML loading error.");
    }

    SimpleXmlNode templatesNode = XML.getRoot("Templates");
    if (templatesNode.IsNull()) {
        throw std::runtime_error("Unable to find Templates node");
    }

    std::vector<SimpleXmlNode> templates;
    templatesNode.GetChilds("Template", templates);

    for (auto & templateNode : templates) {
        ResultTemplate Template;
        Template.Name = templateNode.Attribute("Name");

        Template.TemplateText = templateNode["Text"].Text();

        SimpleXmlNode itemsNode = templateNode["Items"];
        if (!itemsNode.IsNull())
        {
            Template.LineStart = itemsNode.Attribute("LineStart");
            Template.LineEnd = itemsNode.Attribute("LineEnd");
            Template.LineSep = itemsNode.Attribute("LineSep");
            Template.ItemSep = itemsNode.Attribute("ItemSep");

            Template.Items = itemsNode.Text();
        }

        templates_.push_back(Template);
    }
}

void XmlTemplateList::clear() {
    templates_.clear();
}

size_t XmlTemplateList::size() const {
    return templates_.size();
}

const ResultTemplate& XmlTemplateList::at(size_t index) {
    return templates_.at(index);
}

}
