#include "XmlTemplateGenerator.h"

#include <unordered_map>

#include "Core/3rdpart/pcreplusplus.h"
#include "Core/Utils/StringUtils.h"

namespace Uptooda::Core::OutputGenerator {

XmlTemplateGenerator::XmlTemplateGenerator(XmlTemplateList* templateList): templateList_(templateList) {

}

std::string XmlTemplateGenerator::doGenerate(const std::vector<UploadObject>& items) {
    std::string res;
    std::string itemsStr;
    std::unordered_map<std::string, std::string> vars;
    size_t n = items.size();
    int p = itemsPerLine_;
    const auto& templ = templateList_->at(templateIndex_);
    for (int i = 0; i < n; i++) {
        auto& item = items[i];
        if (item.isNull()) {
            continue;
        }
        std::string fname = item.onlyFileName();
        std::string imageUrl = item.getImageUrl(shortenUrl_);
        vars["DownloadUrl"] = item.getDownloadUrl(shortenUrl_);
        vars["ImageUrl"] = imageUrl.empty() ? item.getDownloadUrl(shortenUrl_) : imageUrl;
        vars["ThumbUrl"] = item.getThumbUrl(shortenUrl_);
        vars["FileName"] = fname;
        vars["FullFileName"] = item.displayFileName;
        vars["Index"] = std::to_string(i);
        //CString buffer = WinUtils::GetOnlyFileName(UrlList[i].FileName);
        vars["FileNameWithoutExt"] = IuCoreUtils::ExtractFileNameNoExt(item.displayFileName);
        if (p != 0 && !((i) % p))

            itemsStr += replaceVars(templ.LineStart, vars);
        itemsStr += replaceVars(templ.Items, vars);
        if ((p != 0 && ((i + 1) % p)) || p == 0)
            itemsStr += replaceVars(templ.ItemSep, vars);

        if (p != 0 && !((i + 1) % p)) {
            itemsStr += replaceVars(templ.LineEnd, vars);
            if (p != 0)
                itemsStr += replaceVars(templ.LineSep, vars);
        }
    }
    vars.clear();
    vars["Items"] = itemsStr;
    res += replaceVars(templ.TemplateText, vars);
    return res;
}

GeneratorID XmlTemplateGenerator::id() const
{
    return gidXmlTemplate;
}

std::string XmlTemplateGenerator::replaceVars(const std::string& text, const std::unordered_map<std::string, std::string>& vars)
{
    std::string result = text;

    pcrepp::Pcre reg(R"(\$\(([A-z0-9_]*?)\))", "imc");
    int pos = 0;

    while (pos <= text.length()) {
        if (reg.search(text, pos)) {
            pos = reg.get_match_end() + 1;
            std::string vv = reg[1];
            result = IuStringUtils::Replace(result, "$(" + vv + ")", vars.at(vv));
        }
        else {
            break;
        }
    }

    return IuStringUtils::Replace(result, "\\n", "\r\n");
}

void XmlTemplateGenerator::setTemplateIndex(size_t index) {
    templateIndex_ = index;
}

}
