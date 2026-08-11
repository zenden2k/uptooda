const CURLOPT_FOLLOWLOCATION = 52;

function Authenticate()
{
    local login = ServerParams.getParam("Login");
    local pass = ServerParams.getParam("Password");

    nm.addQueryHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7");
    nm.addQueryHeader("Accept-Language", "ru,en;q=0.9,en-GB;q=0.8,en-US;q=0.7");
    nm.addQueryHeader("Cache-Control", "no-cache");
    nm.addQueryHeader("Connection", "keep-alive");
    nm.addQueryHeader("Pragma", "no-cache");
    nm.addQueryHeader("Referer", "https://imageban.ru/");
    nm.addQueryHeader("Sec-Fetch-Dest", "document");
    nm.addQueryHeader("Sec-Fetch-Mode", "navigate");
    nm.addQueryHeader("Sec-Fetch-Site", "same-origin");
    nm.addQueryHeader("Sec-Fetch-User", "?1");
    nm.addQueryHeader("Upgrade-Insecure-Requests", "1");
    nm.addQueryHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/147.0.0.0 Safari/537.36 Edg/147.0.0.0");
    nm.addQueryHeader("sec-ch-ua", "\"Microsoft Edge\";v=\"147\", \"Not.A/Brand\";v=\"8\", \"Chromium\";v=\"147\"");
    nm.addQueryHeader("sec-ch-ua-mobile", "?0");
    nm.addQueryHeader("sec-ch-ua-platform", "\"Windows\"");
    nm.doGet("https://imageban.ru/u/login");

    if (nm.responseCode() != 200) {
        WriteLog("error", "[imageban.ru] Failed to obtain CSRF token. Could not load the main page. Response code: " + nm.responseCode());
        return ResultCode.Failure;
    }

    local doc = Document(nm.responseBody());
    local inputElement = doc.find("input[name=\"csrf_token\"]");
    if (!inputElement.length()) {
        WriteLog("error", "[imageban.ru] Failed to obtain CSRF token.");
        return ResultCode.Failure;
    }
    local csrfToken = inputElement.attribute("value");

    if (csrfToken == "") {
        WriteLog("error", "[imageban.ru] Failed to obtain CSRF token.");
        return ResultCode.Failure;
    }
    nm.setUrl("https://imageban.ru/backend/users/login");
    nm.setReferer("https://imageban.ru/u/login");
    nm.addQueryHeader("Accept", "*/*");
    nm.addQueryHeader("Accept-Language", "ru,en;q=0.9,en-GB;q=0.8,en-US;q=0.7");
    nm.addQueryHeader("Cache-Control", "no-cache");
    nm.addQueryHeader("Connection", "keep-alive");
    nm.addQueryHeader("Origin", "https://imageban.ru");
    nm.addQueryHeader("Pragma", "no-cache");
    nm.addQueryHeader("Sec-Fetch-Dest", "empty");
    nm.addQueryHeader("Sec-Fetch-Mode", "cors");
    nm.addQueryHeader("Sec-Fetch-Site", "same-origin");
    nm.addQueryHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/147.0.0.0 Safari/537.36 Edg/147.0.0.0");
    nm.addQueryHeader("X-CSRF-Token", csrfToken);
    nm.addQueryHeader("X-Requested-With", "XMLHttpRequest");
    nm.addQueryHeader("sec-ch-ua", "\"Microsoft Edge\";v=\"147\", \"Not.A/Brand\";v=\"8\", \"Chromium\";v=\"147\"");
    nm.addQueryHeader("sec-ch-ua-mobile", "?0");
    nm.addQueryHeader("sec-ch-ua-platform", "\"Windows\"");
    nm.addPostField("csrf_token", csrfToken);
    nm.addPostField("login", login);
    nm.addPostField("pass", pass);
    nm.setCurlOptionInt(CURLOPT_FOLLOWLOCATION, 0);

    if (nm.doUploadMultipartData()) {
        local result = ParseJSON(nm.responseBody());
        if (result == null) {
            WriteLog("error", "[imageban.ru] Failed to authenticate. Response code: " + nm.responseCode());
            return ResultCode.Failure;
        }

        if (!result.success) {
            WriteLog("error", "[imageban.ru] Failed to authenticate: " + result.message + "\nResponse code: " + nm.responseCode());
            return ResultCode.Failure;
        }

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
    nm.setReferer("https://imageban.ru/");
    nm.addQueryHeader("Accept", "application/json, text/javascript, */*; q=0.01");
    nm.addQueryHeader("Accept-Language", "ru,en;q=0.9,en-GB;q=0.8,en-US;q=0.7");
    nm.addQueryHeader("Cache-Control", "no-cache");
    nm.addQueryHeader("Connection", "keep-alive");
    nm.addQueryHeader("Origin", "https://imageban.ru");
    nm.addQueryHeader("Pragma", "no-cache");
    nm.addQueryHeader("Sec-Fetch-Dest", "empty");
    nm.addQueryHeader("Sec-Fetch-Mode", "cors");
    nm.addQueryHeader("Sec-Fetch-Site", "same-origin");
    nm.addQueryHeader("User-Agent", "Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/147.0.0.0 Safari/537.36 Edg/147.0.0.0");
    nm.addQueryHeader("X-Requested-With", "XMLHttpRequest");
    nm.addQueryHeader("sec-ch-ua", "\"Microsoft Edge\";v=\"147\", \"Not.A/Brand\";v=\"8\", \"Chromium\";v=\"147\"");
    nm.addQueryHeader("sec-ch-ua-mobile", "?0");
    nm.addQueryHeader("sec-ch-ua-platform", "\"Windows\"");
    nm.addPostField("compmenu", "0");
    nm.addPostField("albmenu", "0");
    nm.addPostField("inf", thumbUseServerText ? "1" : "0");
    nm.addPostField("cat", "0");
    nm.addPostField("prew", options.getParam("THUMBWIDTH"));
    nm.addPostField("ttl", "0");
    nm.addPostField("ptext", "Увеличить");
    nm.addPostField("itext", "");
    nm.addPostField("grad", "0");
    nm.addPostField("rsize", "1");
    nm.addPostFieldFile("Filedata", filePath, displayName, GetFileMimeType(filePath));
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
