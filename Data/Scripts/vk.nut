clientId <- "4851603";
redirectUri <- "https://oauth.vk.ru/blank.html";
redirectUrlEscaped <- "https:\\/\\/oauth\\.vk\\.ru\\/blank\\.html";
apiVersion <- "5.199";
expiresIn <- 0;
testMode <- "1"; // not used
authCode <- "";
authDeviceId <- "";
authState <- "";
codeVerifier <- "";

function _ClearAuthData() {
    ServerParams.setParam("token", "");
    ServerParams.setParam("refreshToken", "");
    ServerParams.setParam("userId", "");
    ServerParams.setParam("deviceId", "");
    ServerParams.setParam("expiresIn", "");
    ServerParams.setParam("tokenTime", "");
}

function _UrlParam(url, name) {
    local reg = CRegExp("[\\?&#]" + name + "=([^&#]+)", "");
    if ( reg.match(url) ) {
        return reg.getMatch(1);
    }
    return "";
}

function _HexDigit(ch) {
    if ( ch >= '0' && ch <= '9' ) {
        return ch - '0';
    } else if ( ch >= 'a' && ch <= 'f' ) {
        return ch - 'a' + 10;
    } else if ( ch >= 'A' && ch <= 'F' ) {
        return ch - 'A' + 10;
    }
    return 0;
}

function _HexToRaw(hex) {
    local res = "";
    for ( local i = 0; i + 1 < hex.len(); i += 2 ) {
        local b = _HexDigit(hex[i]) * 16 + _HexDigit(hex[i + 1]);
        res += format("%c", b);
    }
    return res;
}

function _Base64UrlEncode(data) {
    local b64 = Base64Encode(data);
    local res = "";
    for ( local i = 0; i < b64.len(); i++ ) {
        if ( b64[i] == '+' ) {
            res += "-";
        } else if ( b64[i] == '/' ) {
            res += "_";
        } else if ( b64[i] != '=' ) {
            res += b64.slice(i, i + 1);
        }
    }
    return res;
}

function _CodeChallenge(verifier) {
    return _Base64UrlEncode(_HexToRaw(Sha256(verifier)));
}

function _TokenStillValid() {
    local token = ServerParams.getParam("token");
    if ( token == "" ) {
        return false;
    }

    local tokenTime  = 0;
    local expiresIn = 0;
    try {
        tokenTime = ServerParams.getParam("tokenTime").tointeger();
        expiresIn = ServerParams.getParam("expiresIn").tointeger();
    } catch ( ex ) {
    }

    if ( expiresIn == 0 ) {
        return ResultCode.Success;
    }
    return time() + 10 < tokenTime + expiresIn;
}

function _SaveTokenResponse(t) {
    if ( !("access_token" in t) || t.access_token == "" ) {
        return ResultCode.Failure;
    }

    ServerParams.setParam("prevLogin", ServerParams.getParam("Login"));
    ServerParams.setParam("token", t.access_token);
    if ( "refresh_token" in t ) {
        ServerParams.setParam("refreshToken", t.refresh_token);
    }
    if ( "user_id" in t ) {
        ServerParams.setParam("userId", t.user_id.tostring());
    }
    if ( "expires_in" in t ) {
        ServerParams.setParam("expiresIn", t.expires_in.tostring());
    } else {
        ServerParams.setParam("expiresIn", "3600");
    }
    if ( authDeviceId != "" ) {
        ServerParams.setParam("deviceId", authDeviceId);
    }
    ServerParams.setParam("tokenTime", time().tostring());
    return ResultCode.Success;
}

function StringPrivacyToAccessType(s) {
    if ( s == "nobody" ) {
        return 3;
    } else if ( s == "friends" ) {
        return 1;
    } else if ( s == "friends_of_friends" ) {
        return 2;
    }
    return 0;
}

function AccessTypeToPrivacy(s) {
    if ( s == 3 ) {
        return "nobody";
    } else if ( s == 1 ) {
        return "friends";
    } else if ( s == 2 ) {
        return "friends_of_friends";
    }
    return "all";
}

function OnUrlChangedCallback(data) {
    local reg = CRegExp("^" +redirectUrlEscaped, "");
    if ( reg.match(data.url) ) {
        local br = data.browser;

        local error = _UrlParam(data.url, "error");
        if ( error != "" ) {
            WriteLog("warning", error);
        } else {
            authCode = _UrlParam(data.url, "code");
            authDeviceId = _UrlParam(data.url, "device_id");
            authState = _UrlParam(data.url, "state");
        }
        br.close();
    }
}

function OnNavigateError(data) {
}

function checkResponse(json) {
    try {
        WriteLog("error", "vk.ru error: " + json.error.error_msg);
        return ResultCode.Failure;
    } catch ( ex ) {

    }
    return ResultCode.Success;
}

function RefreshToken() {
    if ( _TokenStillValid() ) {
        return ResultCode.Success;
    }

    local refreshToken = ServerParams.getParam("refreshToken");
    if ( refreshToken == "" ) {
        return ResultCode.Failure;
    }

    nm.setUrl("https://id.vk.ru/oauth2/auth");
    nm.addQueryParam("grant_type", "refresh_token");
    nm.addQueryParam("refresh_token", refreshToken);
    nm.addQueryParam("client_id", clientId);

    local deviceId = ServerParams.getParam("deviceId");
    if ( deviceId != "" ) {
        nm.addQueryParam("device_id", deviceId);
    }

    nm.doPost("");
    if ( nm.responseCode() != 200 ) {
        WriteLog("error", "vk.ru: unable to refresh token, response code: " + nm.responseCode());
        _ClearAuthData();
        return ResultCode.Failure;
    }

    local t = ParseJSON(nm.responseBody());
    if ( t == null || "error" in t ) {
        WriteLog("error", "vk.ru: unable to refresh token. " + nm.responseBody());
        _ClearAuthData();
        return ResultCode.Failure;
    }

    return _SaveTokenResponse(t);
}

function Authenticate() {
    if ( RefreshToken() ) {
        return ResultCode.Success;
    }

    _ClearAuthData();
    authCode = "";
    authDeviceId = "";
    authState = "";
    local state = md5(RandomString(32) + time().tostring());
    codeVerifier = RandomString(64);

    local browser = CWebBrowser();
    browser.setTitle(tr("vk.browser.title", "vk.ru authorization"))
    browser.setOnUrlChangedCallback(OnUrlChangedCallback, null);
    //browser.setOnNavigateErrorCallback(OnNavigateError, null);
    //browser.setOnLoadFinishedCallback(OnLoadFinished, null);

    local url = "https://id.vk.ru/authorize?" +
            "client_id=" + clientId  +
            "&scope=photos" +
            "&redirect_uri=" + nm.urlEncode(redirectUri) +
            "&response_type=code" +
            "&state=" + state +
            "&code_challenge=" + nm.urlEncode(_CodeChallenge(codeVerifier)) +
            "&code_challenge_method=S256";

    browser.navigateToUrl(url);
    browser.showModal();

    if ( authCode == "" ) {
        WriteLog("error", "vk.ru: cannot authenticate without confirmation code.");
        return ResultCode.Failure;
    }
    if ( authState != state ) {
        WriteLog("error", "vk.ru: authorization state mismatch.");
        return ResultCode.Failure;
    }

    nm.setUrl("https://id.vk.ru/oauth2/auth");
    nm.addQueryParam("grant_type", "authorization_code");
    nm.addQueryParam("code", authCode);
    nm.addQueryParam("code_verifier", codeVerifier);
    nm.addQueryParam("client_id", clientId);
    nm.addQueryParam("redirect_uri", redirectUri);
    nm.addQueryParam("state", authState);
    if ( authDeviceId != "" ) {
        nm.addQueryParam("device_id", authDeviceId);
    }
    nm.doPost("");

    if ( nm.responseCode() != 200 ) {
        WriteLog("error", "vk.ru: unable to obtain bearer token, response code: " + nm.responseCode() + "\r\n" + nm.responseBody());
        return ResultCode.Failure;
    }

    local t = ParseJSON(nm.responseBody());
    if ( t == null || "error" in t ) {
        WriteLog("error", "vk.ru: authentication failed. " + nm.responseBody());
        return ResultCode.Failure;
    }

    return _SaveTokenResponse(t);
}

function IsAuthenticated() {
    local token = ServerParams.getParam("token");
    local userId = ServerParams.getParam("userId");

    if ( token != "") {
        local tokenTime  = 0;
        local expiresIn = 0;
        try {
            tokenTime = ServerParams.getParam("tokenTime").tointeger();
            expiresIn = ServerParams.getParam("expiresIn").tointeger();
        } catch ( ex ) {
        }

        if ( time() + 10 > tokenTime + expiresIn) {
            return ResultCode.Failure;
        }
        return ResultCode.Success;
    }
    return ResultCode.Failure;
}

function DoLogout() {
    _ClearAuthData();
    return ResultCode.Success;
}

function GetFolderList(list) {
    local userId = ServerParams.getParam("userId");
    local token = ServerParams.getParam("token");
    nm.doGet("https://api.vk.ru/method/photos.getAlbums?owner_id=" + userId +"&v=" + apiVersion + "&access_token=" + token);
    if (nm.responseCode() != 200) {
        return ResultCode.Failure;
    }
    local t = ParseJSON(nm.responseBody());
    if (!checkResponse(t)) {
        return ResultCode.Failure;
    }

    for (local i = 0; i < t.response.count; i++) {
        local item = t.response.items[i];
        local album = CFolderItem();
        album.setId(item.id.tostring());
        album.setTitle(item.title);
        album.setSummary(item.description );
        album.setAccessType(StringPrivacyToAccessType(item.privacy_view.category));
        album.setViewUrl("https://vk.ru/album" + userId + "_" + item.id);
        list.AddFolderItem(album);
    }
    return ResultCode.Success;
}

function GetFirstAlbumId() {
    local userId = ServerParams.getParam("userId");
    local token = ServerParams.getParam("token");
    nm.doGet("https://api.vk.ru/method/photos.getAlbums?owner_id=" + userId +"&v=" + apiVersion + "&access_token=" + token);
    if ( nm.responseCode() != 200 ) {
        return "";
    }
    local t = ParseJSON(nm.responseBody());
    if (!checkResponse(t)) {
        return "";
    }

    for (local i = 0; i < t.response.count; i++ ) {
        local item = t.response.items[i];
        if (item.title == "Image Uploader") {
            return item.id;
        }
    }
    return "";
}

function CreateFolder(parentAlbum, album) {
    local userId = ServerParams.getParam("userId");
    local token = ServerParams.getParam("token");
    local title = album.getTitle();
    local summary = album.getSummary();
    local accessType = album.getAccessType();

    nm.addQueryParam("title", title);
    nm.addQueryParam("description", summary);
    nm.addQueryParam("privacy_view", AccessTypeToPrivacy(accessType));
    nm.addQueryParam("privacy_comment", AccessTypeToPrivacy(accessType));

    nm.setUrl("https://api.vk.ru/method/photos.createAlbum?user_id=" + userId +"&v=" + apiVersion + "&access_token=" + token);

    nm.doPost("");
    if ( nm.responseCode() != 200 && nm.responseCode() != 201 ) {
        return ResultCode.Failure;
    }

    local t = ParseJSON( nm.responseBody());
    if ( !checkResponse(t) ) {
        return ResultCode.Failure;
    }
    album.setId(t.response.id.tostring());
    album.setTitle(t.response.title);
    album.setSummary(t.response.description);
    album.setAccessType(StringPrivacyToAccessType(t.response.privacy_view.category));
    album.setTitle(t.response.title);
    album.setViewUrl("https://vk.ru/album" + userId + "_" + t.response.id);

    return ResultCode.Success;
}

function ModifyFolder(album) {
    local userId = ServerParams.getParam("userId");
    local token = ServerParams.getParam("token");
    local title = album.getTitle();
    local id = album.getId();
    local summary = album.getSummary();
    local accessType = album.getAccessType();
    local parentId = album.getParentId;

    nm.addQueryParam("album_id", id);
    nm.addQueryParam("title", title);
    nm.addQueryParam("description", summary);
    nm.addQueryParam("owner_id", userId);
    nm.addQueryParam("privacy_view", AccessTypeToPrivacy(accessType));
    nm.addQueryParam("privacy_comment", AccessTypeToPrivacy(accessType));
    nm.setUrl("https://api.vk.ru/method/photos.editAlbum?user_id=" + userId +"&v=" + apiVersion + "&access_token=" + token);

    nm.doPost("");
    if ( nm.responseCode() == 200 ) {
        local t = ParseJSON( nm.responseBody());
        if ( !checkResponse(t) ) {
            return ResultCode.Failure;
        }
        return ResultCode.Success; // OK
    }
    return ResultCode.Failure; // failure
}

function UploadFile(FileName, options) {
    local token = ServerParams.getParam("token");
    local userId = ServerParams.getParam("userId");
    local albumId = options.getFolderID();
    if ( albumId == "" ) {
        albumId = GetFirstAlbumId().tostring();
        if ( albumId == "" ) {
            local newAlbum = CFolderItem();
            newAlbum.setTitle("Image Uploader");
            newAlbum.setAccessType(3);
            newAlbum.setSummary(tr("vk.default_album_desc", "Images uploaded by Image Uploader") +"\r\nhttps://svistunov.dev/imageuploader");

            if ( !CreateFolder(CFolderItem(), newAlbum) ) {
                return ResultCode.Failure;
            }
            albumId = newAlbum.getId();
        }
    }
    local thumbWidth = options.getParam("THUMBWIDTH");
    thumbWidth = thumbWidth.tointeger();
    nm.doGet("https://api.vk.ru/method/photos.getUploadServer?user_id=" + userId +"&v=" + apiVersion + "&access_token=" + token+"&album_id="+albumId);
    if ( nm.responseCode() != 200 ) {
        return ResultCode.Failure;
    }
    local t = ParseJSON( nm.responseBody());
    if ( !checkResponse(t) ) {
        return ResultCode.Failure;
    }

    local uploadUrl = t.response.upload_url;

    nm.addQueryParamFile("file1", FileName, ExtractFileName(FileName),GetFileMimeType(FileName));
    nm.setUrl(uploadUrl);
    nm.doUploadMultipartData();
    if ( nm.responseCode() >= 200 && nm.responseCode() <= 299 ) {
        local resp = nm.responseBody();
        local json = ParseJSON(resp);

        nm.addQueryParam("album_id", albumId);
        nm.addQueryParam("server", json.server.tostring());
        nm.addQueryParam("photos_list", json.photos_list);
        nm.addQueryParam("hash", json.hash);
        nm.addQueryParam("photo_sizes", "1");
        nm.addQueryParam("https", "1");

        nm.setUrl("https://api.vk.ru/method/photos.save?user_id=" + userId +"&v=" + apiVersion + "&access_token=" + token);

        nm.doPost("");
        if ( nm.responseCode() >= 200 && nm.responseCode() <= 299 ) {
            local t = ParseJSON(nm.responseBody());
            if ( !checkResponse(t) ) {
                return ResultCode.Failure;
            }

            local foundThumbDist = 99999;
            local foundSize = 0;
            local directUrl = "";
            local thumbUrl = "";

            local item = t.response[0];
            for ( local i = 0; i < item.sizes.len(); i++ ) {
                local s = item.sizes[i];
                if ( abs(s.width - thumbWidth) < foundThumbDist ) {
                    foundThumbDist = abs(s.width - thumbWidth);
                    thumbUrl = s.url;
                }
                if ( s.width > foundSize ) {
                    directUrl = s.url;
                    foundSize = s.width;
                }
            }

            options.setDirectUrl(directUrl);
            options.setThumbUrl(thumbUrl);
            options.setViewUrl("https://vk.ru/photo" + userId  + "_" + item.id);

            return ResultCode.Success;
        }
    }
    return ResultCode.Failure;
}

function GetFolderAccessTypeList() {
    return [
        tr("vk.privacy.all_users", "All users"),
        tr("vk.privacy.friends_only", "Friends only"),
        tr("vk.privacy.friends_and_friends_of_friends", "Friends and friends of friends"),
        tr("vk.privacy.just_me", "Just me" )
    ];
}
