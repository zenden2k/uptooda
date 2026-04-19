MyClientId <- "tB3J94mijYhW5Up5fm2c";

function  UploadFile(filePath, options)
{
    local clientId = ServerParams.getParam("ClientId");
    local secretKey = ServerParams.getParam("SecretKey");
    if (clientId == "") {
        clientId = MyClientId;
    }
    if (secretKey == "" ){
        WriteLog("error", "imageban.ru: SecretKey parameter cannot be empty. \r\nYou must set SecretKey in server settings.");
        return 0;
    }
    local task = options.getTask().getFileTask();
    local displayName = task.getDisplayName();
    
    nm.setUrl("https://api.imageban.ru/v1");
    nm.addQueryHeader("Authorization", "TOKEN " + clientId);
    nm.addQueryParamFile("image", filePath, displayName, GetFileMimeType(filePath));
    nm.addQueryParam("name", displayName);
    nm.addQueryParam("secret_key", secretKey);
    nm.doUploadMultipartData();

    if (nm.responseCode() == 200) {
        local data = nm.responseBody();
        local t = ParseJSON(data);
        if ("success" in t && t.success) {
            local viewUrl = t.data.short_link;
            local directUrl = t.data.link;
            local thumbUrl = StrReplace(directUrl, "/out/", "/thumbs/");
            options.setDirectUrl(directUrl);
            options.setViewUrl(viewUrl);
            options.setThumbUrl(thumbUrl);
            return 1; // Success
        } else {
            if ("error" in t) {
                WriteLog("error", "imageban.ru: " + t.error.message);
            } else {
                WriteLog("error", "imageban.ru: Unknown error");
            }
        }
    } else {
        WriteLog("error", "imageban.ru: Upload failed. Response code: " + nm.responseCode());
    }

    return 0;
}

function GetServerParamList()
{
    return {
        ClientId = "ClientId",
        SecretKey = "SecretKey"
    };
}