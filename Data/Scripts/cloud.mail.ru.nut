// Cloud.mail.ru Upload API
if(ServerParams.getParam("folder") == "") {
	ServerParams.setParam("folder", "Screenshots") ;
}

function BeginLogin() {
    try {
        return Sync.beginAuth();
    }
    catch ( ex ) {
    }
    return true;
}

function EndLogin() {
    try {
        return Sync.endAuth();
    } catch ( ex ) {
        
    }
    return true;
}

function _CheckResponse() {
    if ( nm.responseCode() == 403 ) {
        if ( nm.responseBody().find("Invalid token",0)!= null) {
            WriteLog("warning", nm.responseBody());
            ServerParams.setParam("token", "");
            ServerParams.setParam("expiresIn", "");
            ServerParams.setParam("refreshToken", "");
            ServerParams.setParam("tokenType", "");
            ServerParams.setParam("prevLogin", "");
            ServerParams.setParam("tokenTime", "");
            return 0;
        } else {
            WriteLog("error", "403 Access denied" );
            return 0;
        }
    } else if ( /*nm.responseCode() == 0 ||*/ (nm.responseCode() >= 400 && nm.responseCode() <= 499)) {
        WriteLog("error", "Response code " + nm.responseCode() + "\r\n" + nm.errorString() );
        return 0;
    }
    return 1;
}

function _GetServerAddress(character) {
    nm.doGet("https://dispatcher.cloud.mail.ru/" + character);
    if (nm.responseCode() == 200) {
        local reg = CRegExp("(https://.+?) ", "");
        if (reg.match(nm.responseBody())) {
            return reg.getMatch(1);
        }
    }
    return null;
}

function Authenticate() { 
    local token = ServerParams.getParam("token");
    local tokenType = ServerParams.getParam("tokenType");
    local login = ServerParams.getParam("Login");
    local password = ServerParams.getParam("Password");
    local scope = "offline_access files.readwrite";

    if(login == "" ) {
        WriteLog("error", "E-mail should not be empty!");
        return 0;
    }

    nm.setUserAgent("CloudScreenshoterWindows 17.04.1017 beta 5cfe3ab6");
    local oServer = _GetServerAddress("o");
    
    nm.addQueryParam("client_id", "cloud-win");
    nm.addQueryParam("grant_type", "password");
    nm.addQueryParam("username", login);
    nm.addQueryParam("password", password);
    nm.setUrl(oServer);
    nm.doPost("");
    
    if (nm.responseCode() == 200) {
        local t = ParseJSON(nm.responseBody());
        local accessToken = "";
        if ("access_token" in t) {
            accessToken = t.access_token;
        }
            
        if ( accessToken != "" ) {
            token = accessToken;
            local timestamp = time();
            ServerParams.setParam("token", token);
            ServerParams.setParam("expiresIn", t.expires_in);
            ServerParams.setParam("refreshToken", t.refresh_token);
            ServerParams.setParam("prevLogin", login);
            ServerParams.setParam("tokenTime", timestamp.tostring());
            return 1;
        }    else {
            local errorStr = "";
            if ("error"  in t) {
                errorStr = t.error;
            }
            WriteLog("error", "cloud.mail.ru: Authentication failed. " + errorStr);
        }
    } else {
        WriteLog("error", "cloud.mail.ru: unable to obtain token (login failed)");
    }  
    
    return 0;        
} 

function DoLogin() {
    if (!BeginLogin() ) {
        return false;
    }
    local res = Authenticate();
    
    EndLogin();
    return res;
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
            local oServer = _GetServerAddress("o");
            // Refresh access token
            nm.setUrl(oServer);
            nm.addQueryParam("refresh_token", refreshToken); 
            nm.addQueryParam("client_id", "cloud-win"); 
            nm.addQueryParam("grant_type", "refresh_token"); 
            nm.doPost("");
            if ( _CheckResponse() ) {
                local data =  nm.responseBody();
                local t = ParseJSON(data);
                if ("access_token" in t) {
                    token = t.access_token;
                    ServerParams.setParam("expiresIn", t.expires_in);
                    ServerParams.setParam("token", token);
                    ServerParams.setParam("tokenTime", time().tostring());
                    if ( token != "" ) {
                        return 1;
                    } else {
                        token = "";
                        tokenType = "";
                        return 0;
                    }
                } else {
                    WriteLog("error", "cloud.mail.ru: Unable to refresh access token.");
                    return 0;
                }
            } else {
                WriteLog("error", "cloud.mail.ru: Unable to refresh access token.");
            }
        } else {
            return 1;
        }
    }
    return 1;
}

function GetFolderList(list) {
    return 0;
}

function _PackNumber(number) {
    local vec = "";
    while (number >= 0x80) {
        local b = number | 0x80;
        vec += format("%c", b);
        number = number >> 7;
    }
    vec += format("%c", number);
    return vec;
}

function _HexToBytes(hex) {
    local res = "";
    local lookup = ['0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'];
    for (local i = 0; i < hex.len(); i += 2) {
        local byteString = "" + hex.slice(i, i+2);
        local dec = lookup.find(byteString[0]) * 0x10 + lookup.find(byteString[1])
        res += format("%c", dec);   
    }
    return res;
}

function _TrimStr(str) {
    local i = str.len() - 1;
    while (i >= 0 && str[i] == 0) {
        i--;
    }
    return str.slice(0, i+1);
}

function _SendMetaRequest(nc, url, remoteFileName, hashUpper, size, create) {
    nc.setUrl(url);
    local vec = "";
    vec += "\x67\x0";
    vec += format("%c", remoteFileName.len() + 1);
    vec += remoteFileName;
    vec += "\x0";
    vec += _PackNumber(size);
    vec += _PackNumber(time());
    vec += "\x0";
    vec += _HexToBytes(hashUpper);
    vec += create? "\x1": "\x0";
    nm.setMethod("POST");
    return nc.doUpload("", vec);
}

function _CreateFolder(nc, url, folderName) {
    if(!DoLogin()) {
        return 0;
    }
    nc.setUrl(url);
    local vec = "\x6a\x0";
    vec += format("%c", folderName.len() + 1);
    vec += folderName;
    vec += "\x0\x0";
    nm.setMethod("POST");
    return nc.doUpload("", vec);
}
        
function UploadFile(FileName, options) {
    if(!DoLogin()) {
        return -1;
    }
    local remoteFolderName = ServerParams.getParam("folder");
    if (remoteFolderName == "") {
        remoteFolderName = "Screenshots";
    }
    local onlyFileName = ExtractFileName(FileName);
    local remoteFolder = "/" + remoteFolderName + "/";
    local remoteFileName =  onlyFileName;
    local mimeType = GetFileMimeType(FileName);
    local token = ServerParams.getParam("token");
    local fileSize = GetFileSize(FileName);

    local uServer = _GetServerAddress("u");
    local fileHash = sha1_file_prefix(FileName, "mrCloud", fileSize.tostring()).toupper();
    //WriteLog("error", fileHash);
    
    local metaServer = _GetServerAddress("m");
    
    local res = _CreateFolder(nm, metaServer + "?client_id=cloud-win&token=" + token, "/" + remoteFolderName );
    if (!res || nm.responseCode() != 200) {  
        WriteLog("error", "Unable to create folder '" + remoteFolderName + "' on remote server.");
        return 0;
    }  
    
    // Creating stub image
    res = _SendMetaRequest(nm, metaServer + "?client_id=cloud-win&token=" + token, remoteFolder + remoteFileName, "01068324637F894C3AADC0FCDFA33B6A40CB4AD6", 23696, true);
    if (!res || nm.responseCode() != 200) {  
        WriteLog("error", "Unable to create stub file on remote server");
        return 0;
    }    
    local webLinkServer = _GetServerAddress("w");
    nm.setUrl(webLinkServer + "create?client_id=cloud-win&token="+ token);
    nm.addQueryParam("file", remoteFileName);
    nm.addQueryParam("folder", "/" + remoteFolder);
    nm.addQueryParam("shortlink", "1");
    nm.doPost("");
    
    if (nm.responseCode() == 200) {
        local data = nm.responseBody();
        local len = data[0];
        local str = data.slice(1, len);
        local len2 = data[len+1];
        local shortLink = _TrimStr(data.slice(len+2, len+2+len2));
        
        nm.enableResponseCodeChecking(false);
        nm.doGet(uServer + "info/" +  fileHash + "?client_id=cloud-win&token="+token);
        nm.enableResponseCodeChecking(true);
        
        if (nm.responseCode() == 404 ) {
            // File with this hash not found on server
            // Uploading file
            nm.setUrl(uServer +  fileHash + "?client_id=cloud-win&token=" + token); 
            nm.addQueryHeader("Content-Type", "");
            nm.addQueryHeader("Accept-Encoding", "");
            nm.addQueryHeader("Transfer-Encoding", "");
            nm.addQueryHeader("Content-Length", fileSize.tostring());
            nm.setMethod("PUT");
            nm.doUpload(FileName, "");
                
            if (nm.responseCode() == 201) {
                local fileHash2 = nm.responseBody();
                if ( fileHash2 != fileHash) {
                    WriteLog("error", "Hash of uploaded file doesn't match local file's hash.");
                    return 0;
                }
                    
                //nm.setUrl(metaServer + "?client_id=cloud-win&token=" + token);
                res = _SendMetaRequest(nm, metaServer + "?client_id=cloud-win&token=" + token, remoteFolder + remoteFileName, fileHash, fileSize, false);
                if (res && nm.responseCode() == 200 && shortLink != "") {
                    options.setViewUrl("https://s.mail.ru/" + shortLink);
                    return 1;
                }
            } else {
                WriteLog("error", "cloud.mail.ru: Upload failed, response code=" + nm.responseCode());
            }
        } else if (nm.responseCode() == 200){
            // File with this hash already exists on server
            local reg = CRegExp("hash:[0-9A-F]+", "");
            if (reg.match(nm.responseBody())) {
                WriteLog("info", "You are lucky! Hash found on server!");    
                // Hash found on server
                _SendMetaRequest(nm, metaServer + "?client_id=cloud-win&token=" + token, remoteFolder + remoteFileName, fileHash, fileSize, false);
                if (nm.responseCode() == 200 && shortLink != "") {
                    options.setViewUrl("https://s.mail.ru/" + shortLink);
                    return 1;
                }
            }
        } else {
            WriteLog("error", "cloud.mail.ru: Unable to check hash on server.");
        }    
        
    } else {
        WriteLog("error", "cloud.mail.ru: Failed to create short link. Response code=" + nm.responseCode());
    }
    return 0;
}

function GetServerParamList() {
	return {
		folder = "Remote folder"
	};
}
