const API_BASE_URL = "https://api.1fichier.com/v1";

function _LogError(message, response = null) {
    local errorMessage = "[1fichier.com] " + message + " Response code: " + nm.responseCode();
    if (response != null) {
        if ("message" in response && response.message != "") {
            errorMessage += "\n" + response.message;
        } else if ("Message" in response && response.Message != "") {
            errorMessage += "\n" + response.Message;
        }
    }
    WriteLog("error", errorMessage);
}

function _GetApiKey() {
    local apiKey = ServerParams.getParam("apiKey");
    if (apiKey == "") {
        WriteLog("error", "[1fichier.com] API key is not set.");
    }
    return apiKey;
}

function _AddApiHeaders(apiKey) {
    if (apiKey != "") {
        nm.addQueryHeader("Authorization", "Bearer " + apiKey);
    }
    nm.addQueryHeader("Content-Type", "application/json");
}

function _PostApi(path, apiKey, request, errorMessage) {
    nm.setUrl(API_BASE_URL + path);
    _AddApiHeaders(apiKey);
    if (!nm.doPost(ToJSON(request)) || nm.responseCode() != 200) {
        _LogError(errorMessage);
        return null;
    }

    local response = ParseJSON(nm.responseBody());
    if (response == null) {
        _LogError(errorMessage + " The server returned invalid JSON.");
        return null;
    }
    return response;
}

function UploadFile(pathToFile, options) {
    local apiKey = ServerParams.getParam("apiKey");

    _AddApiHeaders(apiKey);
    if (!nm.doGet(API_BASE_URL + "/upload/get_upload_server.cgi") || nm.responseCode() != 200) {
        _LogError("Failed to obtain an upload server.");
        return ResultCode.Failure;
    }

    local uploadServer = ParseJSON(nm.responseBody());
    if (uploadServer == null || !("url" in uploadServer) || !("id" in uploadServer)
        || uploadServer.url == "" || uploadServer.id == "") {
        _LogError("The upload-server response is incomplete.", uploadServer);
        return ResultCode.Failure;
    }

    local task = options.getTask().getFileTask();
    local folderId = options.getFolderID();
    if (apiKey == "" || folderId == "") {
        folderId = "0";
    }

    nm.setUrl("https://" + uploadServer.url + "/upload.cgi?id=" + nm.urlEncode(uploadServer.id));
    if (apiKey != "") {
        nm.addQueryHeader("Authorization", "Bearer " + apiKey);
    }
    nm.addPostField("did", folderId);
    nm.addPostFieldFile("file[]", pathToFile, task.getDisplayName(), GetFileMimeType(pathToFile));
    if (!nm.doUploadMultipartData() || nm.responseCode() != 200) {
        _LogError("Failed to upload the file.");
        return ResultCode.Failure;
    }

    if (apiKey != "") {
        nm.addQueryHeader("Authorization", "Bearer " + apiKey);
    }
    nm.addQueryHeader("JSON", "1");
    local finishUrl = "https://" + uploadServer.url + "/end.pl?xid=" + nm.urlEncode(uploadServer.id);
    if (!nm.doGet(finishUrl) || nm.responseCode() != 200) {
        _LogError("Failed to finish the upload.");
        return ResultCode.Failure;
    }

    local result = ParseJSON(nm.responseBody());
    if (result == null || !("links" in result) || result.links.len() == 0
        || !("download" in result.links[0]) || result.links[0].download == "") {
        _LogError("The upload result does not contain a download URL.", result);
        return ResultCode.Failure;
    }

    local link = result.links[0];
    options.setViewUrl(link.download);
    if ("remove" in link && link.remove != "") {
        options.setDeleteUrl(link.remove);
    }
    return ResultCode.Success;
}

function Authenticate() {
    local apiKey = _GetApiKey();
    if (apiKey == "") {
        return ResultCode.Failure;
    }

    local response = _PostApi("/folder/ls.cgi", apiKey, { folder_id = 0 }, "Authentication failed.");
    if (response == null || !("status" in response) || response.status != "OK") {
        _LogError("Authentication failed.", response);
        return ResultCode.Failure;
    }
    return ResultCode.Success;
}

function GetFolderList(list) {
    local apiKey = _GetApiKey();
    if (apiKey == "") {
        return ResultCode.Failure;
    }

    local parentId = list.parentFolder().getId();
    if (parentId == "") {
        local rootFolder = CFolderItem();
        rootFolder.setId("0");
        rootFolder.setTitle("Root");
        list.AddFolderItem(rootFolder);
        return ResultCode.Success;
    }

    local response = _PostApi("/folder/ls.cgi", apiKey, { folder_id = parentId.tointeger() },
        "Failed to obtain the folder list.");
    if (response == null || !("status" in response) || response.status != "OK"
        || !("sub_folders" in response)) {
        _LogError("The folder-list response is incomplete.", response);
        return ResultCode.Failure;
    }

    foreach (folder in response.sub_folders) {
        if (!("id" in folder) || !("name" in folder)) {
            continue;
        }
        local folderItem = CFolderItem();
        folderItem.setId(folder.id.tostring());
        folderItem.setTitle(folder.name);
        folderItem.setParentId(parentId);
        list.AddFolderItem(folderItem);
    }
    return ResultCode.Success;
}

function CreateFolder(parentFolder, folder) {
    local apiKey = _GetApiKey();
    if (apiKey == "") {
        return ResultCode.Failure;
    }

    local parentId = parentFolder.getId();
    if (parentId == "") {
        parentId = "0";
    }
    local request = {
        name = folder.getTitle(),
        folder_id = parentId.tointeger()
    };
    local response = _PostApi("/folder/mkdir.cgi", apiKey, request, "Failed to create the folder.");
    if (response == null || !("folder_id" in response)) {
        _LogError("The create-folder response is incomplete.", response);
        return ResultCode.Failure;
    }

    folder.setId(response.folder_id.tostring());
    folder.setParentId(parentId);
    if ("name" in response && response.name != "") {
        folder.setTitle(response.name);
    }
    return ResultCode.Success;
}

function ModifyFolder(folder) {
    local apiKey = _GetApiKey();
    if (apiKey == "") {
        return ResultCode.Failure;
    }

    local folderId = folder.getId();
    if (folderId == "" || folderId == "0") {
        WriteLog("error", "[1fichier.com] The root folder cannot be renamed.");
        return ResultCode.Failure;
    }

    local request = {
        folder_id = folderId.tointeger(),
        rename = folder.getTitle()
    };
    local response = _PostApi("/folder/mv.cgi", apiKey, request, "Failed to rename the folder.");
    if (response == null || !("status" in response) || response.status != "OK") {
        _LogError("Failed to rename the folder.", response);
        return ResultCode.Failure;
    }
    if ("new_name" in response && response.new_name != "") {
        folder.setTitle(response.new_name);
    }
    return ResultCode.Success;
}

function GetServerParamList() {
    return {
        apiKey = "API key"
    };
}
