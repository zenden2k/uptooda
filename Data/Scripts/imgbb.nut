auth_token <- "";

function _ObtainToken() {
    if (auth_token == "") {
        nm.doGet("https://imgbb.com/");
        if (nm.responseCode() != 200) {
            return "";
        }
        local reg = CRegExp("auth_token=\"(.+?)\"", "");
        if ( reg.match(nm.responseBody()) ) {
            auth_token = reg.getMatch(1);
        }
    }
    return auth_token;
}

function _UploadToAccount(FileName, options) {
    nm.enableResponseCodeChecking(true);
    local apiKey = ServerParams.getParam("Password");
    if (apiKey == "") {
        WriteLog("error", "[imgbb.com] Cannot upload to account without API key");
        return ResultCode.Failure;
    }
    local expiration = 0;
    try {
        expiration = 60 * ServerParams.getParam("expirationAuthorized").tointeger();
    } catch (ex) {

    }
    nm.addQueryParam("key", apiKey);
    if (expiration) {
        nm.addQueryParam("expiration", expiration);
    }
    nm.addQueryParamFile("image", FileName, ExtractFileName(FileName), "");
    nm.setUrl("https://api.imgbb.com/1/upload?expiration=" + expiration);
    nm.doUploadMultipartData();
    local t = ParseJSON(nm.responseBody());
    if (nm.responseCode() == 200) { 
        if (t.success) {
            options.setViewUrl(t.data.url_viewer);
            options.setDirectUrl(t.data.url);
            options.setThumbUrl(t.data.thumb.url);
            if ("delete_url" in t.data) {
                options.setDeleteUrl(t.data.delete_url);
            }
            return ResultCode.Success;
        }
    } else if ("error" in t) {
        WriteLog("error", "[imgbb.com] got error from server: \nResponse code:" + nm.responseCode() + "\n" + (("error" in t) ? t.error.message: ""));
        if (nm.responseCode() == 400) {
            return ResultCode.FatalError;
        }
    }
    return ResultCode.Failure;
}

function UploadFile(FileName, options) {
    nm.enableResponseCodeChecking(false);
    local login = ServerParams.getParam("Login");

    if (login != "") {
        return _UploadToAccount(FileName, options);
    }

    local name = ExtractFileName(FileName);
    local mime = GetFileMimeType(name);
    local token = _ObtainToken();
    local expiration = ServerParams.getParam("expiration");
    if (token == "") {
        WriteLog("error", "[imgbb.com] Unable to obtain auth token");
        
        return ResultCode.Failure;
    }
    nm.setUrl("https://imgbb.com/json");
    nm.addQueryParam("type", "file");
    nm.addQueryParam("action", "upload");
    nm.addQueryParam("privacy", "public");
    nm.addQueryParam("timestamp", time() + "000");
    nm.addQueryParam("auth_token", token);
    nm.addQueryParam("category_id", "");
    if (expiration != "") {
        nm.addQueryParam("expiration", expiration);
    }

    nm.addQueryParam("nswd", "");
    nm.addQueryParamFile("source", FileName, name, mime);
    nm.doUploadMultipartData();
    if (nm.responseCode() == 200) {
        local sJSON = nm.responseBody();
        local t = ParseJSON(sJSON);
        if (t != null) {
            options.setViewUrl(t.image.url_viewer);
            options.setDirectUrl(t.image.url);
            options.setThumbUrl(t.image.thumb.url);
            if ("delete_url" in t.image) {
                options.setDeleteUrl(t.image.delete_url);
            }
            return ResultCode.Success;
        } else {
            return ResultCode.Failure;
        }
    } else {
        local t = ParseJSON(nm.responseBody());
        if (t != null) {
            WriteLog("error", "[imgbb.com] got error from server: \nResponse code:" + nm.responseCode() + "\n" + (("error" in t) ? t.error.message: ""));
        }
        if (nm.responseCode() == 400) {
            return ResultCode.FatalError;
        }
        return ResultCode.Failure;
    }
}

function GetServerParamList() {
    return {
        expirationAuthorized = "Expiration in minutes (authorized)",
        expiration = "Expiration (anonymous)"
    };
}