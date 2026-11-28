const UPLOAD_URL = "https://api.imgpx.com/v1/upload";

function UploadFile(pathToFile, options) {
    local apiKey = ServerParams.getParam("apiKey");
    if (apiKey == "") {
        WriteLog("error", "[imgpx.com] Set the API key in the server settings before uploading.");
        return ResultCode.Failure;
    }

    local displayName = options.getTask().getFileTask().getDisplayName();
    nm.setUrl(UPLOAD_URL);
    nm.addQueryHeader("Authorization", "Bearer " + apiKey);
    nm.addPostFieldFile("image", pathToFile, displayName, GetFileMimeType(pathToFile));
    nm.addPostField("name", displayName);

    if (!nm.doUploadMultipartData()) {
        WriteLog("error", "[imgpx.com] Upload request failed. Response code: " + nm.responseCode());
        return ResultCode.Failure;
    }

    local responseCode = nm.responseCode();
    local response = ParseJSON(nm.responseBody());
    if (responseCode != 200 || typeof response != "table" || !("success" in response) || response.success != true) {
        local message = "[imgpx.com] Upload failed. Response code: " + responseCode;
        if (typeof response == "table" && "error" in response && typeof response.error == "table"
            && "message" in response.error && typeof response.error.message == "string") {
            message += ". " + response.error.message;
        }
        WriteLog("error", message);
        return ResultCode.Failure;
    }

    if (!("data" in response) || typeof response.data != "table"
        || !("url" in response.data) || typeof response.data.url != "string" || response.data.url == "") {
        WriteLog("error", "[imgpx.com] Upload response is missing the direct image URL.");
        return ResultCode.Failure;
    }

    local data = response.data;
    options.setDirectUrl(data.url);
    if ("page" in data && typeof data.page == "string") {
        options.setViewUrl(data.page);
    }
    if ("thumb" in data && typeof data.thumb == "string") {
        options.setThumbUrl(data.thumb);
    }
    if ("delete_url" in data && typeof data.delete_url == "string") {
        options.setDeleteUrl(data.delete_url);
    }

    return ResultCode.Success;
}

function GetServerParamList() {
    return {
        apiKey = { title = "API key", required = true }
    };
}
