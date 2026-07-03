const API_BASE_URL = "https://dev.opendrive.com/api/v1";
const CHUNK_SIZE = 10485760; // 10 MB

function min(a, b) {
    return a < b ? a : b;
}

function _SessionId() {
    return ServerParams.getParam("sessionId");
}

function _ClearAuthData() {
    ServerParams.setParam("sessionId", "");
}

function _CheckResponse(context, authErrorCode = 0) {
    local code = nm.responseCode();

    if (code >= 200 && code < 300) {
        return 1;
    }

    if (code == 401) {
        _ClearAuthData();
        WriteLog("warning", "[opendrive.com] " + context + " failed: unauthorized");
        return authErrorCode;
    }

    WriteLog("error", "[opendrive.com] " + context + " failed, response code: " + code + "\r\n" + nm.responseBody());
    return 0;
}

function _ParseResponse(context) {
    local response = null;
    try {
        response = ParseJSON(nm.responseBody());
    } catch (ex) {
        WriteLog("error", "[opendrive.com] Unable to parse " + context + " response: " + nm.responseBody());
    }
    return response;
}

function _FolderId(id) {
    return id == "" ? "0" : id;
}

function _AddFolderToList(list, item, parentId) {
    local folder = CFolderItem();
    folder.setId(item.FolderID);
    folder.setTitle(item.Name);
    folder.setParentId(parentId);

    if ("Description" in item) {
        folder.setSummary(item.Description);
    } else {
        folder.setSummary("");
    }

    if ("Link" in item) {
        folder.setViewUrl(item.Link);
    }

    if ("Access" in item) {
        // OpenDrive: 1 - public, 2 - hidden, 3 - private.
        folder.setAccessType(item.Access.tointeger() == 3 ? 0 : 1);
    }

    list.AddFolderItem(folder);
}

function Authenticate() {
    local sessionId = _SessionId();
    if (sessionId != "") {
        return 1;
    }

    local login = ServerParams.getParam("Login");
    local password = ServerParams.getParam("Password");

    if (login == "" || password == "") {
        WriteLog("error", "[opendrive.com] Login and password should not be empty!");
        return 0;
    }

    nm.setUrl(API_BASE_URL + "/session/login.json");
    nm.addQueryParam("username", login);
    nm.addQueryParam("passwd", password);
    nm.addQueryParam("version", "1.0");
    nm.addQueryParam("partner_id", "");
    nm.doPost("");

    if (_CheckResponse("authentication") < 1) {
        return 0;
    }

    local response = _ParseResponse("authentication");
    if (response != null && "SessionID" in response && response.SessionID != "") {
        ServerParams.setParam("sessionId", response.SessionID);
        return 1;
    }

    WriteLog("error", "[opendrive.com] Authentication failed: session id is missing");
    return 0;
}

function IsAuthenticated() {
    return _SessionId() != "" ? 1 : 0;
}

function DoLogout() {
    local sessionId = _SessionId();
    if (sessionId == "") {
        return 1;
    }

    nm.setUrl(API_BASE_URL + "/session/logout.json");
    nm.addQueryParam("session_id", sessionId);
    nm.doPost("");

    _ClearAuthData();
    return nm.responseCode() == 200 ? 1 : 0;
}

function GetFolderList(list) {
    local sessionId = _SessionId();
    if (sessionId == "") {
        return -2;
    }

    local parentId = list.parentFolder().getId();

    if (parentId == "") {
        local rootFolder = CFolderItem();
        rootFolder.setId("0");
        rootFolder.setTitle("/ (root)");
        rootFolder.setSummary("");
        list.AddFolderItem(rootFolder);
        return 1;
    }

    parentId = _FolderId(parentId);

    nm.setUrl(API_BASE_URL + "/folder/list.json/" + nm.urlEncode(sessionId) + "/" + nm.urlEncode(parentId));
    nm.addQueryParam("only_subfolders", "true");
    nm.doGet("");

    local code = _CheckResponse("folder list", -2);
    if (code < 1) {
        return code;
    }

    local response = _ParseResponse("folder list");
    if (response == null) {
        return 0;
    }

    if ("Folders" in response) {
        foreach (item in response.Folders) {
            _AddFolderToList(list, item, parentId);
        }
    }

    return 1;
}

function CreateFolder(parentFolder, folder) {
    local sessionId = _SessionId();
    if (sessionId == "") {
        return 0;
    }

    local parentId = _FolderId(parentFolder.getId());
    local accessType = folder.getAccessType();
    local openDriveAccess = accessType == 1 ? 1 : 0;

    nm.setUrl(API_BASE_URL + "/folder.json");
    nm.addQueryParam("session_id", sessionId);
    nm.addQueryParam("folder_name", folder.getTitle());
    nm.addQueryParam("folder_sub_parent", parentId);
    nm.addQueryParam("folder_is_public", openDriveAccess.tostring());
    nm.addQueryParam("folder_public_upl", "0");
    nm.addQueryParam("folder_public_display", accessType == 1 ? "1" : "0");
    nm.addQueryParam("folder_public_dnl", accessType == 1 ? "1" : "0");
    nm.addQueryParam("folder_display_subfolders", "1");
    nm.addQueryParam("folder_description", folder.getSummary());
    nm.doPost("");

    if (_CheckResponse("folder creation") < 1) {
        return 0;
    }

    local response = _ParseResponse("folder creation");
    if (response != null && "FolderID" in response) {
        folder.setId(response.FolderID);
        folder.setParentId(parentId);
        if ("Link" in response) {
            folder.setViewUrl(response.Link);
        }
        return 1;
    }

    return 0;
}

function ModifyFolder(folder) {
    local sessionId = _SessionId();
    if (sessionId == "") {
        return 0;
    }

    local folderId = _FolderId(folder.getId());
    if (folderId == "0") {
        WriteLog("error", "[opendrive.com] Cannot modify root folder");
        return 0;
    }

    local accessType = folder.getAccessType();
    local openDriveAccess = accessType == 1 ? 1 : 3;

    nm.setMethod("PUT");
    nm.setUrl(API_BASE_URL + "/folder/foldersettings.json");
    nm.addQueryParam("session_id", sessionId);
    nm.addQueryParam("folder_id", folderId);
    nm.addQueryParam("folder_name", folder.getTitle());
    nm.addQueryParam("folder_description", folder.getSummary());
    nm.addQueryParam("folder_access", openDriveAccess.tostring());
    nm.addQueryParam("folder_public_upl", "0");
    nm.addQueryParam("folder_public_display", accessType == 1 ? "1" : "0");
    nm.addQueryParam("folder_public_dnl", accessType == 1 ? "1" : "0");
    nm.addQueryParam("folder_display_subfolders", "1");
    nm.doUpload("", "");

    if (_CheckResponse("folder modification") < 1) {
        return 0;
    }

    local response = _ParseResponse("folder modification");
    if (response != null) {
        if ("FolderID" in response) {
            folder.setId(response.FolderID);
        }
        if ("Link" in response) {
            folder.setViewUrl(response.Link);
        }
    }

    return 1;
}

function _CreateFile(sessionId, folderId, fileName, fileSize) {
    nm.setUrl(API_BASE_URL + "/upload/create_file.json");
    nm.addQueryParam("session_id", sessionId);
    nm.addQueryParam("folder_id", folderId);
    nm.addQueryParam("file_name", fileName);
    nm.addQueryParam("file_size", fileSize.tostring());
    nm.addQueryParam("open_if_exists", "0");
    nm.doPost("");

    if (_CheckResponse("file creation") < 1) {
        return null;
    }

    return _ParseResponse("file creation");
}

function _OpenFileUpload(sessionId, fileId, fileSize) {
    nm.setUrl(API_BASE_URL + "/upload/open_file_upload.json");
    nm.addQueryParam("session_id", sessionId);
    nm.addQueryParam("file_id", fileId);
    nm.addQueryParam("file_size", fileSize.tostring());
    nm.doPost("");

    if (_CheckResponse("opening file upload") < 1) {
        return null;
    }

    return _ParseResponse("opening file upload");
}

function _UploadChunk(fileName, displayName, sessionId, fileId, tempLocation, offset, size) {
    local url = API_BASE_URL + "/upload/upload_file_chunk2.json/" + nm.urlEncode(sessionId) + "/" + nm.urlEncode(fileId);
    url += "?temp_location=" + nm.urlEncode(tempLocation);
    url += "&chunk_offset=" + offset.tostring();
    url += "&chunk_size=" + size.tostring();

    nm.setUrl(url);
    nm.setChunkOffset(offset);
    nm.setChunkSize(size);
    nm.addQueryParamFile("file_data", fileName, displayName, GetFileMimeType(fileName));
    nm.doUploadMultipartData();

    return _CheckResponse("chunk upload");
}

function _CloseFileUpload(sessionId, fileId, fileSize, tempLocation) {
    nm.setUrl(API_BASE_URL + "/upload/close_file_upload.json");
    nm.addQueryParam("session_id", sessionId);
    nm.addQueryParam("file_id", fileId);
    nm.addQueryParam("file_size", fileSize.tostring());
    nm.addQueryParam("temp_location", tempLocation);
    nm.addQueryParam("file_time", time().tostring());
    nm.doPost("");

    if (_CheckResponse("closing file upload") < 1) {
        return null;
    }

    return _ParseResponse("closing file upload");
}

function _SetFilePublic(sessionId, fileId) {
    nm.setUrl(API_BASE_URL + "/file/access.json");
    nm.addQueryParam("session_id", sessionId);
    nm.addQueryParam("file_id", fileId);
    nm.addQueryParam("file_ispublic", "1");
    nm.doPost("");

    if (_CheckResponse("setting public file access") < 1) {
        return null;
    }

    return _ParseResponse("setting public file access");
}

function UploadFile(FileName, options) {
    local sessionId = _SessionId();
    if (sessionId == "") {
        return 0;
    }

    local task = options.getTask().getFileTask();
    local displayName = task.getDisplayName();
    local folderId = _FolderId(options.getFolderID());
    local fileSize = GetFileSize(FileName);

    if (fileSize < 0) {
        WriteLog("error", "[opendrive.com] Unable to get file size");
        return 0;
    }

    local fileInfo = _CreateFile(sessionId, folderId, displayName, fileSize);
    if (fileInfo == null || !("FileId" in fileInfo)) {
        return 0;
    }

    local fileId = fileInfo.FileId;
    local uploadInfo = _OpenFileUpload(sessionId, fileId, fileSize);
    if (uploadInfo == null || !("TempLocation" in uploadInfo)) {
        return 0;
    }

    if ("RequireCompression" in uploadInfo && uploadInfo.RequireCompression.tointeger() != 0) {
        WriteLog("error", "[opendrive.com] Server requested compressed upload, which is not supported by this script");
        return 0;
    }

    local tempLocation = uploadInfo.TempLocation;
    local offset = 0;

    while (offset < fileSize) {
        local currentChunkSize = min(CHUNK_SIZE, fileSize - offset).tointeger();
        local uploaded = 0;

        for (local attempt = 0; attempt < 2; attempt++) {
            if (_UploadChunk(FileName, displayName, sessionId, fileId, tempLocation, offset, currentChunkSize) == 1) {
                uploaded = 1;
                break;
            }
            WriteLog("warning", "[opendrive.com] Retrying chunk upload, offset=" + offset + ", size=" + currentChunkSize);
        }

        if (!uploaded) {
            return 0;
        }

        offset += currentChunkSize;
    }

    local closeInfo = _CloseFileUpload(sessionId, fileId, fileSize, tempLocation);
    if (closeInfo == null) {
        return 0;
    }

    local publicInfo = _SetFilePublic(sessionId, fileId);
    if (publicInfo != null) {
        closeInfo = publicInfo;
    }

    if ("Link" in closeInfo && closeInfo.Link != "") {
        options.setViewUrl(closeInfo.Link);
    }
    if ("DirectLinkPublick" in closeInfo && closeInfo.DirectLinkPublick != "") {
        options.setDirectUrl(closeInfo.DirectLinkPublick);
    } else if ("DownloadLink" in closeInfo && closeInfo.DownloadLink != "") {
        options.setDirectUrl(closeInfo.DownloadLink);
    }
    if ("ThumbLink" in closeInfo && closeInfo.ThumbLink != "") {
        options.setThumbUrl(closeInfo.ThumbLink);
    }

    if (("Link" in closeInfo && closeInfo.Link != "") || ("DownloadLink" in closeInfo && closeInfo.DownloadLink != "")) {
        return 1;
    }

    WriteLog("error", "[opendrive.com] Upload succeeded but public link is missing");
    return 0;
}

function GetFolderAccessTypeList() {
    return ["Private", "Public"];
}
