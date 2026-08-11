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
    nm.addPostField("key", apiKey);
    if (expiration) {
        nm.addPostField("expiration", expiration);
    }
    nm.addPostFieldFile("image", FileName, ExtractFileName(FileName), "");
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
    nm.addPostField("type", "file");
    nm.addPostField("action", "upload");
    nm.addPostField("privacy", "public");
    nm.addPostField("timestamp", time() + "000");
    nm.addPostField("auth_token", token);
    nm.addPostField("category_id", "");
    if (expiration != "") {
        nm.addPostField("expiration", expiration);
    }

    nm.addPostField("nswd", "");
    nm.addPostFieldFile("source", FileName, name, mime);
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
        expirationAuthorized = tr("imgbb.expiration_authorized", "Expiration in minutes (authorized)"),
        expiration = {
            title = tr("imgbb.expiration", "Expiration (anonymous)"),
            type = "choice",
            items = [
                {
                    id = "",
                    label = tr("imgbb.never", "Never")
                },
                {
                    id = "PT5M",
                    label = "5 minutes"
                },
                {
                    id = "PT15M",
                    label = "15 minutes"
                },
                {
                    id = "PT30M",
                    label = "30 minutes"
                },
                {
                    id = "PT1H",
                    label = "1 hour"
                },
                {
                    id = "PT3H",
                    label = "3 hours"
                },
                {
                    id = "PT6H",
                    label = "6 hours"
                },
                {
                    id = "PT12H",
                    label = "12 hours"
                },
                {
                    id = "P1D",
                    label = "1 day"
                },
                {
                    id = "P2D",
                    label = "2 days"
                },
                {
                    id = "P3D",
                    label = "3 days"
                },
                {
                    id = "P4D",
                    label = "4 days"
                },
                {
                    id = "P5D",
                    label = "5 days"
                },
                {
                    id = "P6D",
                    label = "6 days"
                },
                {
                    id = "P1W",
                    label = "1 week"
                },
                {
                    id = "P2W",
                    label = "2 weeks"
                },
                {
                    id = "P3W",
                    label = "3 weeks"
                },
                {
                    id = "P1M",
                    label = "1 month"
                },
                {
                    id = "P2M",
                    label = "2 months"
                },
                {
                    id = "P3M",
                    label = "3 months"
                },
                {
                    id = "P4M",
                    label = "4 months"
                },
                {
                    id = "P5M",
                    label = "5 months"
                },
                {
                    id = "P6M",
                    label = "6 months"
                }
            ]
        }
    };
}