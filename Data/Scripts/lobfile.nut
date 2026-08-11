const BASE_URL = "https://lobfile.com";
const API_VERSION = "v3";

function _PrintError(t, txt) {
    local errorMessage = "[lobfile.com]  " + txt + " Response code: " + nm.responseCode();
    if (t != null && "errormessage" in t) {
        errorMessage += "\n" + t.errormessage;
    }
    WriteLog("error", errorMessage);
}

// This is the function that performs the upload of the file
// @param string pathToFile 
// @param UploadParams options
// @return int - success(1), failure(0)
function UploadFile(pathToFile, options) {
    local task = options.getTask().getFileTask();
    local apiKey = ServerParams.getParam("apiKey");

    if (apiKey == "") {
        WriteLog("error", "[lobfile.com] API key not set!");
        return ResultCode.Failure;
    }

    // Set upload URL
    nm.setUrl(BASE_URL + "/api/" + API_VERSION + "/upload.php");
    
    // Add authentication header
    nm.addQueryHeader("X-API-Key", apiKey);
    
    // Add file as multipart form data
    nm.addPostFieldFile("file", pathToFile, task.getDisplayName(), GetFileMimeType(pathToFile));
    
    // Optional: Add SHA256 verification if enabled
    local enableSHA256 = ServerParams.getParam("enableSHA256");
    if (enableSHA256 == "1") {
        // Calculate SHA256 hash of the file if needed
        nm.addPostField("sha_256", Sha256FromFile(pathToFile, 0, 0));
    }
    
    // Perform multipart upload
    nm.doUploadMultipartData();
    local response = ParseJSON(nm.responseBody());
    if (nm.responseCode() != 200) {
        _PrintError(response, "Upload failed.");
        return ResultCode.Failure;
    }

    // Parse JSON response
    if (response == null) {
        WriteLog("error", "[lobfile.com] Invalid JSON response");
        return ResultCode.Failure;
    }

    if (!("success" in response) || !response.success) {
        _PrintError(response, "Upload failed.");
        return ResultCode.Failure;
    }

    if (!("url" in response)) {
        WriteLog("error", "[lobfile.com] No URL in response");
        return ResultCode.Failure;
    }

    // Set URLs for the upload result
    options.setDirectUrl(response.url);
    //options.setViewUrl(fileUrl);

    return ResultCode.Success;
}

// Authenticating on remote server (optional function)
// For lobfile.com, authentication is done via API key in headers
// @return int - success(1), failure(0) 
function Authenticate() {
    local apiKey = ServerParams.getParam("apiKey");
    
    if (apiKey == "") {
        WriteLog("error", "[lobfile.com] API key not set");
        return ResultCode.Failure;
    }

    // Test API key by getting account info
    nm.setUrl(BASE_URL + "/api/" + API_VERSION + "/rest/get-account-info");
    nm.addQueryHeader("X-API-Key", apiKey);
    nm.doGet("");

    if (nm.responseCode() != 200) {
        WriteLog("error", "[lobfile.com] Authentication failed. Response code: " + nm.responseCode());
        return ResultCode.Failure;
    }

    local response = ParseJSON(nm.responseBody());
    if (response == null || !("success" in response) || !response.success) {
        local errorMsg = response != null && "error" in response ? response.error : "Authentication failed";
        WriteLog("error", "[lobfile.com] " + errorMsg);
        return ResultCode.Failure;
    }

    return ResultCode.Success;
}

// Required function to define server parameters
function GetServerParamList() {
    return {
        apiKey = "API Key",
        enableSHA256 = {
            title = "Enable SHA256 verification",
            type = "boolean"
        }
    }
}