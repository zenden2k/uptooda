#include "AbstractCodeGenerator.h"

namespace Uptooda::Core::OutputGenerator {

std::string AbstractCodeGenerator::doGenerate(const std::vector<UploadObject>& items) {
    std::string result;
    std::vector<std::string> groups;
    std::vector<std::string> fileNames;
    std::vector<int> counters;
    for (const auto & item : items) {
        if (item.isNull()) {
            continue;
        }
        size_t index = groupByFile_ ? item.fileIndex : 0;
        if (groups.size() <= index) {
            groups.resize(index + 1);
            fileNames.resize(index + 1);
            counters.resize(index + 1);
        }

        std::string& cur = groups[index];
        int ii = counters[index]++;
        fileNames[index] = item.onlyFileName();
        cur += generateCodeForItem(item, ii);
    }

    for (size_t i = 0; i < groups.size(); i++) {
        result += group(fileNames[i], groups[i], i);
    }

    return result;
}

std::string AbstractCodeGenerator::generateCodeForItem(const UploadObject& item, int index) {
    std::string url = getItemTargetUrl(item);
    std::string res;

    if ((codeType_ == ctClickableThumbnails || codeType_ == ctTableOfThumbnails)) {
        if (codeType_ == ctClickableThumbnails && index) {
            res += rowSeparator();
        }
        if (!item.getThumbUrl(shortenUrl_).empty()) {
            res += link(url, image(item.getThumbUrl(shortenUrl_), item.onlyFileName()));
        } else {
            res += link(url, groupByFile_ ? item.uploadResult.serverName : item.onlyFileName());
        }

        if (codeType_ == ctTableOfThumbnails) {
            if ((index + 1) % itemsPerLine_) {
                res += itemSeparator();
            } else {
                res += rowSeparator();
            }
        }
        return res;
    } else if (codeType_ == ctImages) {
        if (index) {
            res += lineSeparator();
            res += lineSeparator();
        }
        if (!item.getImageUrl(shortenUrl_).empty() && (preferDirectLinks_ || item.getDownloadUrl(shortenUrl_).empty())) {
            res += image(item.getImageUrl(shortenUrl_), item.onlyFileName());
        } else {
            res += link(url, groupByFile_ ? item.uploadResult.serverName : item.onlyFileName());
        }

        return res;
    } else {
        if (index) {
            if (!groupByFile_) {
                res += lineSeparator();
            }
            res += lineSeparator();
        }
        res += link(url, groupByFile_ ? item.uploadResult.serverName : item.onlyFileName());
    }
    return res;
    // if (m_CodeType == ctLinks ||  item.directUrl.empty())
}

std::string AbstractCodeGenerator::image(const std::string& url, const std::string& alt){
    return {};
}

std::string AbstractCodeGenerator::link(const std::string& url, const std::string& body){
    return {};
}

std::string AbstractCodeGenerator::group(const std::string& fileName, const std::string& content, size_t index){
    std::string result;
    if (index) {
        result += lineSeparator();
    }
    result += content;
    return result;
}

std::string AbstractCodeGenerator::itemSeparator() const {
    return "  ";
}

std::string AbstractCodeGenerator::rowSeparator() const {
    return "\r\n\r\n";
}

std::string AbstractCodeGenerator::lineSeparator() const {
    return "\r\n";
}

std::string AbstractCodeGenerator::getItemTargetUrl(const UploadObject& item) const {
    return (!item.getImageUrl(shortenUrl_).empty() && preferDirectLinks_ || item.getDownloadUrl(shortenUrl_).empty()) ? item.getImageUrl(shortenUrl_) : item.getDownloadUrl(shortenUrl_);
}

}
