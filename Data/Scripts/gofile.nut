/**
 * GoFile.io Image Uploader Script
 * Official API Documentation: https://gofile.io/api
 * Updated: May 16, 2025
 */

const UPLOAD_URL = "https://upload.gofile.io/uploadfile";
const API_BASE_URL = "https://api.gofile.io";

function _PrintError(t, txt) {
    local errorMessage = "[gofile.io] " + txt + " Response code: " + nm.responseCode();
    if (t != null && "message" in t) {
        errorMessage += "\n" + t.message;
    } else if (t != null && "error" in t) {
        errorMessage += "\n" + t.error;
    }
    WriteLog("error", errorMessage);
}

/** 
 * This is the function that performs the upload of the file
 * @param string pathToFile
 * @param UploadParams options
 * @return int - success(1), failure(0)
 */
function UploadFile(pathToFile, options) {
    /** 
     * @var task
     * @type FileUploadTaskWrapper
     */
    local task = options.getTask().getFileTask();
    
    // Get API token if provided (for authenticated uploads)
    local apiToken = ServerParams.getParam("Password");
    local folderId = ""; //options.getFolderID();
    local uploadRegion = ServerParams.getParam("uploadRegion");
    
    // Determine upload URL based on region preference
    local uploadUrl = UPLOAD_URL;
    if (uploadRegion != "" && uploadRegion != "auto") {
        uploadUrl = "https://upload-" + uploadRegion + ".gofile.io/uploadfile";
    }

    nm.setUrl(uploadUrl);

    // Add authentication header if token is provided
    if (apiToken != "") {
        nm.addQueryHeader("Authorization", "Bearer " + apiToken);
    }

    // Add the file (required parameter)
    nm.addPostFieldFile("file", pathToFile, task.getDisplayName(), GetFileMimeType(pathToFile));

    // Add optional folderId if specified
    if (folderId != "") {
        nm.addPostField("folderId", folderId);
    }

    // Perform multipart upload
    if (!nm.doUploadMultipartData()) {
        return ResultCode.Failure;
    }

    // Parse response regardless of HTTP status code as required
    local t = ParseJSON(nm.responseBody());

    if (t == null) {
        WriteLog("error", "[gofile.io] Invalid JSON response");
        return ResultCode.Failure;
    }

    // Check if upload was successful
    if (!("status" in t) || t.status != "ok") {
        _PrintError(t, "Upload failed");
        return ResultCode.Failure;
    }

    if (!("data" in t)) {
        _PrintError(t, "Upload failed - no data in response");
        return ResultCode.Failure;
    }

    local data = t.data;
    local fileId = "";
    local downloadPage = "";
    local directUrl = "";
    local guestToken = "";

    // Extract file information from response
    if ("fileId" in data) {
        fileId = data.fileId;
    }
    if ("downloadPage" in data) {
        downloadPage = data.downloadPage;
    }
    if ("directLink" in data) {
        directUrl = data.directLink;
    }
    if ("guestToken" in data) {
        guestToken = data.guestToken;
        // Store guest token for future uploads to same folder
        ServerParams.setParam("guestToken", guestToken);
    }
    if ("parentFolder" in data) {
        // Store parent folder ID for future uploads
        ServerParams.setParam("lastFolderId", data.parentFolder);
    }

    // Set URLs for the application
    options.setViewUrl(downloadPage);
    options.setDirectUrl(directUrl);

    return ResultCode.Success;
}

/** 
 * Authenticating on remote server (optional function)
 * GoFile.io supports both authenticated and anonymous uploads
 * @return int - success(1), failure(0)
 */
function Authenticate() {
    local userName = ServerParams.getParam("Login");
    local apiToken = ServerParams.getParam("Password");
    
    if (apiToken == "") {
        if (userName != "") {
            WriteLog("error", "[gofile.io] You must set API token.");
            return ResultCode.Failure;
        }
        // No token provided - will use anonymous upload
        return ResultCode.Success;
    }

    // Test the API token by getting account ID
    nm.addQueryHeader("Authorization", "Bearer " + apiToken);
    nm.doGet(API_BASE_URL + "/accounts/getid");

    local t = ParseJSON(nm.responseBody());
    
    if (t != null && "status" in t && t.status == "ok") {
        return ResultCode.Success;
    } else {
        _PrintError(t, "Authentication failed - invalid API token");
        return ResultCode.Failure;
    }
}

function _ObtainRootFolder(apiToken) {
    // Get account ID first
    nm.addQueryHeader("Authorization", "Bearer " + apiToken);
    nm.doGet(API_BASE_URL + "/accounts/getid");

    local accountResponse = ParseJSON(nm.responseBody());
    if (accountResponse == null || !("status" in accountResponse) || accountResponse.status != "ok") {
        _PrintError(accountResponse, "Failed to get account ID");
        return "";
    }

    local accountId = accountResponse.data.id;
    
    // Get account details to find root folder
    nm.addQueryHeader("Authorization", "Bearer " + apiToken);
    nm.doGet(API_BASE_URL + "/accounts/" + accountId);

    local detailsResponse = ParseJSON(nm.responseBody());
    if (detailsResponse == null || !("status" in detailsResponse) || detailsResponse.status != "ok") {
        _PrintError(detailsResponse, "Failed to get account details");
        return "";
    }
    return detailsResponse.data.rootFolder;
}

/** 
 * Optional function:
 * Retrieving folder (album) list from server
 * This requires premium account and authenticated access
 * @var CFolderList list
 * @return int - success(1), failure(0)
 */
function GetFolderList(list) {
    local parentId = list.parentFolder().getId();
    local apiToken = ServerParams.getParam("Password");

    if (parentId == "") {
        local rootFolderId = _ObtainRootFolder(apiToken);
        // Add root folder
        local rootFolder = CFolderItem();
        rootFolder.setId(rootFolderId);
        rootFolder.setTitle("Root (/)");
        list.AddFolderItem(rootFolder);
    } else {
        nm.addQueryHeader("Authorization", "Bearer " + apiToken);
        nm.doGet(API_BASE_URL + "/contents/" + parentId);
    }

    return ResultCode.Success;
}

/** 
 * Create a folder (optional function)
 * Requires API token and premium account
 * @var CFolderItem parentAlbum
 * @var CFolderItem album
 * @return int - success(1), failure(0)
 */
function CreateFolder(parentAlbum, album) {
    local apiToken = ServerParams.getParam("Password");
    
    if (apiToken == "") {
        WriteLog("error", "[gofile.io] API token required for folder creation");
        return ResultCode.Failure;
    }

    local parentFolderId = parentAlbum.getId();
    
    if (parentFolderId == "") {
        parentFolderId = _ObtainRootFolder(apiToken);
    }

    local folderName = album.getTitle();

    nm.setUrl(API_BASE_URL + "/contents/createFolder");
    nm.addQueryHeader("Authorization", "Bearer " + apiToken);
    nm.addQueryHeader("Content-Type", "application/json");

    local requestData = {
        parentFolderId = parentFolderId
    };
    
    if (folderName != "") {
        requestData.folderName <- folderName;
    }

    nm.doPost(ToJSON(requestData));

    local t = ParseJSON(nm.responseBody());
    if (t != null && "status" in t && t.status == "ok" && "data" in t) {
        album.setId(t.data.id);
        album.setTitle(t.data.name);
        album.setViewUrl("https://gofile.io/d/" + t.data.code);
        return ResultCode.Success;
    } else {
        _PrintError(t, "Failed to create folder");
        return ResultCode.Failure;
    }
}

/** 
 * Modify a folder (update name, description) (optional function)
 * @var CFolderItem album
 * @return int - success(1), failure(0)
 */
function ModifyFolder(album) {
    local apiToken = ServerParams.getParam("Password");
    
    if (apiToken == "") {
        WriteLog("error", "[gofile.io] API token required for folder modification");
        return ResultCode.Failure;
    }

    local folderId = album.getId();
    local folderName = album.getTitle();

    nm.setUrl(API_BASE_URL + "/contents/" + folderId + "/update");
    nm.addQueryHeader("Authorization", "Bearer " + apiToken);
    nm.addQueryHeader("Content-Type", "application/json");

    local requestData = {
        attribute = "name",
        attributeValue = folderName
    };

    nm.doPut(ToJSON(requestData));

    local t = ParseJSON(nm.responseBody());
    if (t != null && "status" in t && t.status == "ok") {
        return ResultCode.Success;
    } else {
        _PrintError(t, "Failed to modify folder");
        return ResultCode.Failure;
    }
}

/** 
 * A function that returns a list of types of access restrictions to an album
 * @return array
 */
function GetFolderAccessTypeList() {
    return ["Public", "Private"];
}

/** 
 * Define server parameters that user can configure
 * @return table
 */
function GetServerParamList() {
    return {
        uploadRegion = {
            title = "Upload Region",
            type = "choice",
            items = [
                {
                    id = "auto",
                    label = "Automatic (Closest Region)"
                },
                {
                    id = "eu-par",
                    label = "Europe (Paris)"
                },
                {
                    id = "na-phx",
                    label = "North America (Phoenix)"
                },
                {
                    id = "ap-sgp",
                    label = "Asia Pacific (Singapore)"
                },
                {
                    id = "ap-hkg",
                    label = "Asia Pacific (Hong Kong)"
                },
                {
                    id = "ap-tyo",
                    label = "Asia Pacific (Tokyo)"
                },
                {
                    id = "sa-sao",
                    label = "South America (São Paulo)"
                }
            ]
        }
    }
}