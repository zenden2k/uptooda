const CURLOPT_FOLLOWLOCATION = 52;

function Authenticate()
{
    local login = ServerParams.getParam("Login");
    local pass = ServerParams.getParam("Password");
    nm.setUrl("https://imageban.ru/u/login");
    nm.setReferer("https://imageban.ru/u/login");
    nm.addPostField("login", login);
    nm.addPostField("pass", pass);
    nm.addPostField("ok", "Вход");
    nm.setCurlOptionInt(CURLOPT_FOLLOWLOCATION, 0);
    if (nm.doPost("") && nm.responseCode() == 302) {
        return ResultCode.Success;
    }

    WriteLog("error", "[imageban.ru] Failed to authenticate. Response code: " + nm.responseCode());
    return ResultCode.Failure;
}

function UploadFile(filePath, options){    
    local login = ServerParams.getParam("Login");
    local pass = ServerParams.getParam("Password");
    if (login == "" || pass == "") {
        WriteLog("error", "imageban.ru: Login or Password cannot be empty.\r\nYou must set Login and Password in server settings.");
        return ResultCode.Failure;
    }

    local task = options.getTask().getFileTask();
    local displayName = task.getDisplayName();
    local thumbUseServerText = (options.getParam("THUMBCREATE") == "1" && options.getParam("THUMBADDTEXT") == "1" && options.getParam("THUMBUSESERVER") == "1");

    nm.setUrl("https://imageban.ru/up");
    nm.addQueryHeader("User-Agent", "Shockwave Flash");
    nm.addQueryParam("compmenu", "0");
    nm.addQueryParam("albmenu", "0");
    nm.addQueryParam("inf", thumbUseServerText ? "1" : "0");
    nm.addQueryParam("cat", "0");
    nm.addQueryParam("prew", options.getParam("THUMBWIDTH"));
    nm.addQueryParam("ttl", "0");
    nm.addQueryParam("ptext", "Увеличить");
    nm.addQueryParam("itext", "");
    nm.addQueryParam("grad", "0");
    nm.addQueryParam("rsize", "1");
    nm.addQueryParamFile("Filedata", filePath, displayName, GetFileMimeType(filePath));
    //nm.addQueryHeader("Cookie", "login="+login+"; pass="+md5(pass));
    nm.doUploadMultipartData();

    if (nm.responseCode() == 200) {
        local t = ParseJSON(nm.responseBody());
        if ("files" in t && t.files.len()) {
            local file = t.files[0];
            if ("error" in file) {
                WriteLog("error", "imageban.ru: " + file.error);
                return ResultCode.Failure;;
            }
            if (!("link" in file) || file.link == "") {
                WriteLog("error", "imageban.ru: Getting link failed");
                return ResultCode.Failure;;
            }
            options.setDirectUrl(file.link);
            
            if ("thumbs" in file) {
                options.setThumbUrl(file.thumbs);
            }
            if ("piclink" in file) {
                options.setViewUrl(file.piclink);
            }
            if ("delete" in file) {
                options.setDeleteUrl(file.rawget("delete"));
            }
            return ResultCode.Success;
        } else {
            WriteLog("error", "imageban.ru: Unknown error");
        }
    } else {
        WriteLog("error", "imageban.ru: Upload failed. Response code: " + nm.responseCode());
    }
    return ResultCode.Failure;
}
