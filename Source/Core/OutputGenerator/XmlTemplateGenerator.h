
#pragma once

#include <unordered_map>
#include <string>
#include <vector>

#include "AbstractOutputGenerator.h"
#include "XmlTemplateList.h"

namespace Uptooda::Core::OutputGenerator {

class XmlTemplateGenerator : public AbstractOutputGenerator {
public:
    explicit XmlTemplateGenerator(XmlTemplateList* templateList);
    GeneratorID id() const override;
    void setTemplateIndex(size_t index);
private:
    static std::string replaceVars(const std::string& text, const std::unordered_map<std::string, std::string>& vars);
    XmlTemplateList* templateList_;
    size_t templateIndex_ = 0;
    std::string doGenerate(const std::vector<UploadObject>& items) override;
};

}
