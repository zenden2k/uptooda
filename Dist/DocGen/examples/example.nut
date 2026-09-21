/**
 * Global network client
 * @var nm
 * @type NetworkClient
 */

/**
 * @var ServerParams
 * @type ServerSettingsStruct
 */

/**
 * @var Sync
 * @type ServerSync
 */

const BASE_URL = "http://example.com";

function _PrintError(t, txt) {
    local errorMessage = "[example.com]  " + txt + " Response code: " + nm.responseCode();
    if (t != null && "error" in t) {
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
    local apiToken = ServerParams.getParam("apiToken");
    local folderId = options.getFolderID();

    if (apiToken == "") {
        WriteLog("error", "[example.com] API token not set!");
        return ResultCode.Failure;
    }

    nm.doGet(BASE_URL + "/");

    if (nm.responseCode() != 200) {
        return ResultCode.Failure;
    }

    // Parsing HTML with Gumbo-query
    local doc = Document(nm.responseBody());
    local csrf = doc.find("input[name=csrf]").at(1).attr("value");
    if (csrf == "") {
        csrf = doc.find("meta[name=csrf]").attr("value");
    }

    if (csrf == "") {
        WriteLog("error", "[example.com] Cannot obtain CSRF token.");
        return ResultCode.Failure;
    }

    nm.setUrl(BASE_URL + "/upload.php?fname=" + nm.urlEncode(task.getDisplayName()));
    nm.addQueryHeader("X-CSRF-token", csrf);
    nm.addPostField("token", apiToken); // Add a POST parameter to the request
    nm.addPostFieldFile("file", pathToFile, task.getDisplayName(), GetFileMimeType(pathToFile));
    nm.addPostField("submit", "Upload file!");
    nm.doUploadMultipartData(); // Make multipart/form-data request

    if (nm.responseCode() != 200) {
        WriteLog("error", "[example.com] Upload failed. Response code:" + nm.responseCode());
        return ResultCode.Failure;
    }

    // Extracting links from the server response
    // Method 1 (with regular expression)
    local reg = CRegExp("\\[IMG\\](.+)\\[/IMG\\]", "mi");
    local directUrl = "";
    local viewUrl = "";
    local thumbnailUrl = "";

    if (reg.match(nm.responseBody())) {
        directUrl = reg.getMatch(1);
    }

    // Method 2 (with parsing JSON)
    local t = ParseJSON(nm.responseBody());
    if (nm.responseCode() != 200) {
        _PrintError(t, "Failed to upload");
        return ResultCode.Failure;
    }

    if (t != null && "url" in t) {
        viewUrl = t.view_url;
        thumbnailUrl = t.thumb_url;
    } else {
        _PrintError(t, "Failed to upload");
        return ResultCode.Failure;
    }

    if (directUrl == "") {
        WriteLog("error", "[example.com] Upload failed. Cannot obtain the direct URL!");
        return ResultCode.Failure;
    }

    options.setDirectUrl(directUrl);
    options.setViewUrl(viewUrl);
    options.setThumbUrl(thumbnailUrl);

    return ResultCode.Success;
}

/** 
* Authenticating on remote server (optional function)
* @var CFolderList list
* @return int - success(1), failure(0)
*/
function Authenticate() {
    local userName = ServerParams.getParam("Login");
    if (ServerParams.getParam("token") != "") {
        return ResultCode.Success;
    }

    local username = ServerParams.getParam("Login"); // Login is a predefined parameter, no need to define it in GetServerParamList function
    local password = ServerParams.getParam("Password"); // Password is a predefined parameter, no need to define it in GetServerParamList function

    // Performing request
    // ...
    local t = ParseJSON(nm.responseBody());
    if ("access_token" in t) {
        ServerParams.setParam("token", t.access_token);
        return ResultCode.Success;
    }
    return ResultCode.Failure;
}

/** 
* Optional function:
* Retrieving folder (album) list from server
* @var CFolderList list
* @return int - success(1), failure(0)
*/
function GetFolderList(list) {
    local parentId = list.parentFolder().getId();
    nm.doGet(BASE_URL + "/albums?parentId=" + nm.urlEncode(parentId));
    local obj = ParseJSON(nm.responseBody());
    if (obj != null) {
        if ("success" in obj && obj.success) {
            local rootFolder = CFolderItem();
            rootFolder.setId("/");
            rootFolder.setTitle("My Images");
            list.AddFolderItem(rootFolder);

            local albums = obj.result.albums;
            local count = albums.len();

            for (local i = 0; i < count; i++) {
                local item = albums[i];

                local folder = CFolderItem();
                folder.setId(item.id);
                folder.setTitle(item.title);
                folder.setParentId("/");
                folder.setAccessType(item.public ? 1 : 0);
                folder.setViewUrl("https://example.com/a/" + item.id);
                list.AddFolderItem(folder);
            }
            return ResultCode.Success;
        } else if ("error" in obj){
            WriteLog("error", "[example.com] error: " + obj.error.error_message);
        }
    } else if (nm.responseCode() != 200){
        WriteLog("error", "[example.com] response code: " + m.responseCode());
    }

    return ResultCode.Failure;
}

/** 
* Create an folder or an album (optional function)
* @var CFolderItem parentAlbum
* @var CFolderItem album
* @return int - success(1), failure(0)
*/
function CreateFolder(parentAlbum, album) {
    local req = {
        title = album.getTitle(),
        description = album.getSummary(),
        public = album.getAccessType() == 0 ? "FALSE" : "TRUE"
    };

    nm.setUrl(BASE_URL + "/v2/albums");
    nm.addQueryHeader("Content-Type", "application/json");
    nm.doPost(ToJSON(req));

    local obj = ParseJSON(nm.responseBody());
    if (obj != null) {
        if ("success" in obj && obj.success) {
            local remoteAlbum = obj.result;
            album.setId(remoteAlbum.id);
            album.setTitle(remoteAlbum.title);
            album.setSummary(remoteAlbum.description);
            album.setAccessType(remoteAlbum.public ? 1:0);
            album.setViewUrl("https://example.com/a/" + remoteAlbum.id);
            return ResultCode.Success;
        } else if ("error" in obj){
            WriteLog("error", "[example.com] error: " + obj.error.error_message);
        }
    } else if (nm.responseCode() != 200){
        WriteLog("error", "[example.com] response code: " + m.responseCode());
    }

    return ResultCode.Failure;
}

/** 
* Modify a folder or an album (update name, description) (optional function)
* @var CFolderItem album
* @return int - success(1), failure(0)
*/
function ModifyFolder(album) {
    // TODO: Your code
    return ResultCode.Success;
}

/** 
* A function that returns a list of types of access restrictions to an album: (optional function)
* private, public, only for friends, etc.
* @return array
*/
function GetFolderAccessTypeList() {
    return ["Private", "Public"];
}

/** 
* optional function
*/
function GetServerParamList() {
    return {
        apiToken = "API Token",
        albumid = "Album ID",
        age = "Age",
        striptags = "Strip Tags",
        notify = {
            title = "Email notification on download",
            type = "boolean" // checkbox
        },
        expiration = {
            title = "Expiration",
            type = "choice",
            items = [
                {
                    id = "",
                    label = Never"
                },
                {
                    id = "PT5M",
                    label = "15 minutes"
                },
                {
                    id = "PT30M",
                    label = "30 minutes"
                },
                {
                    id = "PT1H",
                    label = "1 hour"
                }
            ]
        },
        privateKeyPath = {
            title = "Private key path",
            type = "filename"
        } 
    }
}