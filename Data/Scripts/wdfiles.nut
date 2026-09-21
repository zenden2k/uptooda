function UploadFile(FileName, options) {
    nm.doGet("https://wdfiles.ru");

    if (nm.responseCode() != 200) {
        return 0;
    }

    local reg = CRegExp("'(https://wdfiles.ru/core/page/ajax/file_upload_handler.ajax.php.+?)'", "mi");
    local uploadUrl = "";
    if ( reg.match(nm.responseBody()) ) {
        uploadUrl = reg.getMatch(1); 
    }

    local reg2 = CRegExp("_sessionid: '(.+?)'", "mi");
    local sessionId = "";
    if ( reg2.match(nm.responseBody()) ) {
        sessionId = reg2.getMatch(1); 
    }

    local reg3 = CRegExp("cTracker: '(.+?)'", "mi")
    local cTracker = "";
    if ( reg3.match(nm.responseBody()) ) {
        cTracker = reg3.getMatch(1); 
    }

    if (!uploadUrl || !sessionId || !cTracker) {
        WriteLog("error", "[wdfiles.ru] Cannot find the information I need in the server response");
        return 0;
    }

    local name = ExtractFileName(FileName);
    local mime = GetFileMimeType(FileName);

    nm.setUrl(uploadUrl);
    nm.addQueryHeader("X-Requested-With", "XMLHttpRequest");
    nm.addQueryHeader("Accept-Language", "ru,ru-RU;q=0.9,en-US;q=0.8,en;q=0.7");
    nm.addPostField("_sessionid", sessionId);
    nm.addPostField("cTracker", cTracker);
    nm.addPostField("maxChunkSize", "100000000");
    nm.addPostField("folderId", "");
    
    nm.addPostFieldFile("files[]", FileName, name, mime);
    nm.doUploadMultipartData();

    if (nm.responseCode() != 200) {
        WriteLog("error", "[wdfiles.ru] Failed to upload file");
        return 0;
    }

    local t = ParseJSON(nm.responseBody());

    if ("url" in t[0]) {
        options.setViewUrl(t[0].url);
        options.setDeleteUrl(t[0].delete_url);
        return 1;
    } else if ("error" in t[0] && t[0].error != null) {
        WriteLog("error", "[wdfiles.ru] Failed to upload file: " + t[0].error);
    }

    return 0;
}