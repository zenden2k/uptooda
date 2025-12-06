const BASE_URL = "https://anonpic.net";

// This is the function that performs the upload of the file
// @param string pathToFile 
// @param UploadParams options
// @return int - success(1), failure(0)
function UploadFile(pathToFile, options) {
    nm.doGet(BASE_URL + "/");
    local doc = Document(nm.responseBody());
    local csrfToken = doc.find("input[name=\"csrf_token\"]").attr("value");

    if (csrfToken == "") {
        WriteLog("error", "[anonpic.net] Failed to obtain CSRF token. Response code: " + nm.responseCode());
        return ResultCode.Failure;
    }
    
    local task = options.getTask().getFileTask();
    
    // Set up the upload URL
    nm.setUrl(BASE_URL + "/");
    nm.addQueryParamFile("image", pathToFile, task.getDisplayName(), GetFileMimeType(pathToFile));
    nm.addQueryParam("csrf_token", csrfToken); 

    nm.doUploadMultipartData();
    
    if (nm.responseCode() != 200) {
        WriteLog("error", "[anonpic.net] Upload failed. Response code: " + nm.responseCode());
        return ResultCode.Failure;
    }
    
    local bbCodeReg = CRegExp("\\[IMG\\]([^\\]]+)\\[/IMG\\]", "mi");

    if (bbCodeReg.match(nm.responseBody())) {
        options.setDirectUrl(bbCodeReg.getMatch(1));
        return ResultCode.Success;
    }

    WriteLog("error", "[anonpic.net] Upload failed. Cannot obtain the direct URL!");
    return ResultCode.Failure;
}