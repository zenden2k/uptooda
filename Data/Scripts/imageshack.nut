token <- "";
username <- "";
api_key <- "BXT1Z35V8f6ee0522939d8d7852dbe67b1eb9595";

function DoLogin()
{
    if (token == "") {
        local login = ServerParams.getParam("Login");
        local pass =  ServerParams.getParam("Password");
        if (login == "" || pass == "") {
            return 0;
        }
        nm.enableResponseCodeChecking(false);
        nm.setUrl("https://api.imageshack.com/v2/user/login");
        nm.addPostField("user", login);
        nm.addPostField("password", pass);
        nm.addPostField("api_key", api_key);
        nm.doPost("");
        local res = ParseJSON(nm.responseBody());
        if (res != null) {
            if ("result" in res && "auth_token" in res.result) {
                token = res.result.auth_token;
                username = res.result.username;
                return 1;
            } else if ("error" in res) {
                WriteLog("error", "imagehack.com error: " + res.error.error_message);
                return 0;
            }
        }
        return 0;
    }
    return 1;
}

function GetFolderList(list)
{
    if(!DoLogin()) {
        return 0;
    }

    if (username == "") {
        return 0;
    }

    nm.doGet("https://api.imageshack.com/v2/user/" + username +"/albums?auth_token="+nm.urlEncode(token)+"&show_private=1&limit=1000");
    local obj = ParseJSON(nm.responseBody());
    if (obj != null) {
        if ("success" in obj && obj.success) {
            local rootFolder = CFolderItem();
            rootFolder.setId("/");
            rootFolder.setTitle("My Images");
            list.AddFolderItem(rootFolder);

            local albums = obj.result.albums;
            local count = albums.len();

            for ( local i = 0; i < count; i++ ) {
                local item = albums[i];

                local folder = CFolderItem();
                folder.setId(item.id);
                folder.setTitle(item.title);
                folder.setParentId("/");
                folder.setAccessType(item.public ? 1:0);
                folder.setViewUrl("https://imageshack.com/a/" + item.id +"/1");
                list.AddFolderItem(folder);
            }
            return 1;
        } else if ("error" in obj){
            WriteLog("error", "imageshack.com error: " + obj.error.error_message);
        }
    } else if (nm.responseCode() != 200){
        WriteLog("error", "imageshack.com response code: " + m.responseCode());
    }

    return 0;
}


function CreateFolder(parentAlbum,album)
{
    if ( !DoLogin() ) {
        return 0;
    }

    local title = album.getTitle();
    local summary = album.getSummary();
    local accessType = album.getAccessType();

    nm.enableResponseCodeChecking(false);
    nm.addPostField("title", title);
    nm.addPostField("description", summary);
    nm.addPostField("public", accessType==0?"FALSE":"TRUE");

    nm.addPostField("auth_token", token);
    nm.addPostField("api_key", api_key);
    nm.setUrl("https://api.imageshack.com/v2/albums");

    nm.doPost("");

    local obj = ParseJSON(nm.responseBody());
    if (obj != null) {
        if ("success" in obj && obj.success) {
            local remoteAlbum = obj.result;
            album.setId(remoteAlbum.id);
            album.setTitle(remoteAlbum.title);
            album.setSummary(remoteAlbum.description);
            album.setAccessType(remoteAlbum.public ? 1:0);
            album.setViewUrl("https://imageshack.com/a/" + remoteAlbum.id +"/1");
            return 1;
        } else if ("error" in obj){
            WriteLog("error", "imagehack.com error: " + obj.error.error_message);
        }
    } else if (nm.responseCode() != 200){
        WriteLog("error", "imagehack.com response code: " + m.responseCode());
    }

    return 1;
}

function ModifyFolder(album)
{
    if(!DoLogin())
        return 0;

    local id = album.getId();
    if (id == "" || id == "/") {
        return 0;
    }
    local title =album.getTitle();
    local summary = album.getSummary();
    local accessType = album.getAccessType();

    nm.enableResponseCodeChecking(false);
    nm.addPostField("title", title);
    nm.addPostField("description", summary);
    nm.addPostField("public", accessType==0?"FALSE":"TRUE");

    nm.addPostField("auth_token", token);
    nm.addPostField("api_key", api_key);
    nm.setUrl("https://api.imageshack.com/v2/albums/"+id);
    nm.setMethod("PATCH");
    nm.doPost("");

    local obj = ParseJSON(nm.responseBody());
    if (obj != null) {
        if ("success" in obj && obj.success) {
            return 1;
        } else if ("error" in obj){
            WriteLog("error", "imageshack.com error: " + obj.error.error_message);
        }
    } else if (nm.responseCode() != 200){
        WriteLog("error", "imageshack.com response code: " + m.responseCode());
    }

    return 0; // failure
}
function  UploadFile(FileName, options)
{
    if(!DoLogin()) {
        return 0;
    }

    local albumId = options.getFolderID();

    nm.enableResponseCodeChecking(false);
    nm.setUrl("https://api.imageshack.com/v2/images");
    nm.addPostField("auth_token", token);
    nm.addPostField("api_key", api_key);
    if (albumId!="" && albumId!="/") {
        nm.addPostField("album", albumId);
    }
    nm.addPostFieldFile("file", FileName, ExtractFileName(FileName), GetFileMimeType(FileName));
    local thumbwidth = options.getParam("THUMBWIDTH");

    nm.doUploadMultipartData();
    local obj = ParseJSON(nm.responseBody());
    if (obj != null) {
        if ("success" in obj && obj.success) {
            local img = obj.result.images[0];
            local directUrl  = img.direct_link;
            if ( directUrl != "" ) {
                directUrl = "https://" + StrReplace(directUrl, "\\", "");
                options.setDirectUrl(directUrl);
                return 1;
            }
        } else if ("error" in obj){
            WriteLog("error", "imageshack.com error: " + obj.error.error_message);
        }
    } else if (nm.responseCode() != 200){
        WriteLog("error", "imageshack.com response code: " + m.responseCode());
    }
    return 0;
}

function GetFolderAccessTypeList()
{
    return ["Private", "Public"];
}