const BASE_URL = "https://thumbsnap.com";

function _PrintError(t, txt) {
    local errorMessage = "[thumbsnap.com] " + txt + " Response code: " + nm.responseCode();
    if (t != null && "error" in t && "message" in t.error) {
        errorMessage += "\n" + t.error.message;
    }
    WriteLog("error", errorMessage);
}

// This is the function that performs the upload of the file
// @param string pathToFile 
// @param UploadParams options
// @return int - success(1), failure(0)
function UploadFile(pathToFile, options) {
    local task = options.getTask().getFileTask();
    local apiKey = ServerParams.getParam("Password");

    if (apiKey == "") {
        WriteLog("error", "[thumbsnap.com] API key not set!");
        return ResultCode.Failure;
    }

    // Set upload URL
    nm.setUrl(BASE_URL + "/api/upload");
    
    // Add API key as POST parameter
    nm.addPostField("key", apiKey);
    
    // Add the file as multipart/form-data
    nm.addPostFieldFile("media", pathToFile, task.getDisplayName(), GetFileMimeType(pathToFile));
    
    // Perform multipart upload
    nm.doUploadMultipartData();

    local t = ParseJSON(nm.responseBody());
    
    if (nm.responseCode() != 200) {
        _PrintError(t, "Upload failed");
        return ResultCode.Failure;
    }

    if (t == null) {
        WriteLog("error", "[thumbsnap.com] Failed to parse server response as JSON");
        return ResultCode.Failure;
    }

    // Check if upload was successful
    if (!("success" in t) || !t.success) {
        _PrintError(t, "Upload failed");
        return ResultCode.Failure;
    }

    // Extract URLs from response
    if (!("data" in t)) {
        _PrintError(t, "Invalid response format - missing data field");
        return ResultCode.Failure;
    }

    local data = t.data;
    local directUrl = "";
    local viewUrl = "";
    local thumbnailUrl = "";

    // Get the direct media URL
    if ("media" in data) {
        directUrl = data.media;
    }

    // Get the view URL (page URL)
    if ("url" in data) {
        viewUrl = data.url;
    }

    // Get the thumbnail URL
    if ("thumb" in data) {
        thumbnailUrl = data.thumb;
    }

    if (directUrl == "") {
        WriteLog("error", "[thumbsnap.com] Upload failed. Cannot obtain the direct URL!");
        return ResultCode.Failure;
    }

    // Set the URLs in options
    options.setDirectUrl(directUrl);
    options.setViewUrl(viewUrl);
    options.setThumbUrl(thumbnailUrl);

    return ResultCode.Success;
}