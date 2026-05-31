const BASE_URL = "https://8upload.com";

function _PrintError(t, txt) {
    local errorMessage = "[8upload.com] " + txt + " Response code: " + nm.responseCode();
    if (t != null && "error" in t) {
        errorMessage += "\n" + t.error;
    }
    WriteLog("error", errorMessage);
}

// Authenticating on remote server (optional function)
// @return int - success(1), failure(0) 
function Authenticate() {
    local username = ServerParams.getParam("Login");
    local password = ServerParams.getParam("Password");
    
    if (username == "" || password == "") {
        WriteLog("error", "[8upload.com] Username or password not set!");
        return ResultCode.Failure;
    }
    
    // Step 1: Get CSRF token
    nm.doGet(BASE_URL + "/myaccount/ASLibrary/js/js-bootstrap.php");
    
    if (nm.responseCode() != 200) {
        WriteLog("error", "[8upload.com] Failed to get CSRF token");
        return ResultCode.Failure;
    }
    
    local csrfToken = "";
    local responseBody = nm.responseBody();
    local reg = CRegExp("_data\\[\"_as_csrf_token\"\\]\\s*=\\s*\"([^\"]+)\"", "mi");
    
    if (reg.match(responseBody)) {
        csrfToken = reg.getMatch(1);
    }
    
    if (csrfToken == "") {
        WriteLog("error", "[8upload.com] Cannot obtain CSRF token");
        return ResultCode.Failure;
    }
    
    // Step 2: Perform login
    nm.setUrl(BASE_URL + "/myaccount/ASEngine/ASAjax.php");
    nm.addQueryHeader("X-Requested-With", "XMLHttpRequest");
    nm.addQueryParam("_as_csrf_token", csrfToken);
    nm.addQueryParam("action", "checkLogin");
    nm.addQueryParam("username", username);
    nm.addQueryParam("password", Sha512(password));
    nm.addQueryParam("id[username]", "login-username");
    nm.addQueryParam("id[password]", "login-password");
    nm.doPost("");
    
    if (nm.responseCode() != 200) {
        WriteLog("error", "[8upload.com] Login request failed. Response code: " + nm.responseCode());
        return ResultCode.Failure;
    }
    
    local t = ParseJSON(nm.responseBody());
    if (t != null && "status" in t && t.status == "success") {
        return ResultCode.Success;
    } else {
        local errorMsg = "[8upload.com] Login failed";
        if (t != null && "message" in t) {
            errorMsg += ": " + t.message;
        }
        WriteLog("error", errorMsg);
        return ResultCode.Failure;
    }
}

// This is the function that performs the upload of the file
// @param string pathToFile 
// @param UploadParams options
// @return int - success(1), failure(0)

function UploadFile(pathToFile, options) {
    local task = options.getTask().getFileTask();
    
    // Step 1: Upload file to 8upload.com
    nm.setUrl(BASE_URL + "/upload/mt/");
    nm.addQueryHeader("X-Requested-With", "XMLHttpRequest");
    nm.addQueryParamFile("images[]", pathToFile, task.getDisplayName(), GetFileMimeType(pathToFile));
    nm.doUploadMultipartData();

    // Parse JSON response even if response code is not 200
    local uploadResponse = ParseJSON(nm.responseBody());
    
    if (uploadResponse == "" || uploadResponse == null) {
        _PrintError(null, "Failed to get upload path from response");
        return ResultCode.Failure;
    }
    
    
    // Step 2: Get the page with links
    local pageUrl = BASE_URL + uploadResponse;
    nm.doGet(pageUrl);
    
    if (nm.responseCode() != 200) {
        _PrintError(null, "Failed to get upload page");
        return ResultCode.Failure;
    }
    
    // Step 3: Parse HTML with Gumbo-query to extract links
    local doc = Document(nm.responseBody());

    local directUrl = "";
    local viewUrl = "";
    local thumbnailUrl = "";
    local deleteUrl = "";

    local inputs = doc.find("input[type=text]");
    local inputsCount = inputs.length();
    
    for (local i = 0; i < inputsCount; i++) {
        local input = inputs.at(i);
        local value = input.attr("value");
        WriteLog("error", value);
        if (value.find("[") == null && value.find("<") == null) {
            if (value.find("/image/") != null) {
                directUrl = value;
            } else if (value.find("/view/") != null) {
                viewUrl = value;
            } else if (value.find("/thumbnail/") != null) {
                thumbnailUrl = value;
            } else if (value.find("/remove/") != null) {
                deleteUrl = value;
            }
        } else if (thumbnailUrl == "") {
            local thumbReg = CRegExp("\\[IMG\\](.+?)\\[\\/IMG\\]", "mi");
            if (thumbReg.match(value)) {
                thumbnailUrl = thumbReg.getMatch(1);
            }
        }
    }

    if (directUrl == "") {
        _PrintError(null, "Failed to extract direct URL from response");
        return ResultCode.Failure;
    }
    
    // Set the extracted URLs
    options.setDirectUrl(directUrl);
    options.setViewUrl(viewUrl);
    options.setThumbUrl(thumbnailUrl);
    options.setDeleteUrl(deleteUrl);
    
    return ResultCode.Success;
}