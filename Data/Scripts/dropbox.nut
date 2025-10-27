appKey <- "973quph3jxdgqoe";
appSecret <- "wloizpn331cc8zd";
accessType <- "app_folder";

redirectUri <- "https://svistunov.dev/callback";
redirectUrlEscaped <- "https:\\/\\/oauth\\.vk\\.com\\/blank\\.html";

authStep1Url <- "https://api.dropbox.com/1/oauth/request_token";
authStep2Url <- "https://api.dropbox.com/1/oauth/access_token";

token <- "";
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

function _DoLogin() {
	token = ServerParams.getParam("token");
	
	if ( token != ""){
		return 1;
	}
	
	local url = "https://www.dropbox.com/oauth2/authorize?" + 
			"client_id=" + appKey  + 
			"&response_type=token" +
			"&redirect_uri=" + nm.urlEncode(redirectUri);

	ShellOpenUrl(url);		
	
    local confirmCode = InputDialog(tr("dropbox.confirmation.text", "You need to need to sign in to your Dropbox account\r\nin web browser which just have opened and then copy\r\nconfirmation code into the text field below.\r\n\r\nPlease enter confirmation code:"), "");
       
    if (confirmCode != ""){
		local t = ParseJSON(confirmCode);
		token = t.access_token;
		accountId = t.account_id;
		//tokenType = t.token_type;

		ServerParams.setParam("token", token);
		ServerParams.setParam("accountId", accountId);	
		ServerParams.setParam("tokenTime", t.timestamp);	
		ServerParams.setParam("expiresIn", t.expires_in.tointeger());	

        /*local url = "https://api.dropboxapi.com/2/users/get_current_account";
        nm.addQueryHeader("Content-Type","")
        sendOauthRequest(url, token);
        WriteLog("warning", nm.responseBody() );*/
		return 1;
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
	local userPath = ServerParams.getParam("UploadPath");
	if ( userPath!="" && userPath[userPath.len()-1] != "/") {
		userPath+= "/";
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
	local path = "/"+ userPath;
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

function GetServerParamList()
{
	local a =
	{
        token = "token"
		UploadPath = "Upload Path"
	}
	return a;
}