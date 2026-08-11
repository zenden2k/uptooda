function UploadFile(FileName, options) {
    local name = ExtractFileName(FileName);
    local mime = GetFileMimeType(FileName);

    nm.setUrl("https://abcvg.org/server/php/");
    nm.addPostFieldFile("files[]", FileName, name, mime);
    nm.doUploadMultipartData();
    if (nm.responseCode() == 200) {
        local sJSON = nm.responseBody();
        local t = ParseJSON(sJSON);
        if (t != null) {
            if ("files" in t && t.files.len()) {
                local f = t.files[0];
                options.setDirectUrl(f.url);
                return 1;
            } else {
                WriteLog("error", "abcvg.org: Invalid server response");
            }
        } else {
            WriteLog("error", "abcvg.org: Failed to parse server's answer as JSON.");
        }

    } else {
        WriteLog("error", "abcvg.org: Failed to upload. Server status code: " + nm.responseCode());
    }
    return 0;
}