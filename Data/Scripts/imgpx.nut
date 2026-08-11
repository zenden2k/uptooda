const BASE_URL = "https://imgpx.com";

function _PrintError(t, txt) {
    local errorMessage = "[imgpx.com] " + txt + " Response code: " + nm.responseCode();
    if (t != null && "error" in t) {
        errorMessage += "\n" + t.error;
    }
    WriteLog("error", errorMessage);
}

// This is the function that performs the upload of the file
// @param string pathToFile 
// @param UploadParams options
// @return int - success(1), failure(0)
function UploadFile(pathToFile, options) {
    local task = options.getTask().getFileTask();
    
    // Get upload token from the main page
    nm.doGet(BASE_URL + "/en/");
    
    if (nm.responseCode() != 200) {
        WriteLog("error", "[imgpx.com] Failed to access main page. Response code: " + nm.responseCode());
        return ResultCode.Failure;
    }
    
    // Parse HTML to get upload token
    local doc = Document(nm.responseBody());
    local uploadToken = doc.find("input[name=upload_token]").attr("value");
    
    if (uploadToken == "") {
        WriteLog("error", "[imgpx.com] Cannot obtain upload token.");
        return ResultCode.Failure;
    }
    
    // Prepare upload request
    nm.setUrl(BASE_URL + "/en/uploads");
    nm.addPostFieldFile("fileToUpload[]", pathToFile, task.getDisplayName(), GetFileMimeType(pathToFile));
    nm.addPostField("upload_token", uploadToken);
    nm.addPostField("photoSize", "original"); // Use original size as requested
    nm.addPostField("submit", "Upload"); // Submit button value
    nm.doUploadMultipartData();
    
    if (nm.responseCode() != 200) {
        WriteLog("error", "[imgpx.com] Upload failed. Response code: " + nm.responseCode());
        return ResultCode.Failure;
    }
    
    // Parse response to extract URLs
    local responseBody = nm.responseBody();
    local doc = Document(responseBody);
    
    // Extract direct link to image
    local directUrlInput = doc.find("div.link-container:contains('Direct link') input");
    local directUrl = "";
    if (directUrlInput.length() > 0) {
        directUrl = directUrlInput.attr("value");
    }
    
    // Extract short link (view URL)
    local viewUrlInput = doc.find("div.link-container:contains('Short link:') input");
    local viewUrl = "";
    if (viewUrlInput.length() > 0) {
        viewUrl = viewUrlInput.attr("value");
    }
    
    // Extract thumbnail URL from BB code
    local thumbnailUrl = "";
    local thumbnailInput = doc.find("div.link-container:contains('BB code for image with preview (320x240):') input");
    if (thumbnailInput.length() > 0) {
        local bbCode = thumbnailInput.attr("value");
        local reg = CRegExp("\\[img\\](.+?)\\[/img\\]", "mi");
        if (reg.match(bbCode)) {
            thumbnailUrl = reg.getMatch(1);
        }
    }
    
    if (directUrl == "") {
        local error = doc.find(".error-container p").text();
        if (error != "") {
            WriteLog("error", "[imgpx.com] " + error);
        } else {
            WriteLog("error", "[imgpx.com] Upload failed. Cannot obtain the direct URL!");
        }

        return ResultCode.Failure;
    }
    
    options.setDirectUrl(directUrl);
    options.setViewUrl(viewUrl);
    options.setThumbUrl(thumbnailUrl);
    
    return ResultCode.Success;
}

// Authenticating on remote server (optional function)
// @return int - success(1), failure(0) 
function Authenticate() {
    local username = ServerParams.getParam("Login");
    local password = ServerParams.getParam("Password");
    
    if (username == "" || password == "") {
        WriteLog("error", "[imgpx.com] Username or password not set!");
        return ResultCode.Failure;
    }
    
    nm.setUrl(BASE_URL + "/en/login");
    nm.addPostField("username", username);
    nm.addPostField("password", password);
    nm.addPostField("submit", "Log In"); // Submit button value
    nm.doPost("");
    
    if (nm.responseCode() != 200) {
        WriteLog("error", "[imgpx.com] Authentication failed. Response code: " + nm.responseCode());
        return ResultCode.Failure;
    }
    
    // Check if login was successful by looking for redirect or success indicators
    local responseBody = nm.responseBody();
    if (responseBody.find("Invalid login or password") != null) {
        WriteLog("error", "[imgpx.com] Authentication failed. Invalid credentials.");
        return ResultCode.Failure;
    }
    
    return ResultCode.Success;
}