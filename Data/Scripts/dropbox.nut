appKey <- "84rxb5xxbi7gkvo";
appSecret <- "wwukxwreo6ig30f";

redirectUri <- "https://svistunov.dev/callback";

authStep1Url <- "https://api.dropbox.com/1/oauth/request_token";
authStep2Url <- "https://api.dropbox.com/1/oauth/access_token";

token <- "";
authCode <- "";
accountId <- "";

regMatchOffset <- 0;

try {
	local ver = GetAppVersion();
	if ( ver.Build > 4422 ) {
		regMatchOffset = 1;
	}
} catch ( ex ) {
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

function tr(key, text) {
	try {
		return Translate(key, text);
	}
	catch(ex) {
		return text;
	}
}
	
function regex_simple(data,regStr,start)
{
	local ex = regexp(regStr);
	local res = ex.capture(data, start);
	local resultStr = "";
	if(res != null){	
		resultStr = data.slice(res[1].begin, res[1].end);
	}
		return resultStr;
}

function _WriteLog(type,message) {
	try {
		WriteLog(type, message);
	} catch (ex ) {
		print(type + " : " + message);
	}
}

function signRequest(url, token) {
	nm.addQueryHeader("Authorization", "Bearer " + token);
	
	return url;
}

function sendOauthRequest(url, token) {
	nm.setUrl(url);
	signRequest(url, token);
	nm.doPost("" );
	return 0;
}
function openUrl(url) {
	try{
		return ShellOpenUrl(url);
	}catch(ex){}

	system("start "+ reg_replace(url,"&","^&") );
}

function RefreshToken() {
    local expiresIn = 0;
    try {
        expiresIn = ServerParams.getParam("expiresIn").tointeger();
    } catch (e) {
    }
    local refreshToken = ServerParams.getParam("refreshToken"); 

    if (time() + 10 > expiresIn && refreshToken != "") {
        nm.setUrl("https://api.dropboxapi.com/oauth2/token");
        nm.addQueryParam("grant_type", "refresh_token");
        nm.addQueryParam("client_id", appKey);
        nm.addQueryParam("client_secret", appSecret);
        nm.addQueryParam("refresh_token", refreshToken);
        nm.doPost("");

        if (nm.responseCode() == 200) {
            local t = ParseJSON(nm.responseBody());
            ServerParams.setParam("token", t.access_token);
            ServerParams.setParam("expiresIn", t.expires_in + time());
            return 1;
        } else {
            WriteLog("error", "[dropbox.nut] Unable to refresh  token, response code: " + nm.responseCode());
            return 0;
        }
    }
    return 1;
}

function _ObtainAccessToken()  {
    if (authCode != ""){
        local url = "https://api.dropboxapi.com/oauth2/token";
        nm.setUrl(url);
        nm.addQueryParam("code", authCode);
        nm.addQueryParam("grant_type", "authorization_code");
        nm.addQueryParam("redirect_uri", redirectUri);
        nm.addQueryParam("client_id", appKey);
        nm.addQueryParam("client_secret", appSecret);
        nm.doPost("");

        if (nm.responseCode() == 200) {
            local t = ParseJSON(nm.responseBody());
			token = t.access_token;
            ServerParams.setParam("token", t.access_token);
            ServerParams.setParam("refreshToken", t.refresh_token);
            ServerParams.setParam("expiresIn", t.expires_in + time());
            ServerParams.setParam("accountId", t.account_id);	
            ServerParams.setParam("tokenTime", time().tostring());	
            ServerParams.setParam("uid", t.uid);
            authCode = "";	
            return 1;
        } else {
            WriteLog("error", "[dropbox.nut] Unable to obtain bearer token, response code: " + nm.responseCode());
        } 
    } 
    return 0;
}

function _DoLogin() {
	token = ServerParams.getParam("token");
	
	if (token != "") {
		if (RefreshToken() == 1) {
			return 1;
		}
	}
	
	local url = "https://www.dropbox.com/oauth2/authorize?" + 
			"client_id=" + appKey  + 
			"&response_type=code" +
			"&token_access_type=offline" +
			"&redirect_uri=" + nm.urlEncode(redirectUri);

	ShellOpenUrl(url);		
	
    authCode = InputDialog(tr("dropbox.confirmation.text", "You need to need to sign in to your Dropbox account\r\nin web browser which just have opened and then copy\r\nconfirmation code into the text field below.\r\n\r\nPlease enter confirmation code:"), "");
       
    if (authCode != "") {
		return _ObtainAccessToken();
	}
    
	return 0;
}

function DoLogin() {
	if (!BeginLogin() ) {
		return false;
	}
	local res = _DoLogin();
	
	EndLogin();
	return res;
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
    local url = "https://api.dropboxapi.com/2/auth/token/revoke";
    nm.setUrl(url);
    signRequest(url, token);
    nm.addQueryHeader("Content-Type", "application/json");
    nm.doPost("null");
    ServerParams.setParam("token", "");
    
    if (nm.responseCode() == 200) {
        return 1;
    } else {
        local t = ParseJSON(nm.responseBody());
        if ("error" in t && t.error.rawget(".tag") == "invalid_access_token") {
            WriteLog("error", "Token already revoked.");
            return 1;
        }
    }
    return 0;
}

function min(a,b) {
	return a < b ? a : b;
}
function  UploadFile(FileName, options) {		
	if (!DoLogin() ) {
		return 0;
	}
	local url = null;
	local folderId = options.getFolderID();
    if (folderId == "/") {
        folderId = ""; 
    }

	local chunkSize = (50*1024*1024).tofloat();
	local fileSize = 0;
	try { 
		fileSize=GetFileSize(FileName);
	} catch ( ex ) {
		
	}
	
	if ( fileSize < 0 ) {
		_WriteLog("error","fileSize < 0 ");
		return 0;
	}
	local path = folderId == "" ? "/" : folderId;
    local remotePath =path+ExtractFileName(FileName);
    local fileId="";
	if ( fileSize > 150000000 ) {
		local chunkCount = ceil(fileSize / chunkSize);
		local session = null;
		local offset = 0;
        
        local session="";
		for(local i = 0; i < chunkCount; i++ ) {
			for ( local j =0; j < 2; j++ ) {
				try {
					nm.setChunkOffset(offset.tofloat());
				} catch ( ex ) {
					_WriteLog("error", "Your Image Uploader version does not support chunked uploads for big files. \r\nPlease update to the latest version");
					return 0;
				}
				if( session==""){
                    url = "https://content.dropboxapi.com/2/files/upload_session/start" ;
                    signRequest(url, token);
                    local arg ={
                        close=false
                    };
                    local json = reg_replace(ToJSON(arg),"\n","");
                    nm.addQueryHeader("Dropbox-API-Arg", json);
                } else{
                    url = "https://content.dropboxapi.com/2/files/upload_session/append_v2" ;
                    signRequest(url, token);
                    local arg ={
                        cursor={
                            session_id=session,
                            offset=offset
                        },
                        close=false
                    };
                    local json = reg_replace(ToJSON(arg),"\n","");
                    nm.addQueryHeader("Dropbox-API-Arg", json);
                }
				local chunkSize = min(chunkSize,fileSize.tofloat()-offset);
				nm.setChunkSize(chunkSize);
                nm.addQueryHeader("Content-Type", "application/octet-stream");
				nm.setUrl(url);
				nm.doUpload(FileName,"");
                
				if ( nm.responseCode() != 200 ) {
					_WriteLog("warning","Chunk upload failed, offset="+offset+", size="+chunkSize+(j< 1? "Trying again..." : ""));
					if ( nm.responseCode() == 403 ) {
						_WriteLog("error","Upload failed. Access denied");
						return 0;
					}
				} else {
					local t = ParseJSON(nm.responseBody());
					if(session==""){
                        session = t.session_id;
                    }else{
                        
                    }
                    offset += chunkSize;
                    
					break;
				}
			}
			//return 0;
		}
		if ( session=="" ) {
			_WriteLog("error","Upload failed");
			return 0;
		}
		url = "https://content.dropboxapi.com/2/files/upload_session/finish";
		
		nm.setUrl(url);
        local arg ={
            cursor={
                session_id=session,
                offset=offset
            },
            commit={
                path=remotePath,
                mode="add",
                autorename=true,
                mute=false
            }
        };
        local json = reg_replace(ToJSON(arg),"\n","");
        nm.addQueryHeader("Dropbox-API-Arg", json);
        nm.addQueryHeader("Content-Type", "application/octet-stream");
		signRequest(url, token);
		nm.setMethod("POST");
		nm.doPost("");

		if ( nm.responseCode() != 200 ) {
            return 0;
			//_WriteLog("error",nm.responseCode().tostring());
		}
        local data = ParseJSON(nm.responseBody());

	} else {
		url = "https://content.dropboxapi.com/2/files/upload" ;
		signRequest(url, token);
        local arg ={
            path=remotePath,
            mode="add",
            autorename=true,
            mute=false
        };
        nm.addQueryHeader("Content-Type", "application/octet-stream");
        local json = reg_replace(ToJSON(arg),"\n","");
        nm.addQueryHeader("Dropbox-API-Arg", json);
		nm.setUrl(url);
		nm.doUpload(FileName,"");
	}

    local data = ParseJSON(nm.responseBody());
    if(nm.responseCode()!=200){
        _WriteLog("error",nm.responseBody());
        return 0;
    }

    fileId = data.id;
    
    url = "https://api.dropboxapi.com/2/sharing/create_shared_link_with_settings" ;
	signRequest(url, token);
    local arg ={
            path=remotePath,
            settings={
                requested_visibility="public"
            }
    };
    local json = reg_replace(ToJSON(arg),"\n","");
    //_WriteLog("error",json);
    nm.addQueryHeader("Content-Type","application/json")
	nm.setUrl(url);
    nm.enableResponseCodeChecking(false);
	nm.doUpload("",json);
    
    local viewUrl = "";
    
    if(nm.responseCode()!=200){ 
        if (nm.responseCode() == 409) { // Shared link already exists
            data = ParseJSON(nm.responseBody());
            url = "https://api.dropboxapi.com/2/sharing/list_shared_links" ;
            signRequest(url, token);
            local arg ={
                    path=remotePath,
                    direct_only=true
            };
            local json = reg_replace(ToJSON(arg),"\n","");
            nm.addQueryHeader("Content-Type","application/json")
            nm.setUrl(url);
            nm.doUpload("",json);
            if (nm.responseCode() == 200) {
                data = ParseJSON(nm.responseBody());
                if ("links" in data && data.links.len() > 0){
                    viewUrl = data.links[0].url;
                    options.setViewUrl( viewUrl );
                }
            } else {
                _WriteLog("error",nm.responseBody());
                return 0;
            }
        } else {
             _WriteLog("error",nm.responseBody());
            return 0;
        }
       
    } else {
        data = ParseJSON(nm.responseBody());
        viewUrl =data.url;
        options.setViewUrl( viewUrl);
    }
	
	if ( viewUrl != "" ) {
		return 1;
	}
 	
	return 0;
}

function GetFolderList(list) {
    local token = ServerParams.getParam("token");
    if (token == "") {
        return -2; // Not authenticated
    }

    local parentId = list.parentFolder().getId();
    
    // Create root folder entry
    if (parentId == "") {
        local rootFolder = CFolderItem();
        rootFolder.setId("/");
        rootFolder.setTitle("/ (root)");
        rootFolder.setSummary("");
        list.AddFolderItem(rootFolder);
        return 1;
    }

    // List contents of the current folder
    local url = "https://api.dropboxapi.com/2/files/list_folder";
    nm.setUrl(url);
    _SignRequest(url, token);
    nm.addQueryHeader("Content-Type", "application/json");

    // Prepare request body
    local requestBody = {
        path = parentId == "/" ? "" : parentId,
        recursive = false,
        include_media_info = false,
        include_deleted = false,
        include_has_explicit_shared_members = false
    };
    
    nm.doPost(ToJSON(requestBody));

    if (nm.responseCode() != 200) {
        WriteLog("error", "[dropbox.nut] Failed to list folder, response code: " + nm.responseCode());
        if (nm.responseCode() == 401) {
            ServerParams.setParam("token", ""); // Invalidate token
            return -2; // Authentication error
        }
        return 0;
    }

    local response = ParseJSON(nm.responseBody());
    
    // Process entries - only add folders to the list
    if ("entries" in response) {
        foreach (entry in response.entries) {
            if (entry[".tag"] == "folder") {
                local folder = CFolderItem();
                folder.setId(entry.path_lower);
                folder.setTitle(entry.name);
                folder.setSummary("");
                folder.setParentId(parentId);
                list.AddFolderItem(folder);
            }
        }
    }
    
    return 1;
}

function CreateFolder(parentAlbum, album) {
    local token = ServerParams.getParam("token");
    if (token == "") {
        return 0;
    }

    local parentId = parentAlbum.getId();
    
    local folderName = album.getTitle();
    if (folderName == "") {
        return 0;
    }
    
    // Construct the full path for the new folder
    local path = (parentId == "" || parentId == "/") ? "/" + folderName : parentId + "/" + folderName;

    local url = "https://api.dropboxapi.com/2/files/create_folder_v2";
    nm.setUrl(url);
    _SignRequest(url, token);
    nm.addQueryHeader("Content-Type", "application/json");

    // Prepare request body
    local requestBody = {
        path = path,
        autorename = false
    };
    
    nm.doPost(ToJSON(requestBody));

    if (nm.responseCode() != 200 && nm.responseCode() != 201) {
        WriteLog("error", "[dropbox.nut] Failed to create folder, response code: " + nm.responseCode());
        if (nm.responseCode() == 409) {
            WriteLog("error", "[dropbox.nut] Folder already exists");
        } else if (nm.responseCode() == 401) {
            ServerParams.setParam("token", ""); // Invalidate token
        }
        return 0;
    }

    local response = ParseJSON(nm.responseBody());
    if ("metadata" in response) {
        album.setId(response.metadata.path_lower);
        album.setParentId(parentId);
    }
    
    return 1;
}

function ModifyFolder(folder) {
    local token = ServerParams.getParam("token");
    if (token == "") {
        return 0;
    }

    local oldPath = folder.getId();
    if (oldPath == "" || oldPath == "/") {
        WriteLog("error", "[dropbox.nut] Cannot rename root folder");
        return 0;
    }
    
    local parentId = folder.getParentId();
    if (parentId == "") {
        parentId = "/"; // Default to root
    }
    
    local newName = folder.getTitle();
    if (newName == "") {
        return 0;
    }
    
    // Construct the new path for the folder
    local newPath = parentId == "/" ? "/" + newName : parentId + "/" + newName;
    
    local url = "https://api.dropboxapi.com/2/files/move_v2";
    nm.setUrl(url);
    _SignRequest(url, token);
    nm.addQueryHeader("Content-Type", "application/json");

    // Prepare request body
    local requestBody = {
        from_path = oldPath,
        to_path = newPath,
        allow_shared_folder = false,
        autorename = false,
        allow_ownership_transfer = false
    };
    
    nm.doPost(ToJSON(requestBody));

    if (nm.responseCode() != 200) {
        WriteLog("error", "[dropbox.nut] Failed to rename folder, response code: " + nm.responseCode());
        if (nm.responseCode() == 401) {
            ServerParams.setParam("token", ""); // Invalidate token
        }
        return 0;
    }

    local response = ParseJSON(nm.responseBody());
    if ("metadata" in response) {
        folder.setId(response.metadata.path_lower);
    }
    
    return 1;
}

function reg_replace(str, pattern, replace_with)
{
	local resultStr = str;	
	local res;
	local start = 0;

	while( (res = resultStr.find(pattern,start)) != null ) {	

		resultStr = resultStr.slice(0,res) +replace_with+ resultStr.slice(res + pattern.len());
		start = res + replace_with.len();
	}
	return resultStr;
}




function hex2int(str){
	local res = 0;
	local step = 1;
	for( local i = str.len() -1; i >= 0; i-- ) {
		local val = 0;
		local ch = str[i];
		if ( ch >= 'a' && ch <= 'f' ) {
			val = 10 + ch - 'a';
		}
		else if ( ch >= '0' && ch <= '9' ) {
			val = ch - '0';
		}
		res += step * val;
		step = step * 16;
	}
	return res;
}

function unescape_json_string(data) {
    local tmp;

    local ch = 0x0424;
	local result = data;
	local ex = regexp("\\\\u([0-9a-fA-F]{1,4})");
	local start = 0;
	local res = null;
	for(;;) {
		res = ex.capture(data, start);
		local resultStr = "";
		if (res == null){
			break;
		}
			
		resultStr = data.slice(res[1].begin, res[1].end);
		ch = hex2int(resultStr);
		start = res[1].end;
		 if(ch>=0x00000000 && ch<=0x0000007F)
			tmp = format("%c",(ch&0x7f));
		else if(ch>=0x00000080 && ch<=0x000007FF)
			tmp = format("%c%c",(((ch>>6)&0x1f)|0xc0),((ch&0x3f)|0x80));
		else if(ch>=0x00000800 && ch<=0x0000FFFF)
		   tmp= format("%c%c%c",(((ch>>12)&0x0f)|0xe0),(((ch>>6)&0x3f)|0x80),((ch&0x3f)|0x80));
			result = reg_replace( result, "\\u"+resultStr, tmp);
   
	}

    return result;
}