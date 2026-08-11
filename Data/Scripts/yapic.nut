const BASE_HOST = "https://yapic.ru";

function UploadFile(fileName, options) {
    nm.doGet(BASE_HOST + "/");
    if (nm.responseCode() != 200) {
        WriteLog("error", "[yapic.ru] Cannot obtain token!");
        return 0;
    }
    local timestamp = 0, token = "";
    local reg = CRegExp("token'\\s*:\\s*'(.+?)'", "mi"); 
    if (reg.match(nm.responseBody()) ) {
        token = reg.getMatch(1);
    }
    local reg2 = CRegExp("timestamp'\\s*:\\s*'(.+?)'", "mi"); 
    if ( reg2.match(nm.responseBody()) ) {
        timestamp = reg2.getMatch(1);
    }
    nm.setUrl(BASE_HOST + "/includes/upload.php");
    nm.addPostFieldFile("Filedata", fileName, ExtractFileName(fileName), GetFileMimeType(fileName));
    nm.addPostField("timestamp", timestamp);
    nm.addPostField("token", token);
    nm.doUploadMultipartData();
    
    if (nm.responseCode() == 200) {
        local t = ParseJSON(nm.responseBody());
        if ("success" in t && t.success) {
            options.setDirectUrl(t.preview);
            options.setViewUrl(t.url);
            if (t.url != ""){
                return 1;
            }
        }
    } else {
        WriteLog("error", "[yapic.ru] Failed to upload, response code = " + nm.responseCode());
    }

    return 0;
}