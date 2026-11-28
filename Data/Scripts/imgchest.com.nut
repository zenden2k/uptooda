function UploadFile(fileName, options) {
    local apiToken = ServerParams.getParam("apiToken");
    if (apiToken == "") {
        WriteLog("error", "[imgchest.com] Set the API token in the server settings before uploading.");
        return ResultCode.Failure;
    }

    local displayName = options.getTask().getFileTask().getDisplayName();
    nm.setUrl("https://api.imgchest.com/v1/post");
    nm.addQueryHeader("Authorization", "Bearer " + apiToken);
    nm.addPostFieldFile("images[]", fileName, ExtractFileName(displayName), GetFileMimeType(fileName));
    if (!nm.doUploadMultipartData() || nm.responseCode() < 200 || nm.responseCode() >= 300) {
        WriteLog("error", "[imgchest.com] Upload failed. Response code: " + nm.responseCode());
        return ResultCode.Failure;
    }

    local response = ParseJSON(nm.responseBody());
    if (typeof response != "table" || !("data" in response) || typeof response.data != "table"
        || !("id" in response.data) || typeof response.data.id != "string" || response.data.id == "") {
        WriteLog("error", "[imgchest.com] The upload response is missing the post ID.");
        return ResultCode.Failure;
    }

    local data = response.data;
    options.setViewUrl("https://imgchest.com/p/" + data.id);
    if ("images" in data && typeof data.images == "array" && data.images.len() > 0
        && typeof data.images[0] == "table" && "link" in data.images[0] && typeof data.images[0].link == "string") {
        options.setDirectUrl(data.images[0].link);
    }
    if ("delete_url" in data && typeof data.delete_url == "string") {
        options.setDeleteUrl(data.delete_url);
    }
    return ResultCode.Success;
}

function GetServerParamList() {
    return {
        apiToken = { title = "API token", required = true }
    };
}
