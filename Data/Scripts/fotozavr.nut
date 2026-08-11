const BASE_URL = "https://fotozavr.ru";
const CURLOPT_COOKIELIST = 10135;

// This is the function that performs the upload of the file
// @param string pathToFile 
// @param UploadParams options
// @return int - success(1), failure(0)
function UploadFile(pathToFile, options) {
    local task = options.getTask().getFileTask();
    nm.setCurlOption(CURLOPT_COOKIELIST, "ALL"); // Clear all cookies
    nm.doGet(BASE_URL);
    // Step 1: Upload the file
    nm.setUrl(BASE_URL + "/plugins/uploadify/uploadify.php?u_id=0");
    nm.addPostFieldFile("Filedata", pathToFile, task.getDisplayName(), GetFileMimeType(pathToFile));
    nm.doUploadMultipartData();

    if (nm.responseCode() != 200) {
        WriteLog("error", "[fotozavr.ru] Upload failed. Response code: " + nm.responseCode());
        return ResultCode.Failure;
    }

    local uploadResponse = nm.responseBody();

    // Check if upload was successful
    if (uploadResponse.find("Ошибка: 0") == null) {
        WriteLog("error", "[fotozavr.ru] Upload failed. Server response: " + uploadResponse);
        return ResultCode.Failure;
    }

    // Step 2: Get the links from the after-upload page
    nm.doGet(BASE_URL + "/afterupload/0/no_album:-/private");

    if (nm.responseCode() != 200) {
        WriteLog("error", "[fotozavr.ru] Failed to get links page. Response code: " + nm.responseCode());
        return ResultCode.Failure;
    }

    // Parse the HTML response with Gumbo-query
    local doc = Document(nm.responseBody());
    
    // Extract direct URL (Прямая ссылка)
    local directUrlInputs = doc.find("input[type=text]");
    local directUrl = "";
    local viewUrl = "";
    local thumbnailUrl = "";

    if (directUrlInputs.length() >= 2) {
        // First input should be the view URL (Ссылка)
        viewUrl = directUrlInputs.at(0).attr("value");
        
        // Second input should be the direct URL (Прямая ссылка)
        directUrl = directUrlInputs.at(1).attr("value");
        
        // Generate thumbnail URL by replacing the base path with medium path
        if (directUrl != "") {
            thumbnailUrl = StrReplace(directUrl, "/photos/", "/photos/medium/");
        }
    }

    // Alternative method: use more specific selectors
    if (directUrl == "") {
        // Try to find the direct link more specifically
        local allInputs = doc.find("input[type=text]");
        for (local i = 0; i < allInputs.length(); i++) {
            local inputValue = allInputs.at(i).attr("value");
            if (inputValue.find("/photos/") != null && inputValue.find("medium") == null) {
                directUrl = inputValue;
                break;
            }
        }
        
        // Find view URL
        for (local i = 0; i < allInputs.length(); i++) {
            local inputValue = allInputs.at(i).attr("value");
            if (inputValue.find("/image/") != null && inputValue.find(".html") != null) {
                viewUrl = inputValue;
                break;
            }
        }
        
        // Generate thumbnail URL
        if (directUrl != "") {
            thumbnailUrl = directUrl;
            thumbnailUrl = thumbnailUrl.replace("/photos/", "/photos/medium/");
        }
    }

    if (directUrl == "") {
        WriteLog("error", "[fotozavr.ru] Failed to extract direct URL from response");
        return ResultCode.Failure;
    }

    options.setDirectUrl(directUrl);
    options.setViewUrl(viewUrl);
    options.setThumbUrl(thumbnailUrl);

    return ResultCode.Success;
}