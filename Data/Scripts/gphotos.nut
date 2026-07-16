CLIENT_ID <-  GetEnvDecode("IU_GOOGLE_PHOTOS_CLIENT_ID");
CLIENT_SECRET <- GetEnvDecode("IU_GOOGLE_PHOTOS_CLIENT_SECRET");

function _GetAuthorizationString() {
    local token = ServerParams.getParam("token");
    local tokenType = ServerParams.getParam("tokenType");
    return tokenType + " " + token ;
}

function Authenticate() {
    local login = ServerParams.getParam("Login");
    local scope = "https://www.googleapis.com/auth/photoslibrary https://www.googleapis.com/auth/photoslibrary.sharing";
    //local redirectUrl = "urn:ietf:wg:oauth:2.0:oob";

    if(login == "" ) {
        WriteLog("error", "E-mail should not be empty!");
        return 0;
    }
    
    local token = ServerParams.getParam("token");
    local tokenType = ServerParams.getParam("tokenType");
    if ( token != "" && tokenType != "") {
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
            nm.setUrl("https://www.googleapis.com/oauth2/v3/token");
            nm.addQueryParam("refresh_token", refreshToken); 
            nm.addQueryParam("client_id", CLIENT_ID); 
            nm.addQueryParam("client_secret", CLIENT_SECRET); 
            nm.addQueryParam("grant_type", "refresh_token"); 
            nm.doPost("");
            if ( _CheckResponse() ) {
                local parsedData = ParseJSON(nm.responseBody());
                if ("access_token" in parsedData) {
                    token = parsedData.access_token;
                }
                ServerParams.setParam("expiresIn", "" + parsedData.expires_in);
                tokenType = parsedData.token_type;
                ServerParams.setParam("tokenType", tokenType);
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
                return 0; //<-- need to check this
            }
        } else {
            return 1;
        }
    }

    local server = WebServer();
    local confirmCode = "";

    server.resource("^/$", "GET", function(d) {
        local responseBody = "";
        if ("code" in d.queryParams){
            confirmCode = d.queryParams.code;
            responseBody = "<h1>" + tr("googledrive.oauth.title", "Authorization") + "</h1><p>" + tr("googledrive.oauth.success", "Success! Now you can close this page.")+"</p>";
        } else {
            responseBody = "<h1>" + tr("googledrive.oauth.title", "Authorization") + "</h1><p>" + tr("googledrive.oauth.success", "Failed to obtain confirmation code") + "</p>";
        }

        return {
            responseBody = responseBody,
            stopDelay = 500
        };
    }, null);

    local port = server.bind(0);

    local redirectUrl = "http://127.0.0.1:" + port + "/";

    local url = "https://accounts.google.com/o/oauth2/auth?scope="+ nm.urlEncode(scope) +"&redirect_uri="+redirectUrl+"&response_type=code&"+ "client_id="+CLIENT_ID;
    ShellOpenUrl(url);
    server.start();

    if ( confirmCode == "" ) {
        WriteLog("error", "Cannot authenticate without confirm code");
        return 0;
    }
    
    nm.setUrl("https://www.googleapis.com/oauth2/v3/token");
    nm.addQueryParam("code", confirmCode); 
    nm.addQueryParam("client_id", CLIENT_ID); 
    nm.addQueryParam("client_secret", CLIENT_SECRET); 
    nm.addQueryParam("redirect_uri", redirectUrl); 
    nm.addQueryParam("grant_type", "authorization_code"); 
    nm.doPost("");
    if ( !_CheckResponse() ) {
        return 0;
    }
    local parsedResponse = ParseJSON(nm.responseBody());
    local accessToken = "";
    if ("access_token" in parsedResponse) {
        accessToken = parsedResponse.access_token;
    }
    local timestamp = time();
    if ( accessToken != "" ) {
        token = accessToken;
        
        ServerParams.setParam("token", token);
        ServerParams.setParam("expiresIn", "" + parsedResponse.expires_in);
        ServerParams.setParam("refreshToken", parsedResponse.refresh_token);
        tokenType = parsedResponse.token_type;
        
        ServerParams.setParam("tokenType", tokenType);
        ServerParams.setParam("tokenTime", ""+timestamp);
        return 1;
    }	else {
        WriteLog("error", "Autentication failed");
    }
    return 0;		
} 

function RefreshToken() {
    local token = ServerParams.getParam("token");
    local tokenType = ServerParams.getParam("tokenType");

    if (token != "" && tokenType != "") {
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
        if (time() + 10 > tokenTime + expiresIn && refreshToken != "") {
            // Refresh access token
            nm.setUrl("https://www.googleapis.com/oauth2/v3/token");
            nm.addQueryParam("refresh_token", refreshToken);
            nm.addQueryParam("client_id", CLIENT_ID);
            nm.addQueryParam("client_secret", CLIENT_SECRET);
            nm.addQueryParam("grant_type", "refresh_token");
            nm.doPost("");
            if (_CheckResponse()) {
                local data =  nm.responseBody();
                local t = ParseJSON(data);
                if ("access_token" in t) {
                    token = t.access_token;
                    if ( token != "" ) {
                        ServerParams.setParam("expiresIn", t.expires_in);
                        ServerParams.setParam("tokenType", t.token_type);
                        ServerParams.setParam("token", token);
                        ServerParams.setParam("tokenTime", time().tostring());
                        return 1;
                    }
                }
                return 0;
            }
        } else {
            return 1;
        }
        return 1;
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
    if (token == "") {
        return 0;
    }
    nm.setUrl("https://accounts.google.com/o/oauth2/revoke?token=" + nm.urlEncode(token));
    nm.doPost("");

    if (nm.responseCode() == 200) {
        ServerParams.setParam("token", "");
        ServerParams.setParam("refreshToken", "");
        return 1;
    } else {
        local t = ParseJSON(nm.responseBody());

        if ("error" in t && t.error == "invalid_token") {
            ServerParams.setParam("token", "");
            ServerParams.setParam("refreshToken", "");
            return 1;
        }
    }
    return 0;
}

function _ParseAlbumList(data,list) {
    local t = ParseJSON(data);
    if ( t != null ) {
        local count = t.albums.len();
        for ( local i = 0; i < count; i++ ) {
            local item = t.albums[i];
      
            if ("isWriteable" in item && item.isWriteable) {
                local folder = CFolderItem();
                folder.setId(item.id);
                folder.setTitle(item.title);
                //folder.setSummary(summary);
                folder.setViewUrl(item.productUrl);
                list.AddFolderItem(folder);
            }
        }
        return 1;
    }

    return 0;
}

function _CheckResponse() {
    if ( nm.responseCode() == 403 ) {
        if ( nm.responseBody().find("Invalid token",0)!= null) {
            //WriteLog("warning", nm.responseBody());
            ServerParams.setParam("token", "");
            ServerParams.setParam("expiresIn", "");
            ServerParams.setParam("refreshToken", "");
            ServerParams.setParam("tokenType", "");
            ServerParams.setParam("prevLogin", "");
            ServerParams.setParam("tokenTime", "");
            throw "unauthorized_exception";
        } else {
            WriteLog("error", "403 Access denied" );
            return 0;
        }
    } else if ( nm.responseCode() == 0 || (nm.responseCode() >= 400 && nm.responseCode() <= 499)) {
        WriteLog("error", "Response code " + nm.responseCode() + "\r\n" + nm.errorString() );
        return 0;
    }
    return 1;
}

function _LoadAlbumList(list) {
    local i = 0;
    while ( 1 ) {
        i++;
        nm.addQueryHeader("Expect","");
        nm.addQueryHeader("Authorization", _GetAuthorizationString());
        nm.doGet("https://photoslibrary.googleapis.com/v1/albums");
        local response = _CheckResponse();
        if ( response == 0 ) {
            return 0;
        } else if ( response == 1 ) {
            break;
        } else if (  i > 3 ) {
            return 0;
        }
    }
    
    return _ParseAlbumList(nm.responseBody(),list);	
}

function GetFolderList(list) {
    return _LoadAlbumList(list);
}

function CreateFolder(parentAlbum,album) {
    nm.addQueryHeader("Expect", "");
    nm.setUrl("https://photoslibrary.googleapis.com/v1/albums");
    nm.addQueryHeader("Authorization", _GetAuthorizationString());
    
    local data = {
        album = {
            title = album.getTitle()
        }
    };
    nm.addQueryHeader("Content-Type","application/json");
    nm.doPost(ToJSON(data));
    if ( nm.responseCode() == 200) {
        local t = ParseJSON(nm.responseBody())
        album.setId(t.id);
        //album.setViewUrl(t.productUrl);
    }
    
    return 1;
}

function UploadFile(FileName, options) {    
    local i = 0;
    while ( i < 3 ) {
        i++;
        local albumStr = options.getFolderID();
        if (albumStr == "") {
            WriteLog("error", "[Google Photos] You should select an album before upload");
            return -1;
        }
        nm.setUrl("https://photoslibrary.googleapis.com/v1/uploads");
        nm.addQueryHeader("Authorization", _GetAuthorizationString());

        local ServerFileName = options.getServerFileName();
        if(ServerFileName=="") ServerFileName = ExtractFileName(FileName);
        local encodedFname = /*nm.urlEncode*/StrReplace(ServerFileName, " ", "_");
        nm.addQueryHeader("X-Goog-Upload-File-Name", encodedFname);
        nm.addQueryHeader("X-Goog-Upload-Protocol", "raw");
        nm.addQueryHeader("Expect","");
        nm.addQueryHeader("Content-Type", "application/octet-stream");
        nm.doUpload(FileName, "");
        
        local responseInfo = _CheckResponse ();
        if ( responseInfo == 0 ) {
            return 0;
        } else if ( responseInfo == 2 ) {
            continue;
        }

        local uploadToken = nm.responseBody();
        
        nm.setUrl("https://photoslibrary.googleapis.com/v1/mediaItems:batchCreate");
        nm.addQueryHeader("Authorization", _GetAuthorizationString());
        nm.addQueryHeader("Content-Type", "application/json");
        local requestData = {
                newMediaItems = [
                    {
                        description = "",
                        simpleMediaItem = {
                            uploadToken = uploadToken
                        }
                    }
                ]
        };
        if (albumStr != "") {
            requestData.albumId <- albumStr;
        }
        nm.doPost(ToJSON(requestData));
        if (nm.responseCode() == 200) {
            local t = ParseJSON(nm.responseBody());
            if( "newMediaItemResults" in t && t.newMediaItemResults.len() > 0) {
                local item = t.newMediaItemResults[0];
                if (albumStr != "") {
                    local shareUrl = Sync.getValue("shareUrl");
                    if (shareUrl=="") {
                        nm.addQueryHeader("Authorization", _GetAuthorizationString());
                        nm.doGet("https://photoslibrary.googleapis.com/v1/albums/" + albumStr);
                        if (nm.responseCode() == 200) {
                            local album = ParseJSON(nm.responseBody());
                            if ("shareInfo" in album) {
                                shareUrl = album.shareInfo.shareableUrl;
                            }
                        }
                        
                        if (shareUrl == "") {
                            nm.setUrl("https://photoslibrary.googleapis.com/v1/albums/" + albumStr + ":share");
                            nm.addQueryHeader("Content-Type", "application/json");
                            local postData = {
                                sharedAlbumOptions = { 
                                }
                            };
                            nm.addQueryHeader("Authorization", _GetAuthorizationString());
                            nm.doPost(ToJSON(postData));
                            if (nm.responseCode() == 200) {
                                local parsedData = ParseJSON(nm.responseBody());
                                //options.setDirectUrl(directUrl);
                                //options.setThumbUrl(thumbUrl);
                                shareUrl = parsedData.shareInfo.shareableUrl;
                            }
                        }
                        if ( shareUrl != "" ) {
                            Sync.setValue("shareUrl", shareUrl);
                        }
                    }
                    options.setViewUrl(shareUrl);
                }
                return 1;
            }  
        } else if (nm.responseCode() == 400) {
            local e = ParseJSON(nm.responseBody());
            if ( "error" in e && "message" in e.error) {
                WriteLog("error", "gphotos: " + e.error.message+"\r\nYou can add photos only to an album which was created by Image Uploader.");
            }
        }
    }
    
    return 0;
}

function ModifyFolder(album) {
    return 0;
}

function GetFolderAccessTypeList() {
    return ["Private", "Public"];
}

function GetServerParamList() {
    return {
        token = "token",
        tokenType = "tokenType",
        expiresIn = "expiresIn",
        refreshToken = "refreshToken",
        tokenTime = "tokenTime"
    }
}