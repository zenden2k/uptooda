#include "JSONGenerator.h"

#include <json/value.h>
#include <json/writer.h>

namespace Uptooda::Core::OutputGenerator {

std::string JSONGenerator::doGenerate(const std::vector<UploadObject>& items) {
    Json::Value arrValue(Json::arrayValue);

    for (const auto& item : items) {
        Json::Value objValue(Json::objectValue);
        objValue["success"] = !item.isNull();
        objValue["direct_url"] = item.getImageUrl(shortenUrl_);
        objValue["thumb_url"] = item.uploadResult.thumbUrl;
        objValue["view_url"] = item.getDownloadUrl(shortenUrl_);
        objValue["delete_url"] = item.uploadResult.deleteUrl;
        objValue["edit_url"] = item.uploadResult.editUrl;
        objValue["message"] = item.uploadResult.message;
        objValue["filename"] = item.displayFileName;
        arrValue.append(objValue);
    }

    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "None";
    builder["indentation"] = "   ";
    return Json::writeString(builder, arrValue);
}

GeneratorID JSONGenerator::id() const
{
    return gidJSON;
}

}
