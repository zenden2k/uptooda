appKey <- GetEnvDecode("IU_DROPBOX_APP_KEY");
appSecret <- GetEnvDecode("IU_DROPBOX_APP_SECRET");
accessType <- "app_folder";

const REDIRECT_URI = "https://svistunov.dev/blank.html";
const REDIRECT_URI_ESCAPED = "https:\\/\\/svistunov\\.dev\\/blank\\.html";

authStep1Url <- "https://api.dropbox.com/1/oauth/request_token";
authStep2Url <- "https://api.dropbox.com/1/oauth/access_token";

authCode <- "";
redirectUrl <- "";
    
function _SignRequest(url, token) {
    nm.addQueryHeader("Authorization", "Bearer " + token);
    
    return url;
}

function OnUrlChangedCallback(data) {
    local reg = CRegExp("^" + REDIRECT_URI_ESCAPED, "");
    if ( reg.match(data.url) ) {
        local br = data.browser;
        local regError = CRegExp("error=([^&]+)", "");
        if ( regError.match(data.url) ) {
            WriteLog("warning", regError.getMatch(1));
        } else {
            local codeRegexp = CRegExp("code=([^&]+)", "");
            if ( codeRegexp.match(data.url) ) {
                authCode = codeRegexp.getMatch(1);
            }
        }
        br.close();
    }
}

function RefreshToken() {
    local expiresIn = 0;
    try {
        expiresIn = ServerParams.getParam("expiresIn").tointeger();
    } catch (e) {
    }
    local refreshToken = ServerParams.getParam("refreshToken"); 

    if (time() > expiresIn && refreshToken != "") {
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
        nm.addQueryParam("redirect_uri", redirectUrl);
        nm.addQueryParam("client_id", appKey);
        nm.addQueryParam("client_secret", appSecret);
        nm.doPost("");

        if (nm.responseCode() == 200) {
            local t = ParseJSON(nm.responseBody());
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

function Authenticate() {
    local token = ServerParams.getParam("token");
    
    if (token != ""){
        return 1;
    }
    
    local server = WebServer();
    local port = 0;

    local htmlHead =  @"<html>
                            <head>
                                <meta http-equiv='content-type' content='text/html; charset=utf-8' />
                                <title>%s</title>
                            </head>
                            <body>";

    htmlHead = format(htmlHead, tr("oauth.title", "Authorization"));   
    
    local htmlFooter = "</body></html>";

    server.resource("^/$", "GET", function(d) {
        local responseBody = "";
        if ("code" in d.queryParams){
            authCode = d.queryParams.code;
            responseBody = htmlHead + "<h1>" + tr("oauth.title", "Authorization") + "</h1><p>" + tr("oauth.success", "Success! Now you can close this page.")+"</p>" + htmlFooter;
        } else {
            responseBody = htmlHead + "<h1>" + tr("oauth.title", "Authorization") + "</h1><p>" + tr("oauth.success", "Failed to obtain confirmation code") + "</p>" +  htmlFooter;
        }

        return {
            responseBody = responseBody,
            stopDelay = 500
        };
    }, null);

    local ports = [49707, 39517, 22690, 27966, 51502];

    foreach (localPort in ports) {
        port = server.bind(localPort);
        if (port != 0) {
            break;
        }
    }

    if (port != 0) {
        redirectUrl = "http://127.0.0.1:" + port;
        local url = "https://www.dropbox.com/oauth2/authorize?" + 
            "client_id=" + appKey  + 
            "&response_type=code" +
            "&token_access_type=offline" + 
            "&redirect_uri=" + nm.urlEncode(redirectUrl);
        ShellOpenUrl(url);
        server.start();
    } else {
        local browser = CWebBrowser();
        browser.setTitle(tr("dropbox.browser.title", "Dropbox authorization"));
        browser.setOnUrlChangedCallback(OnUrlChangedCallback, null);
        redirectUrl = REDIRECT_URI;
        local url = "https://www.dropbox.com/oauth2/authorize?" + 
                "client_id=" + appKey  + 
                "&response_type=code" +
                "&token_access_type=offline" + 
                "&redirect_uri=" + nm.urlEncode(redirectUrl);

        browser.navigateToUrl(url);
        browser.showModal();
    }
        
    return _ObtainAccessToken();
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
    _SignRequest(url, token);
    nm.addQueryHeader("Content-Type", "application/json");
    nm.doPost("null");
    ServerParams.setParam("token", "");
    
    if (nm.responseCode() == 200) {
        return 1;
    } else {
        local t = ParseJSON(nm.responseBody());
        if ("error" in t && t.error.rawget(".tag") == "invalid_access_token") {
            WriteLog("warning", "[dropbox.nut] Token already revoked.");
            return 1;
        }
    }
    return 0;
}

function min(a,b) {
    return a < b ? a : b;
}

function UploadFile(FileName, options) {		
    local task = options.getTask().getFileTask();
    local displayName = task.getDisplayName();
    local token = ServerParams.getParam("token");
    local url = null;
    local userPath = ServerParams.getParam("UploadPath");
    if ( userPath!="" && userPath[userPath.len()-1] != "/") {
        userPath+= "/";
    }
    const CHUNK_SIZE = 52428800;
    local fileSize = 0;
    try { 
        fileSize=GetFileSize(FileName);
    } catch ( ex ) {
        
    }
    
    if ( fileSize < 0 ) {
        WriteLog("error", "[dropbox.nut] fileSize < 0 ");
        return 0;
    }
    local path = "/"+ userPath;
    local remotePath =path + displayName;
    local fileId="";
    
    if ( fileSize > 150000000 ) {
        local chunkCount = ceil(fileSize.tofloat() / CHUNK_SIZE).tointeger();
        local session = null;
        local offset = 0;
        
        local session="";
        for(local i = 0; i < chunkCount; i++ ) {
            for ( local j =0; j < 2; j++ ) {
                try {
                    nm.setChunkOffset(offset);
                } catch ( ex ) {
                    WriteLog("error", "Your Image Uploader version does not support chunked uploads for big files. \r\nPlease update to the latest version");
                    return 0;
                }
                if( session==""){
                    url = "https://content.dropboxapi.com/2/files/upload_session/start" ;
                    _SignRequest(url, token);
                    local arg ={
                        close=false
                    };
                    local json = StrReplace(ToJSON(arg),"\n","");
                    nm.addQueryHeader("Dropbox-API-Arg", json);
                } else{
                    url = "https://content.dropboxapi.com/2/files/upload_session/append_v2" ;
                    _SignRequest(url, token);
                    local arg ={
                        cursor={
                            session_id=session,
                            offset=offset
                        },
                        close=false
                    };
                    local json = StrReplace(ToJSON(arg),"\n","");
                    nm.addQueryHeader("Dropbox-API-Arg", json);
                }
                local currentChunkSize = min(CHUNK_SIZE, fileSize - offset).tointeger();
                nm.setChunkSize(currentChunkSize);
                nm.addQueryHeader("Content-Type", "application/octet-stream");
                nm.setUrl(url);
                nm.doUpload(FileName,"");

                if (nm.responseCode() != 200) {
                    WriteLog("warning", "[dropbox.nut] Chunk upload failed, offset="+offset+", size="+currentChunkSize+(j< 1? "Trying again..." : ""));
                    if ( nm.responseCode() == 403 ) {
                        WriteLog("error", "[dropbox.nut] Upload failed. Access denied");
                        return 0;
                    }
                } else {
                    local t = ParseJSON(nm.responseBody());
                    if (session==""){
                        session = t.session_id;
                    }
                    offset += currentChunkSize;
                    
                    break;
                }
            }
            //return 0;
        }
        if ( session == "" ) {
            WriteLog("error", "[dropbox.nut] Upload failed");
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
        local json = StrReplace(ToJSON(arg),"\n","");
        nm.addQueryHeader("Dropbox-API-Arg", json);
        nm.addQueryHeader("Content-Type", "application/octet-stream");
        _SignRequest(url, token);
        nm.setMethod("POST");
        nm.doPost("");

        if ( nm.responseCode() != 200 ) {
            return 0;
        }
        local data = ParseJSON(nm.responseBody());

    } else {
        url = "https://content.dropboxapi.com/2/files/upload" ;
        _SignRequest(url, token);
        local arg ={
            path=remotePath,
            mode="add",
            autorename=true,
            mute=false
        };
        nm.addQueryHeader("Content-Type", "application/octet-stream");
        local json = StrReplace(ToJSON(arg),"\n","");
        nm.addQueryHeader("Dropbox-API-Arg", json);
        nm.setUrl(url);
        nm.doUpload(FileName, "");
    }

    local data = ParseJSON(nm.responseBody());
    if(nm.responseCode()!=200){
        WriteLog("error", "[dropbox.nut] " + nm.responseBody());
        return 0;
    }

    fileId = data.id;
    
    url = "https://api.dropboxapi.com/2/sharing/create_shared_link_with_settings" ;
    _SignRequest(url, token);
    local arg ={
            path=remotePath,
            settings={
                requested_visibility="public"
            }
    };
    local json = StrReplace(ToJSON(arg),"\n","");
    nm.addQueryHeader("Content-Type","application/json")
    nm.setUrl(url);
    nm.enableResponseCodeChecking(false);
    nm.doPost(json);
    
    local viewUrl = "";
    
    if(nm.responseCode()!=200){ 
        if (nm.responseCode() == 409) { // Shared link already exists
            data = ParseJSON(nm.responseBody());
            url = "https://api.dropboxapi.com/2/sharing/list_shared_links" ;
            _SignRequest(url, token);
            local arg ={
                    path=remotePath,
                    direct_only=true
            };
            local json = StrReplace(ToJSON(arg),"\n","");
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
                WriteLog("error", "[dropbox.nut] " + nm.responseBody());
                return 0;
            }
        } else {
            WriteLog("error", "[dropbox.nut] " + nm.responseBody());
            return 0;
        }
       
    } else {
        data = ParseJSON(nm.responseBody());
        viewUrl = data.url;
        options.setViewUrl( viewUrl);
    }
    
    if ( viewUrl != "" ) {
        return 1;
    }
     
    return 0;
}

function GetServerParamList() {
    return {
        UploadPath = "Upload Path"
    };
}