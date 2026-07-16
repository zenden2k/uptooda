const BASE_URL = "https://www.linkbox.to/api";
const EXPIRATION_PERMANENT = 4;

function _PrintError(t, txt) {
    local errorMessage = "[TeleBox] " + txt + " Response code: " + nm.responseCode();
    if (t != null && "msg" in t) {
        errorMessage += "\n" + t.msg;
    }
    WriteLog("error", errorMessage);
}

function UploadFile(fileName, options) {
    local task = options.getTask().getFileTask();
    local apiToken = ServerParams.getParam("Password");
    local expiration = EXPIRATION_PERMANENT;

    try {
        expiration = ServerParams.getParam("expiration").tointeger();
    } catch(e) {
    }

    if (expiration < 1 || expiration > EXPIRATION_PERMANENT) {
        expiration = EXPIRATION_PERMANENT; 
    }

    local destFolderId = options.getFolderID();

    if (destFolderId == "") {
        destFolderId = "0";
    }

    if (apiToken == "") {
        WriteLog("error", "[TeleBox] You must provide an API token for uploading to this hosting.");
        return ResultCode.Failure;
    }
    local fileMd5ofPre10m = Md5(GetFileContentsEx(fileName, 0, 10485760, true));
    nm.doGet(BASE_URL + "/open/get_upload_url?fileMd5ofPre10m=" + fileMd5ofPre10m + "&fileSize=" + GetFileSize(fileName)
            + "&token=" + apiToken
    );

    local t = ParseJSON(nm.responseBody());

    if (nm.responseCode() != 200 || t == null || t.status != 1){
        _PrintError(t, "Failed to obtain upload URL.");
        return ResultCode.Failure;
    }

    local uploadUrl = StrReplace(t.data.signUrl, "http://", "https://");

    nm.setUrl(uploadUrl);
    nm.setMethod("PUT");
    nm.doUpload(fileName, "");

    if (nm.responseCode() != 200) {
        _PrintError(null, "Failed to upload file.");
        return ResultCode.Failure;
    }

    nm.doGet(BASE_URL + "/open/folder_upload_file?fileMd5ofPre10m=" + fileMd5ofPre10m 
        + "&fileSize=" + GetFileSize(fileName)
        + "&pid=" + destFolderId    
        + "&diyName=" + nm.urlEncode(task.getDisplayName())
        + "&token=" + apiToken
    );

    t = ParseJSON(nm.responseBody());

    if (nm.responseCode() != 200 || t == null || t.status != 1 || t.data.itemId == "") {
        _PrintError(t, "Failed to create file item.");
        return ResultCode.Failure;
    }

    nm.doGet(BASE_URL + "/open/file_share?itemIds=" + t.data.itemId 
                + "&expire_enum=" + expiration 
                + "&token=" + apiToken
    );

    t = ParseJSON(nm.responseBody());

    if (nm.responseCode() != 200 || t == null || t.status != 1 || t.data.shareToken == "") {
        _PrintError(t, "Failed to obtain share token.");
        return ResultCode.Failure;
    }

    options.setViewUrl("https://lbx.cx/f/" + t.data.shareToken);
    return ResultCode.Success;
}

function GetFolderList(list) {
    local apiToken = ServerParams.getParam("Password");

    if (apiToken == "") {
        WriteLog("error", "[TeleBox] You must provide an API token.");
        return ResultCode.Failure;
    }

    local parentFolderId = list.parentFolder().getId();
    if (parentFolderId == "") {
        local album = CFolderItem();
        album.setId("0");
        album.setTitle("/ (root)");
        list.AddFolderItem(album);
        return ResultCode.Success;
    }

    nm.doGet(BASE_URL + "/open/folder_details?dirId=" + parentFolderId
        + "&token=" + apiToken
    );

    local t = ParseJSON(nm.responseBody());

    if (nm.responseCode() != 200 || t == null || t.status != 1) {
        _PrintError(t, "Failed to load folder list.");
        return ResultCode.Failure;
    }

    return 1; //success
}

function CreateFolder(parentFolder, folder) {
    local apiToken = ServerParams.getParam("Password");

    if (apiToken == "") {
        WriteLog("error", "[TeleBox] You must provide an API token.");
        return ResultCode.Failure;
    }

    local parentFolderId = parentFolder.getId();

    if (parentFolderId == "") {
        parentFolderId = "0";
    }
    nm.doGet(BASE_URL + "/open/folder_create?name=" + nm.urlEncode(folder.getTitle())
        + "&pid=" + parentFolderId
        + "&token=" + apiToken
        + "&isShare=0"
        + "&canInvite=1"
        + "&canShare=1"
        + "&withBodyImg=0"
        + "&desc=" + folder.getSummary()
    );
    
    local t = ParseJSON(nm.responseBody());

    if (nm.responseCode() != 200 || t == null || t.status != 1 || t.data.dirId == "") {
        _PrintError(t, "Failed to create folder.");
        return ResultCode.Failure;
    }
        
    folder.setId(t.data.dirId);
    return ResultCode.Success;
}

function GetServerParamList() {
    return {
        //apiToken = "API token",
        expiration = {
            title = tr("chevereto.expiration"),
            type = "choice",
            items = [
                {
                    id = "4",
                    label = tr("chevereto.never")
                },
                {
                    id = "1",
                    label = "24 hours"
                },
                {
                    id = "2",
                    label = "7 days"
                },
                {
                    id = "3",
                    label = "30 days"
                },
            ]
        }
    };
}