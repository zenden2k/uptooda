function UploadFile(FileName, options) {
    local name = ExtractFileName(FileName);
    local mime = GetFileMimeType(name);
    local thumbWidth = options.getParam("THUMBWIDTH");

    nm.setUrl("https://api.pixhost.to/images");
    nm.addPostFieldFile("img", FileName, name, mime);
    nm.addPostField("content_type", "0");
    nm.addPostField("max_th_size", thumbWidth);

    nm.doUploadMultipartData();

    if (nm.responseCode() == 200) {
        local t = ParseJSON(nm.responseBody());
        if (t != null) {
            options.setViewUrl(t.show_url);
            options.setThumbUrl(t.th_url);
            return 1;
        }
    } else {
        WriteLog("error", "[pixhost.to] Response code " + nm.responseCode() + "\r\n" + nm.errorString());  
    }

    return 0;
}