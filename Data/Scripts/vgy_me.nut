function UploadFile(FileName, options) {
    local apiKey = ServerParams.getParam("Password");
    if (apiKey == "") {
        WriteLog("error", "[vgy.me] Cannot proceed without API key.");
        return -1;
    }	
    nm.setUrl("https://vgy.me/upload");
    nm.addPostFieldFile("file", FileName, ExtractFileName(FileName), "");
    nm.addPostField("userkey", apiKey);
    nm.doUploadMultipartData();
    
    if (nm.responseCode() == 200) {
        local t = ParseJSON(nm.responseBody());
        options.setDirectUrl(t.image);
        options.setViewUrl(t.url);
        options.setDeleteUrl(t.rawget("delete"));
        if (t.image != "" || t.url != ""){
            return 1;
        }
    }

    return 0;
}