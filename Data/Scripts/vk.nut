clientId <- "4851603";
redirectUri <- "https://oauth.vk.ru/blank.html";
redirectUrlEscaped <- "https:\\/\\/oauth\\.vk\\.ru\\/blank\\.html";
apiVersion <- "5.131";
expiresIn <- 0;
testMode <- "1"; // not used

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

        local regError = CRegExp("error=([^&]+)", "");
        if ( regError.match(data.url) ) {
            WriteLog("warning", regError.getMatch(1));
        } else {
            local regToken = CRegExp("access_token=([^&]+)", "");
            local token = "";
            if ( regToken.match(data.url) ) {
                token = regToken.getMatch(1);
            }
            
            local regExpires = CRegExp("expires_in=([^&]+)", "");
            if ( regExpires.match(data.url) ) {
                expiresIn = regExpires.getMatch(1);
            }

            local regUserId = CRegExp("user_id=([^&]+)", "");
            local userId = "";
            if ( regUserId.match(data.url) ) {
                userId = regUserId.getMatch(1);
            }

            ServerParams.setParam("prevLogin", ServerParams.getParam("Login"));
            ServerParams.setParam("token", token);
            ServerParams.setParam("userId", userId);
            ServerParams.setParam("expiresIn", expiresIn.tostring());
            ServerParams.setParam("tokenTime", time().tostring());
        }
        br.close();
    }
}

function OnNavigateError(data) {
}

function checkResponse(json) {
    try {
        WriteLog("error", "vk.ru error: " + json.error.error_msg);
        return 0;
    } catch ( ex ) {

    }
    return 1;
}

function Authenticate() {
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

        if (time() + 10 < tokenTime + expiresIn) { 
            return 1;
        }
    }

    ServerParams.setParam("token", "");
    ServerParams.setParam("userId", "");
    ServerParams.setParam("expiresIn", "");
    ServerParams.setParam("tokenTime", "");
    local browser = CWebBrowser();
    browser.setTitle(tr("vk.browser.title", "vk.ru authorization"))
    browser.setOnUrlChangedCallback(OnUrlChangedCallback, null);
    //browser.setOnNavigateErrorCallback(OnNavigateError, null);
    //browser.setOnLoadFinishedCallback(OnLoadFinished, null);

    local url = "https://oauth.vk.ru/authorize?" +
            "client_id=" + clientId  +
            "&scope=photos" +
            "&redirect_uri=" + nm.urlEncode(redirectUri) +
            "&display=popup" +
            "&v=" + apiVersion  +
            "&response_type=token";

    browser.navigateToUrl(url);
    browser.showModal();
    return ServerParams.getParam("token") != "" ? 1: 0;
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
            return 0;
        }
        return 1;
    }
    return 0;
}

function DoLogout() {
    ServerParams.setParam("token", "");
    ServerParams.setParam("userId", "");
    ServerParams.setParam("tokenTime", "");
    ServerParams.setParam("expiresIn", "");
    return 1;
}

function GetFolderList(list) {
    local userId = ServerParams.getParam("userId");
    local token = ServerParams.getParam("token");
    nm.doGet("https://api.vk.ru/method/photos.getAlbums?owner_id=" + userId +"&v=" + apiVersion + "&access_token=" + token);
    if (nm.responseCode() != 200) {
        return 0;
    }
    local t = ParseJSON(nm.responseBody());
    if (!checkResponse(t)) {
        return 0;
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
    return 1;
}

function GetFirstAlbumId() {
    local userId = ServerParams.getParam("userId");
    local token = ServerParams.getParam("token");
    nm.doGet("https://api.vk.ru/method/photos.getAlbums?owner_id=" + userId +"&v=" + apiVersion + "&access_token=" + token);
    if ( nm.responseCode() != 200 ) {
        return 0;
    }
    local t = ParseJSON(nm.responseBody());
    if (!checkResponse(t)) {
        return 0;
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
        return 0;
    }

    local t = ParseJSON( nm.responseBody());
    if ( !checkResponse(t) ) {
        return 0;
    }
    album.setId(t.response.id.tostring());
    album.setTitle(t.response.title);
    album.setSummary(t.response.description);
    album.setAccessType(StringPrivacyToAccessType(t.response.privacy_view.category));
    album.setTitle(t.response.title);
    album.setViewUrl("https://vk.ru/album" + userId + "_" + t.response.id);

    return 1;
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
            return 0;
        }
        return 1; // OK
    }
    return 0; // failure
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
                return 0;
            }
            albumId = newAlbum.getId();
        }
    }
    local thumbWidth = options.getParam("THUMBWIDTH");
    thumbWidth = thumbWidth.tointeger();
    nm.doGet("https://api.vk.ru/method/photos.getUploadServer?user_id=" + userId +"&v=" + apiVersion + "&access_token=" + token+"&album_id="+albumId);
    if ( nm.responseCode() != 200 ) {
        return 0;
    }
    local t = ParseJSON( nm.responseBody());
    if ( !checkResponse(t) ) {
        return 0;
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
                return 0;
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

            return 1;
        }
    }
    return 0;
}

function GetFolderAccessTypeList() {
    return [
        tr("vk.privacy.all_users", "All users"),
        tr("vk.privacy.friends_only", "Friends only"),
        tr("vk.privacy.friends_and_friends_of_friends", "Friends and friends of friends"),
        tr("vk.privacy.just_me", "Just me" )
    ];
}