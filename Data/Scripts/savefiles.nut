const BASE_URL = "https://savefiles.com/api";

function _PrintError(t, txt) {
    local errorMessage = "[savefiles.com] " + txt;
    if (t != null && "msg" in t) {
        errorMessage += " Message: " + t.msg;
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
        WriteLog("error", "[savefiles.com] API key not set!");
        return ResultCode.Failure;
    }

    // Get upload server URL
    nm.doGet(BASE_URL + "/upload/server?key=" + nm.urlEncode(apiKey));

    // Parse JSON response to get upload server
    local serverResponse = ParseJSON(nm.responseBody());
    if (serverResponse == null) {
        WriteLog("error", "[savefiles.com] Failed to parse server response");
        return ResultCode.Failure;
    }

    if (serverResponse.status != 200) {
        _PrintError(serverResponse, "Failed to get upload server");
        return ResultCode.Failure;
    }

    local uploadServer = serverResponse.result;
    if (uploadServer == "") {
        WriteLog("error", "[savefiles.com] Empty upload server URL");
        return ResultCode.Failure;
    }

    // Upload file to the server
    nm.setUrl(uploadServer);
    nm.addPostField("key", apiKey);
    
    local folderId = options.getFolderID();
    if (folderId != "" && folderId != "0") {
        nm.addPostField("fld_id", folderId);
    }

    local isPublic = ServerParams.getParam("isPublic");
    if (isPublic != "") {
        nm.addPostField("file_public", isPublic == "1" ? "1" : "0");
    }

    local isAdult = ServerParams.getParam("isAdult");
    if (isAdult != "") {
        nm.addPostField("file_adult", isAdult == "1" ? "1" : "0");
    }

    local fileTitle = ServerParams.getParam("fileTitle");
    if (fileTitle != "") {
        nm.addPostField("file_title", fileTitle);
    }

    local fileDescription = ServerParams.getParam("fileDescription");
    if (fileDescription != "") {
        nm.addPostField("file_descr", fileDescription);
    }

    local tags = ServerParams.getParam("tags");
    if (tags != "") {
        nm.addPostField("tags", tags);
    }

    nm.addPostFieldFile("file", pathToFile, task.getDisplayName(), GetFileMimeType(pathToFile));
    nm.doUploadMultipartData();

    // Parse upload response
    local uploadResponse = ParseJSON(nm.responseBody());
    if (uploadResponse == null) {
        WriteLog("error", "[savefiles.com] Failed to parse upload response");
        return ResultCode.Failure;
    }

    if (uploadResponse.status != 200) {
        _PrintError(uploadResponse, "Upload failed");
        return ResultCode.Failure;
    }

    if (!("files" in uploadResponse) || uploadResponse.files.len() == 0) {
        _PrintError(uploadResponse, "No files in upload response");
        return ResultCode.Failure;
    }

    local uploadedFile = uploadResponse.files[0];
    if (uploadedFile.status != "OK") {
        WriteLog("error", "[savefiles.com] File upload status: " + uploadedFile.status);
        return ResultCode.Failure;
    }

    local fileCode = uploadedFile.filecode;
    if (fileCode == "") {
        WriteLog("error", "[savefiles.com] Empty file code in response");
        return ResultCode.Failure;
    }

    // Get file info to obtain URLs
    nm.doGet(BASE_URL + "/file/info?key=" + nm.urlEncode(apiKey) + "&file_code=" + nm.urlEncode(fileCode));

    local infoResponse = ParseJSON(nm.responseBody());
    if (infoResponse == null) {
        WriteLog("error", "[savefiles.com] Failed to parse file info response");
        return ResultCode.Failure;
    }

    if (infoResponse.status != 200) {
        _PrintError(infoResponse, "Failed to get file info");
        return ResultCode.Failure;
    }

    if (!("result" in infoResponse) || infoResponse.result.len() == 0) {
        _PrintError(infoResponse, "No file info in response");
        return ResultCode.Failure;
    }

    local fileInfo = infoResponse.result[0];
    
    // Construct URLs
    local viewUrl = "https://savefiles.com/" + fileCode;
    local thumbnailUrl = "";
    
    if ("player_img" in fileInfo && fileInfo.player_img != "") {
        thumbnailUrl = fileInfo.player_img;
    }

    options.setViewUrl(viewUrl);
    options.setThumbUrl(thumbnailUrl);

    return ResultCode.Success;
}

// Authenticating on remote server (optional function)
// @return int - success(1), failure(0)
function Authenticate() {
    local apiKey = ServerParams.getParam("apiKey");
    if (apiKey == "") {
        WriteLog("error", "[savefiles.com] API key not set!");
        return ResultCode.Failure;
    }

    // Test API key by getting account info
    nm.doGet(BASE_URL + "/account/info?key=" + nm.urlEncode(apiKey));

    local response = ParseJSON(nm.responseBody());
    if (response == null) {
        WriteLog("error", "[savefiles.com] Failed to parse authentication response");
        return ResultCode.Failure;
    }

    if (response.status != 200) {
        _PrintError(response, "Authentication failed");
        return ResultCode.Failure;
    }

    return ResultCode.Success;
}

// Optional function:
// Retrieving folder (album) list from server
// @var CFolderList list
// @return int - success(1), failure(0)
function GetFolderList(list) {
    local apiKey = ServerParams.getParam("apiKey");
    if (apiKey == "") {
        WriteLog("error", "[savefiles.com] API key not set!");
        return ResultCode.Failure;
    }

    local parentId = list.parentFolder().getId();
    if (parentId == "") {
        // Add root folder
        local rootFolder = CFolderItem();
        rootFolder.setId("0");
        rootFolder.setTitle("Root");
        list.AddFolderItem(rootFolder);
        return ResultCode.Success;
    }

    nm.doGet(BASE_URL + "/folder/list?key=" + nm.urlEncode(apiKey) + "&fld_id=" + nm.urlEncode(parentId));

    local response = ParseJSON(nm.responseBody());
    if (response == null) {
        WriteLog("error", "[savefiles.com] Failed to parse folder list response");
        return ResultCode.Failure;
    }

    if (response.status != 200) {
        _PrintError(response, "Failed to get folder list");
        return ResultCode.Failure;
    }

    if ("result" in response && "folders" in response.result) {
        local folders = response.result.folders;
        for (local i = 0; i < folders.len(); i++) {
            local folder = folders[i];
            local folderItem = CFolderItem();
            folderItem.setId(folder.fld_id);
            folderItem.setTitle(folder.name);
            folderItem.setParentId(parentId);
            folderItem.setViewUrl("https://savefiles.com/?op=my_files&fld_id=" + folder.fld_id);
            list.AddFolderItem(folderItem);
        }
    }

    return ResultCode.Success;
}

// Create a folder or an album (optional function)
// @var CFolderItem parentAlbum
// @var CFolderItem album
// @return int - success(1), failure(0)
function CreateFolder(parentAlbum, album) {
    local apiKey = ServerParams.getParam("apiKey");
    if (apiKey == "") {
        WriteLog("error", "[savefiles.com] API key not set!");
        return ResultCode.Failure;
    }

    local parentId = parentAlbum.getId();
    local folderName = album.getTitle();
    local folderDescription = album.getSummary();

    local url = BASE_URL + "/folder/create?key=" + nm.urlEncode(apiKey) + 
              "&name=" + nm.urlEncode(folderName) + 
              "&parent_id=" + nm.urlEncode(parentId);
    
    if (folderDescription != "") {
        url += "&descr=" + nm.urlEncode(folderDescription);
    }

    nm.doGet(url);

    local response = ParseJSON(nm.responseBody());
    if (response == null) {
        WriteLog("error", "[savefiles.com] Failed to parse create folder response");
        return ResultCode.Failure;
    }

    if (response.status != 200) {
        _PrintError(response, "Failed to create folder");
        return ResultCode.Failure;
    }

    if ("result" in response && "fld_id" in response.result) {
        album.setId(response.result.fld_id);
        return ResultCode.Success;
    }

    return ResultCode.Failure;
}

// Modify a folder or an album (update name, description) (optional function)
// @var CFolderItem album
// @return int - success(1), failure(0)
function ModifyFolder(album) {
    local apiKey = ServerParams.getParam("apiKey");
    if (apiKey == "") {
        WriteLog("error", "[savefiles.com] API key not set!");
        return ResultCode.Failure;
    }

    local folderId = album.getId();
    local folderName = album.getTitle();
    local folderDescription = album.getSummary();

    if (folderId == "0" || folderId == "") {
        WriteLog("error", "[savefiles.com] You cannot edit root folder!");
        return ResultCode.Failure;
    }
    local url = BASE_URL + "/folder/edit?key=" + nm.urlEncode(apiKey) + 
              "&fld_id=" + nm.urlEncode(folderId) + 
              "&name=" + nm.urlEncode(folderName);

    
    if (folderDescription != "") {
        url += "&descr=" + nm.urlEncode(folderDescription);
    }

    nm.doGet(url);

    local response = ParseJSON(nm.responseBody());
    if (response == null) {
        WriteLog("error", "[savefiles.com] Failed to parse modify folder response");
        return ResultCode.Failure;
    }

    if (response.status != 200) {
        _PrintError(response, "Failed to modify folder");
        return ResultCode.Failure;
    }

    return ResultCode.Success;
}

// optional function
function GetServerParamList() {
    return {
        apiKey = { title = "API key", required = true },
        fileTitle = "File Title",
        fileDescription = "File Description",
        tags = "Tags (comma separated)",
        isPublic = {
            title = "Public File",
            type = "boolean"
        },
        isAdult = {
            title = "Adult Content",
            type = "boolean"
        }
    };
}