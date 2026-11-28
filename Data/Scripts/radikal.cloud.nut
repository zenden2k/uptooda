function UploadFile(fileName, options) {
    local apiKey = ServerParams.getParam("apiKey");
    if (apiKey == "") {
        WriteLog("error", "[radikal.cloud] Set the API key in the server settings before uploading.");
        return ResultCode.Failure;
    }

    local displayName = options.getTask().getFileTask().getDisplayName();
    nm.setUrl("https://radikal.cloud/api/1/upload");
    nm.addQueryHeader("X-API-Key", apiKey);
    nm.addPostFieldFile("source", fileName, ExtractFileName(displayName), GetFileMimeType(fileName));
    if (!nm.doUploadMultipartData() || nm.responseCode() < 200 || nm.responseCode() >= 300) {
        WriteLog("error", "[radikal.cloud] Upload failed. Response code: " + nm.responseCode());
        return ResultCode.Failure;
    }

    local response = ParseJSON(nm.responseBody());
    if (typeof response != "table" || !("image" in response) || typeof response.image != "table"
        || !("url_viewer" in response.image) || typeof response.image.url_viewer != "string" || response.image.url_viewer == "") {
        WriteLog("error", "[radikal.cloud] The upload response is missing the image view URL.");
        return ResultCode.Failure;
    }

    local image = response.image;
    options.setViewUrl(image.url_viewer);
    if ("url" in image && typeof image.url == "string") {
        options.setDirectUrl(image.url);
    }
    if ("thumb" in image && typeof image.thumb == "table" && "url" in image.thumb && typeof image.thumb.url == "string") {
        options.setThumbUrl(image.thumb.url);
    }
    if ("data" in response && typeof response.data == "table" && "delete_url" in response.data
        && typeof response.data.delete_url == "string") {
        options.setDeleteUrl(response.data.delete_url);
    }
    return ResultCode.Success;
}

function GetServerParamList() {
    return {
        apiKey = { title = "API key", required = true }
    };
}
