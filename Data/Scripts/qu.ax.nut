function UploadFile(FileName, options) {
    local name = ExtractFileName(FileName);
    local mime = GetFileMimeType(name);
    nm.setUrl("https://qu.ax/upload.php");
    nm.addPostFieldFile("files[]", FileName, name, mime);
    nm.doUploadMultipartData();

    if (nm.responseCode() == 200) {
        local sJSON = nm.responseBody();
        local t = ParseJSON(sJSON);
        if (t != null) {
            if ("success" in t && t.success == true) {
                options.setDirectUrl(t.files[0].url);
                return 1;
            } 
        }

        WriteLog("error", "[qu.ax] Cannot obtain file URL from server's response.");
    } else {
        WriteLog("error", "[qu.ax] Upload failed. Response code: " + nm.responseCode());
    }

    return 0;
}