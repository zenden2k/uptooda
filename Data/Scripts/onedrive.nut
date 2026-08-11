redirectUri <- "https://login.microsoftonline.com/common/oauth2/nativeclient";
redirectUrlEscaped <- "https:\\/\\/login\\.microsoftonline\\.com\\/common\\/oauth2\\/nativeclient";
code <- "";
clientId <- GetEnvDecode("IU_ONE_DRIVE_CLIENT_ID");

function min(a,b) {
    return (a < b) ?  a : b;
}

function _GetAuthorizationString() {
    return ServerParams.getParam("tokenType") + " " + ServerParams.getParam("token");
}

function _ClearAuthData() {
    ServerParams.setParam("token", "");
    ServerParams.setParam("expiresIn", "");
    ServerParams.setParam("refreshToken", "");
    ServerParams.setParam("tokenType", "");
    ServerParams.setParam("prevLogin", "");
    ServerParams.setParam("tokenTime", "");
}

function _CheckResponse(except = false) {
    local t = ParseJSON(nm.responseBody());
    
    if ( nm.responseCode() == 403 || nm.responseCode() == 400  ) { 
        if ( nm.responseBody().find("Invalid token",0)!= null 
                || (("error" in t) && t.error == "invalid_grant")) {
            _ClearAuthData();
            if (except) {
                throw "unauthorized_exception";
            } 
            return -2;
        } else {
            WriteLog("error", "403 Access denied" );
            return 0;
        }
    } else if (nm.responseCode() == 401)  {
        _ClearAuthData();
        if (except) {
            throw "unauthorized_exception";
        } 
        return -2;
    } else if ( /*nm.responseCode() == 0 ||*/ (nm.responseCode() >= 400 && nm.responseCode() <= 499)) {
        WriteLog("error", "Response code " + nm.responseCode() + "\r\n" + nm.errorString() );
        return 0;
    }
    return 1;
}

function OnUrlChangedCallback(data) {
    local reg = CRegExp("^" +redirectUrlEscaped, "");
    if ( reg.match(data.url) ) {
        local br = data.browser;
        local regError = CRegExp("error=([^&]+)", "");
        if ( regError.match(data.url) ) {
            //WriteLog("warning", regError.getMatch(1));
        } else {
            local codeToken = CRegExp("code=([^&]+)", "");
            if ( codeToken.match(data.url) ) {
                code = codeToken.getMatch(1);
            }
        }
        br.close();
    }
}

function RefreshToken() {
    local token = ServerParams.getParam("token");
    local tokenType = ServerParams.getParam("tokenType");
    if ( token != "" && tokenType != "" ) {
        local tokenTime  = 0;
        local expiresIn = 0;
        local refreshToken = "";
        try {
            tokenTime = ServerParams.getParam("tokenTime").tointeger();
        } catch ( ex ) {

        }
        try {
        expiresIn = ServerParams.getParam("expiresIn").tointeger();
        } catch ( ex ) {

        }
        
        refreshToken = ServerParams.getParam("refreshToken");
        if ( time() > tokenTime + expiresIn && refreshToken != "") {
            // Refresh access token
            nm.setUrl("https://login.microsoftonline.com/common/oauth2/v2.0/token");
            nm.addPostField("refresh_token", refreshToken);
            nm.addPostField("client_id", clientId);
            nm.addPostField("grant_type", "refresh_token");
            nm.doPost("");
            local code =  _CheckResponse();
            if (code < 1) {
                WriteLog("error", "onedrive.nut: Unable to refresh access token.");
                return code;
            } else {
                local data =  nm.responseBody();
                local t = ParseJSON(data);
                if ("access_token" in t) {
                    token = t.access_token;
                    ServerParams.setParam("expiresIn", t.expires_in);
                    tokenType = t.token_type;
                    ServerParams.setParam("tokenType", tokenType);
                    ServerParams.setParam("token", token);
                    ServerParams.setParam("tokenTime", time().tostring());
                    ServerParams.setParam("refreshToken", t.refresh_token);
                    if ( token != "" ) {
                        return 1;
                    } else {
                        token = "";
                        tokenType = "";
                        return 0;
                    }
                } else {
                    WriteLog("error", "onedrive.nut: Unable to refresh access token.");
                    return 0;
                }
            }
        } else {
            return 1;
        }
    }
    return 1;
}

function Authenticate() {
    if (ServerParams.getParam("token") != "") {
        return 1;
    }
    local token = "";
    local tokenType = "";
    local login = ServerParams.getParam("Login");
    local scope = "offline_access files.readwrite";

    if(login == "" ) {
        WriteLog("error", "E-mail should not be empty!");
        return 0;
    }

    local url = "https://login.live.com/oauth20_authorize.srf?client_id=" + clientId + "&scope="+ nm.urlEncode(scope) + "&response_type=code&redirect_uri=" + redirectUri
    local browser = CWebBrowser();
    browser.setTitle(tr("onedrive.browser.title", "OneDrive authorization"));
    browser.setOnUrlChangedCallback(OnUrlChangedCallback, null);
    browser.navigateToUrl(url);
    browser.showModal();

    local confirmCode = code;
    if (confirmCode == "") {
        WriteLog("error", "Cannot authenticate without confirm code");
        return 0;
    }

    nm.setUrl("https://login.microsoftonline.com/common/oauth2/v2.0/token");
    nm.addPostField("code", confirmCode);
    nm.addPostField("client_id", clientId);
    nm.addPostField("redirect_uri", redirectUri);
    nm.addPostField("grant_type", "authorization_code");
    nm.doPost("");
    if (!_CheckResponse()) {
        return 0;
    }
    local data =  nm.responseBody();
    local t = ParseJSON(data);

    local accessToken = "";
    if ("access_token" in t) {
        accessToken = t.access_token;
    }

    if (accessToken != "") {
        token = accessToken;
        local timestamp = time();
        ServerParams.setParam("token", token);
        ServerParams.setParam("expiresIn", t.expires_in);
        ServerParams.setParam("refreshToken", t.refresh_token);
        tokenType = t.token_type;
        ServerParams.setParam("tokenType", tokenType);
        ServerParams.setParam("prevLogin", login);
        ServerParams.setParam("tokenTime", timestamp.tostring());
        return 1;
    } else {
        WriteLog("error", "Authentication failed");
    }
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
    if (token == "" ) {
        return 0;
    }

    ServerParams.setParam("token", "");
    ServerParams.setParam("refreshToken", "");

    local browser = CWebBrowser();
    browser.setTitle(tr("onedrive.browser.title", "OneDrive authorization"));
    browser.navigateToUrl("https://login.microsoftonline.com/common/oauth2/v2.0/logout?post_logout_redirect_uri=" + nm.urlEncode(redirectUri));
    browser.showModal();

    return 1;
}

function GetFolderList(list) {
    local parentId = list.parentFolder().getId();
    if (parentId != "" && parentId != "root") {
        return 1;
    }
    if (parentId == "") {
        local rootFolder = CFolderItem();
        rootFolder.setId("root");
        rootFolder.setTitle("/");
        list.AddFolderItem(rootFolder);
        return 1;
    }
    nm.addQueryHeader("Authorization", _GetAuthorizationString());
    nm.enableResponseCodeChecking(false);
    nm.doGet("https://graph.microsoft.com/v1.0/drive/root/children?select=id%2cname&filter=folder%20ne%20null");
    nm.enableResponseCodeChecking(true);
    local code = _CheckResponse();
    if (code < 1) {
        WriteLog("error", "onedrive: Unable to get folder list, response code: " + nm.responseCode());
        return code;
    } else {
        local t = ParseJSON(nm.responseBody());
        if ( t != null) {
            if ("error" in t) {
                if (t.error.code != "itemNotFound") {
                    WriteLog("error", "onedrive: " + t.error.message);
                } else {
                    WriteLog("error", "onedrive: You have no drives." );

                }
                return 0;
            }

            if ("value" in t)  {
                local num = t.value.len();
                for (local i = 0; i < num; i++) {
                    local item = t.value[i];
                    local folder = CFolderItem();
                    folder.setId(item.id);
                    folder.setTitle(item.name);
                    folder.setParentId("root");
                    list.AddFolderItem(folder);
                }
                return 1; // SUCCESS!
            }
        } else {
            WriteLog("error", "onedrive: failed to parse answer");
        }
    }

    return 0;
}

function CreateFolder(parentFolder, folder) {
    local logError = function(responseCode) { 
        WriteLog("error", "onedrive: Unable to create folder, response code: " + responseCode);
    }
    local parentId = parentFolder.getId();
    if (parentId == "root" || parentId == "") {
        nm.setUrl("https://graph.microsoft.com/v1.0/me/drive/root/children");
    } else{
        nm.setUrl("https://graph.microsoft.com/v1.0/me/drive/items/" + parentFolder.getId() + "/children");
    }

    nm.addQueryHeader("Authorization", _GetAuthorizationString());

    local data = {
        name = folder.getTitle(),
        "folder": { },
        "@microsoft.graph.conflictBehavior": "rename"
    };
    nm.addQueryHeader("Content-Type","application/json");
    nm.doPost(ToJSON(data));
    local code = _CheckResponse();
    if (code < 1) {
        logError(nm.responseCode());
        return code;
    }
    if (nm.responseCode() == 201) {
        local responseData = nm.responseBody();
        local item = ParseJSON(responseData);
        if ( item != null ) {
            folder.setId(item.id);
            folder.setTitle(item.name);
            return 1;
        }
    } else {
       logError(nm.responseCode()); 
    }
    return 0;
}

function ModifyFolder(folder) {
    local title = folder.getTitle();
    local id = folder.getId();

    if (id == "root" || id == "") {
        WriteLog("error", "Cannot rename root folder");
        return 0;
    }

    nm.setMethod("PATCH");
    nm.setUrl("https://graph.microsoft.com/v1.0/me/drive/items/" + id);
    nm.addQueryHeader("Authorization", _GetAuthorizationString());
    nm.addQueryHeader("Content-Type", "application/json");
    local postData = {
        name = title
    };

    nm.doPost(ToJSON(postData));
    local code = _CheckResponse();
    if (code < 1) {
        WriteLog("error", "onedrive: Unable to rename folder, response code: " + nm.responseCode());
        return code;
    } else {
        return 1;     
    }

    return 0;
}

function UploadFile(FileName, options) {
    local mimeType = GetFileMimeType(FileName);

    local postData = {
        item = {
            "@microsoft.graph.conflictBehavior": "rename"
        }
    };
    local folderId = options.getFolderID();
    if (folderId == "/" || folderId == "") {
        folderId = "root";
    }
    local onlyFileName = ExtractFileName(FileName);

    nm.enableResponseCodeChecking(false);
    nm.setUrl("https://graph.microsoft.com/v1.0/me/drive/items/" + folderId + ":/" + nm.urlEncode(onlyFileName) + ":/createUploadSession");
    nm.addQueryHeader("Authorization", _GetAuthorizationString());
    nm.addQueryHeader("Content-Type", "application/json");
    nm.setMethod("POST");
    nm.doPost(ToJSON(postData));
    const CHUNK_SIZE = 32768000;
    local code = _CheckResponse();
    if (code < 1) {
        WriteLog("error", "onedrive: Unable to create upload session file, response code: " + nm.responseCode());
        return code;
    } else {
        local t = ParseJSON(nm.responseBody());
        if (!("uploadUrl" in t)) {
            WriteLog("error", "onedrive: Failed to obtain upload URL.");
            return 0;
        }
        local uploadUrl = t.uploadUrl;
        local fileSize = GetFileSize(FileName).tointeger();
        local chunkCount = ceil(fileSize.tofloat() / CHUNK_SIZE).tointeger();

        for ( local i = 0; i < chunkCount; i++) {
            local offset = i * CHUNK_SIZE;
            local currentRequestSize = min(CHUNK_SIZE, fileSize - offset);
            nm.setUrl(uploadUrl);
            nm.setMethod("PUT");
            nm.setChunkSize(currentRequestSize);
            nm.setChunkOffset(offset);
            nm.addQueryHeader("Content-Length", currentRequestSize.tostring());
            nm.addQueryHeader("Content-Range", "bytes " + offset + "-"+ (offset+currentRequestSize-1) + "/"+ fileSize);
            nm.addQueryHeader("Transfer-Encoding","");
            nm.doUpload(FileName, "");
            code = _CheckResponse();
            if (code < 0) {
                WriteLog("error", "onedrive: Failed to upload a chunk, response code: " + nm.responseCode());
                return code;
            }
        }
        if (nm.responseCode() == 201) {
                local answer = ParseJSON(nm.responseBody());
                local fileId = answer.id;
                local postData2 = {
                    type = "view"
                };
                nm.setUrl("https://graph.microsoft.com/v1.0/me/drive/items/"+fileId + "/createLink");
                nm.addQueryHeader("Authorization", _GetAuthorizationString());
                nm.addQueryHeader("Content-Type", "application/json");
                nm.doPost(ToJSON(postData2));
                if (nm.responseCode() == 201) {
                    local answer2 = ParseJSON(nm.responseBody());
                    if ("link" in answer2) {
                        options.setViewUrl(answer2.link.webUrl);
                        return 1;
                    }
                }
        }
        
    }

    return 0;
}
