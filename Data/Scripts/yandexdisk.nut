const CURLOPT_FOLLOWLOCATION = 52;

if (ServerParams.getParam("enableOAuth") == "") {
    ServerParams.setParam("enableOAuth", "true");
}

if (ServerParams.getParam("useWebdav") == "") {
    ServerParams.setParam("useWebdav", "false");
    ServerParams.setParam("token", "");
    ServerParams.setParam("tokenType", "");
}

login <- "";
enableOAuth <- true;
baseUrl <-"https://cloud-api.yandex.net/v1/disk/resources/";
clientId <-  GetEnvDecode("IU_YANDEX_DISK_CLIENT_ID");
clientSecret <- GetEnvDecode("IU_YANDEX_DISK_SECRET");

function _RegexSimple(data, regStr, start) {
    local ex = regexp(regStr);
    local res = ex.capture(data, start);
    local resultStr = "";
    if (res != null){
        resultStr = data.slice(res[1].begin, res[1].end);
    }
        return resultStr;
}

function _IdFromPath(path) {
    return StrReplace(path, "disk:", "") + "/";
}
/*function _UrlEncodePath(str) {
    local res = "";
    local start = 0;
    local pos;
    while ((pos = str.find("/", start)) != null ) {
        res += nm.urlEncode(str.slice(start, pos)) + "/";
        start = pos + 1;
    }
    res += nm.urlEncode(str.slice(start));

    return res;
}*/

function _CheckResponse() {
    if ( nm.responseCode() == 0 || (nm.responseCode() >= 400 && nm.responseCode() <= 499)) {
        WriteLog("error", "Response code " + nm.responseCode() + "\r\n" + nm.errorString() );
        return 0;
    }
    return 1;
}

function _IsSuccessCode(code) {
    return ( code >= 200 && code < 400);
}

function _GetAuthorizationString() {
    return "OAuth " + ServerParams.getParam("token");
}

function _ParseAlbumList(data,list,parentid)
{
    local start = 0;

    while(1) {
        local title="",id="",summary="";
        local response = "";

        start += response.len();
        local ex = regexp("<d:response>");
        local res = ex.search(data, start);
        local link = "";
        local album = CFolderItem();
        if(res != null){
            start = res.end;
            ex = regexp("</d:response>");
            res = ex.search(data, start);
            if (res != null){
                response = data.slice(start, res.end);
                local href = _RegexSimple(response,"<d:href>(.+)</d:href>",0);

                local resourceType =  _RegexSimple(response,"<d:resourcetype>(.+)</d:resourcetype>",0);
                if (_RegexSimple(response, "(d:collection)", 0) != "") {
                    local displayName = _RegexSimple(response,"<d:displayname>(.+)</d:displayname>",0);
                    album.setId(href);
                    if (href == "/" ) {
                        displayName += " (/)";
                    }
                    album.setTitle(displayName);
                    album.setSummary("");
                    list.AddFolderItem(album);
                }
            } 	else {
                break;
            }
        }
        else break;
    }
    return 1;
}

function GetFolderList(list) {
    if (_UseRestApi()) {
        nm.addQueryHeader("Authorization", _GetAuthorizationString());
        nm.addQueryHeader("Accept", "application/json");
        nm.enableResponseCodeChecking(false);
        nm.doGet("https://cloud-api.yandex.net:443/v1/disk");// test request

        local code = nm.responseCode();
        if (code == 401){
            ServerParams.setParam("token","");
            return -2;
        }
        local parentId = list.parentFolder().getId();
        local folder = CFolderItem();
        if (parentId == "") {
            parentId = "/";
            folder.setId("/");
            folder.setTitle("/ (root)");
            folder.setSummary("");
            list.AddFolderItem(folder);
    
            return 1;
        }

        local url = "https://cloud-api.yandex.net:443/v1/disk/resources?path=" + nm.urlEncode(parentId) + "&limit=100";
        nm.addQueryHeader("Authorization", _GetAuthorizationString());
        nm.addQueryHeader("Accept", "application/json");
        nm.doGet(url);
        code = nm.responseCode();

        if (code == 200) {
            
            local res = ParseJSON(nm.responseBody());
            local itemsCount = res._embedded.items.len();
            for ( local i = 0; i< itemsCount; i++ ) {
                local item = res._embedded.items[i];
                if ( item.type != "dir" ) {
                    continue;
                }
                local folder = CFolderItem();
                local path = item.path;
                path = StrReplace(path, "disk:", "") + "/";
                folder.setId(path);
                folder.setTitle(item.name);
                folder.setSummary("");
                folder.setParentId(parentId);
                list.AddFolderItem(folder);
            }
            return 1;
        }
        return 0;
    }

    local url = "https://webdav.yandex.ru/";

    nm.setUrl(url);
    nm.addQueryHeader("Authorization",_GetAuthorizationString());
    nm.addQueryHeader("Connection", "close");
    nm.addQueryHeader("Depth", "1");
    nm.setMethod("PROPFIND");
    nm.doGet(url);

    if ( _IsSuccessCode(nm.responseCode()) ) {
        return _ParseAlbumList(nm.responseBody(), list, "");
    } else if (nm.responseCode() == 401) {
        WriteLog("error", "Authentication failed for username '" + ServerParams.getParam("Login") + "'.\r\nResponse code: "+ nm.responseCode());
        return 0;
    }

    return 0;
}

function CreateFolder(parentAlbum, album) {
    local parentId = parentAlbum.getId();
    if (parentId == "") {
        parentId = "/";
    }
    if (_UseRestApi()) {
        nm.addQueryHeader("Authorization", _GetAuthorizationString());
        nm.addQueryHeader("Accept", "application/json");
        //nm.addQueryHeader("Content-Type", "application/json; charset=utf-8");
        nm.enableResponseCodeChecking(false);
        nm.setUrl("https://cloud-api.yandex.net/v1/disk/resources?path=" + nm.urlEncode(parentId + album.getTitle()));
        nm.setMethod("PUT");
        nm.doUpload("", "");
        if (nm.responseCode() == 201) {
            local t = ParseJSON(nm.responseBody());
            if (t != null && "href" in t) {
                nm.addQueryHeader("Authorization", _GetAuthorizationString());
                nm.addQueryHeader("Accept", "application/json");
                nm.doGet(t.href);
                if (nm.responseCode() == 200) {
                    t = ParseJSON(nm.responseBody());
                    if (t != null && "path" in t) {
                        album.setId(_IdFromPath(t.path));
                        album.setParentId(parentId);
                    }
                }
            }
            return 1;
        } else {
            WriteLog("error", "[yandex.disk] Failed to create a folder, response code: " + nm.responseCode());
        }
        return 0;
    }

    local folderName = album.getTitle();
    if (folderName == "") {
        return 0;
    }
    local url = "https://webdav.yandex.ru/" + nm.urlEncode(folderName);

    nm.setUrl(url);
    nm.addQueryHeader("Authorization",_GetAuthorizationString());
    nm.addQueryHeader("Connection", "close");
    nm.addQueryHeader("Depth", "1");
    nm.setMethod("MKCOL");
    nm.doGet(url);

    if (nm.responseCode() != 201) { // HTTP Status code 201 = Created
        return 0;
    }

    return 1;
}

function _WaitForOperation(operationUrl, successCallback) {
    while (true) {
        nm.addQueryHeader("Authorization", _GetAuthorizationString());
        nm.addQueryHeader("Accept", "application/json");
        nm.doGet(operationUrl);
        if (nm.responseCode() == 200) {
            local t2 = ParseJSON(nm.responseBody());
            if (t2 != null && "status" in t2) {
                if (t2.status == "in-progress") {
                    Sleep(1000);
                    continue;
                } else if (t2.status == "success") {
                   return successCallback();
                } else if (t2.status == "failed") {
                    return 0;
                }
            } else {
                break;
            }
        }
    }
    return 0;
}

// Rename folder
function ModifyFolder(folder) {
    local title = folder.getTitle();
    local id = folder.getId();

    if (id == "" || id == "/") {
        WriteLog("error", "[yandex.disk] Cannot rename root folder");
        return 0;
    }

    nm.setMethod("POST");
    nm.addQueryHeader("Authorization", _GetAuthorizationString());
    nm.addQueryHeader("Accept", "application/json");
    nm.enableResponseCodeChecking(false);
    nm.setUrl("https://cloud-api.yandex.net/v1/disk/resources/move?from=" + nm.urlEncode(id) + "&path=" +  nm.urlEncode(folder.getParentId() + title));

    nm.doPost("");
    local code = _CheckResponse();
    if (code < 1) {
        WriteLog("error", "[yandex.disk] Unable to rename folder, response code: " + nm.responseCode());
        return code;
    } else {
        local t = ParseJSON(nm.responseBody());
        if (t != null && "href" in t) {
            if (t.href.find("operations", 0) != null) {
                return _WaitForOperation(t.href, function() {
                    // Operation has finished successfully
                    nm.addQueryHeader("Authorization", _GetAuthorizationString());
                    nm.addQueryHeader("Accept", "application/json");
                    nm.doGet("https://cloud-api.yandex.net/v1/disk/resources?path=" + nm.urlEncode(folder.getParentId() + title));
                    if (_IsSuccessCode(nm.responseCode())) {
                        local t3 = ParseJSON(nm.responseBody());
                        folder.setId(_IdFromPath(t3.path));
                        return 1;
                    } else {
                        WriteLog("error", "[yandex.disk] Renaming operation has finished successfully, but resource not found, response code: " + nm.responseCode());
                    }
                    return 0;
                });
            } else {
                nm.addQueryHeader("Authorization", _GetAuthorizationString());
                nm.addQueryHeader("Accept", "application/json");
                nm.doGet(t.href);
                if (_IsSuccessCode(nm.responseCode())) {
                    t = ParseJSON(nm.responseBody());
                    if (t != null && "path" in t) {
                        folder.setId(_IdFromPath(t.path));
                    }
                } else {
                    WriteLog("error", "[yandex.disk] Unable to rename folder, response code: " + nm.responseCode());
                }
                return 1;  
            }
        }
    }

    return 0;
}

function _GetAuthentificationErrorString(code) {
    return "Authentication failed for username '" + ServerParams.getParam("Login") + "'.\r\nResponse code: " + code;
}

function _CheckIfExists(filename){
    local url = "https://webdav.yandex.ru" + filename;

    nm.enableResponseCodeChecking(false);

    nm.setUrl(url);
    nm.addQueryHeader("Authorization",_GetAuthorizationString());
    nm.addQueryHeader("Connection", "close");
    nm.addQueryHeader("Depth", "1");
    nm.addQueryHeader("Content-Type", "application/xml");
    nm.setMethod("PROPFIND");
    nm.addQueryHeader("Transfer-Encoding", "");
    nm.doUpload("", "<?xml version=\"1.0\" encoding=\"utf-8\"?>" +
                    "<propfind xmlns=\"DAV:\">"+
                    "<prop>"+
                    "<myprop xmlns=\"mynamespace\"/>"+
                    "</prop>"+
                    "</propfind>"
                    );

    nm.enableResponseCodeChecking(true);

    if (nm.responseCode() == 401) {
        throw _GetAuthentificationErrorString(nm.responseCode());
    }
    return (nm.responseCode() >= 200 && nm.responseCode() < 300);
}

function _CheckResponseCode() {
    if ( !_IsSuccessCode(nm.responseCode()) ) {
        local errorMessage = "";
        if ( nm.responseCode() == 401 ) {
            errorMessage += "Authentication failed for username '" + ServerParams.getParam("Login") + "'.";
        } else {
            errorMessage += "\r\nResponse: " + nm.responseBody();
        }

        errorMessage  += "\r\nResponse code: "+ nm.responseCode();
        throw errorMessage;
    }
}

function _UseRestApi() {
    local l = ServerParams.getParam("useWebdav");
    return (l == "") || ( l != "true" && l != "yes" && l != "1");
}

function Authenticate() {
    if ( enableOAuth ) {
        local token = ServerParams.getParam("token");
        local tokenType = ServerParams.getParam("tokenType");
        if ( token != "" && ServerParams.getParam("PrevLogin") == ServerParams.getParam("Login") ) {
            if ( tokenType == "oauth" ) {
                local OAuthLogin = ServerParams.getParam("OAuthLogin");
                if ( OAuthLogin != "") {
                    login = OAuthLogin;
                }
            }
            return 1;
        }

        ShellOpenUrl("https://oauth.yandex.ru/authorize?response_type=code&client_id=a49c34035aa8418d9a77ff24e0660719");

        local confirmCode = InputDialog(tr("yandexdisk.confirmation.text", "You need to need to sign in to your Yandex.Disk account\r\nin web browser which just have opened and then copy\r\nconfirmation code into the text field below.\r\n\r\nPlease enter confirmation code:"), "");
        if ( confirmCode != "" ) {
            nm.setUrl("https://oauth.yandex.ru/token");
            nm.addQueryParam("grant_type", "authorization_code");
            nm.addQueryParam("code", confirmCode);
            //nm.addQueryParam("client_id", "28d8d9c854554812ad8b60c150375462");
            //nm.addQueryParam("client_secret", "7d6fee42d583498ea7740bcf8b753197");
            nm.addQueryParam("client_id", clientId);
            nm.addQueryParam("client_secret", clientSecret);
            local deviceId = GetDeviceId();
            if (deviceId != "") {
                nm.addQueryParam("device_id", deviceId);
            }
            local deviceName = GetDeviceName();
            if (deviceName != "") {
                nm.addQueryParam("device_name", deviceName);
            }
            nm.doPost("");

            if ( !_CheckResponse() ) {
                return 0;
            }
            local t = ParseJSON(nm.responseBody());

            local accessToken = "";
            if ("access_token" in t) {
                accessToken = t.access_token;
            }

            if (accessToken != "") {
                token = 	accessToken;
                tokenType = "oauth";
                ServerParams.setParam("token", token);
                ServerParams.setParam("tokenType", tokenType);
                ServerParams.setParam("PrevLogin", ServerParams.getParam("Login"));

                return 1;
            } else {
                WriteLog("error", "Unable to get OAuth token!");
                return 0;
            }
        }
    }

    login <- ServerParams.getParam("Login");

    return 0;
}

function IsAuthenticated() {
    if (ServerParams.getParam("token") != "") {
        return 1;
    }
    return 0;
}

function DoLogout() {
    local token = ServerParams.getParam("token");
    if (token == "") {
        return 0;
    }
    local url = "https://oauth.yandex.ru/revoke_token";
    nm.setUrl(url);
    nm.addQueryParam("access_token", token);
    nm.addQueryParam("client_id", clientId);
    nm.addQueryParam("client_secret", clientSecret);

    nm.doPost("");

    if (nm.responseCode() == 200) {
        ServerParams.setParam("token", "");
        return 1;
    } else {
        local t = ParseJSON(nm.responseBody());
        if ("error" in t && t.error == "unsupported_token_type") {
            // Token cannot be revoked, because token was issued without device_id
            // Just remove it from local storage
            ServerParams.setParam("token", "");
            return 1;
        }
    }
    return 0;
}

function UploadFile(FileName, options) {
    local ansiFileName = ExtractFileName(FileName);
    local fileSize = GetFileSize(FileName);
    local mimeType = GetFileMimeType(FileName);

    try {
        local task = options.getTask().getFileTask();
        ansiFileName = task.getDisplayName(); // ensure that this field is filled up correctly
    } catch ( ex ) {
    }

    local folder = options.getFolderID();
    if ( folder == "" ) {
        folder = "/";
    }

    local initialRemotePath = folder + (ansiFileName);
    local remotePath = folder + ansiFileName;
    local remotePathEncoded = folder + nm.urlEncode(ansiFileName);
    local i = 2;

    if (_UseRestApi()) {
        local url = baseUrl + "upload/?path=" + nm.urlEncode(remotePath);

        nm.addQueryHeader("Accept", "application/json");
        nm.addQueryHeader("Authorization", _GetAuthorizationString());

        try {
            nm.enableResponseCodeChecking(false);
        } catch ( ex ) {}
        nm.doGet(url);

        if (nm.responseCode() == 401) { // not authorized
            ServerParams.setParam("token", "");
            return -2;
        }

        while ( nm.responseCode() == 409 ) {
            local ext = GetFileExtension(ansiFileName);
            local suffix = i.tostring();
            if ( i > 5 ) {
                suffix = md5( random().tostring()).slice(0,4);
            }
            local filename = ansiFileName.slice(0,ansiFileName.len()-ext.len()-1) + "_" + suffix;

            if ( ext.len() ) {
                filename += "." + ext;
            }
            remotePath = folder + filename;
            nm.setUrl(baseUrl + "upload/?path=" + nm.urlEncode(remotePath));
            nm.addQueryHeader("Accept", "application/json");
            nm.addQueryHeader("Authorization", _GetAuthorizationString());

            try {
                nm.enableResponseCodeChecking(false);
            } catch ( ex ) {}
            nm.doGet("");
            i++;
            if ( i > 10 ) {
                return 0;
            }
        }

        try {
            nm.enableResponseCodeChecking(true);
        } catch ( ex ) {}

        if ( nm.responseCode() == 200 ) {
            local data = nm.responseBody();
            local href = null;
            local method = "PUT";

            local t = ParseJSON(data);
            href = t.href;
            method = t.method;

            if (href == null || href == "") {
                return 0;
            }
            nm.setUrl(href);
            nm.setMethod(method);
            //nm.addQueryHeader("Transfer-Encoding", "");
            nm.addQueryHeader("Authorization", _GetAuthorizationString());
            nm.doUpload(FileName, "");
            if (nm.responseCode() != 201) {
                WriteLog("error", "Failed to upload file " + ExtractFileName(FileName)+".");
                return 0;
            }
            nm.setUrl(baseUrl + "publish?path=" + nm.urlEncode(remotePath));
            nm.addQueryHeader("Accept", "application/json");
            //nm.addQueryHeader("Content-Length", "0");
            nm.addQueryHeader("Content-Type", mimeType);
            nm.addQueryHeader("Transfer-Encoding", "");
            nm.addQueryHeader("Authorization",_GetAuthorizationString());
            nm.setMethod("PUT");
            nm.doUpload("", "");

            if ( nm.responseCode() == 200 ) {
                local viewUrl = "";

                try {
                    local t = ParseJSON(nm.responseBody());
                    href = t.href;
                    method = t.method;
                } catch ( ex ) {
                    href = _RegexSimple(data, "href\":\"(.+)\"", 0);
                    method = "GET";
                }
                nm.setMethod(method);
                nm.addQueryHeader("Authorization", _GetAuthorizationString());
                nm.doGet("https://cloud-api.yandex.net:443/v1/disk/resources?path=" + nm.urlEncode(remotePath));

                if (nm.responseCode() == 200) {
                    try {
                        local t = ParseJSON(nm.responseBody());
                        viewUrl = t.public_url;
                    } catch ( ex ) {
                        viewUrl = _RegexSimple(data, "public_url\":\"(.+)\"", 0);
                    }
                    options.setViewUrl(viewUrl);
                    return 1;
                }
            }
        }
        return 0;
    }

    try {
        while (_CheckIfExists(remotePathEncoded)) {
            local ext = GetFileExtension(ansiFileName);
            local suffix = i.tostring();
            if (i > 2) {
                suffix = md5(random().tostring()).slice(0, 4);
            }
            local filename = ansiFileName.slice(0,ansiFileName.len()-ext.len()-1) + "_" + suffix;

            if (ext.len()) {
                filename += "." + ext;
            }
            remotePathEncoded = folder + nm.urlEncode(filename);
            i++;
        }
        local url = "https://webdav.yandex.ru" + remotePathEncoded;
        nm.setUrl(url);
        nm.addQueryHeader("Authorization", _GetAuthorizationString());
        nm.addQueryHeader("Connection", "close");
        nm.addQueryHeader("Content-Type", mimeType);
        nm.setMethod("PUT");
        nm.doUpload(FileName, "");
        _CheckResponseCode();

        nm.setCurlOptionInt(CURLOPT_FOLLOWLOCATION, 0); // disable CURLOPT_FOLLOWLOCATION   
        nm.addQueryHeader("Authorization", _GetAuthorizationString());
        nm.setUrl(url + "?publish");
        nm.enableResponseCodeChecking(false);
        nm.doPost("");
        _CheckResponseCode();

        local viewUrl = nm.responseHeaderByName("Location");

        if (viewUrl != "") {
            options.setViewUrl(viewUrl);
            return 1;
        }
    } catch ( ex ) {
        WriteLog("error", "Exception:" + ex.tostring());
    }

    return 0;
}

function GetFolderAccessTypeList() {
    return [
        tr("yandexdisk.privacy.private", "Private"),
		tr("yandexdisk.privacy.public", "Public"),
    ];
}

function GetServerParamList() {
    return {
        useWebdav = "Use WebDav",
        enableOAuth ="enableOAuth"
    };
}