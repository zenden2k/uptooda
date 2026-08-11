// Chevereto uploader
function UploadFile(FileName, options) {
    local name = ExtractFileName(FileName);
    local mime = GetFileMimeType(name);
    nm.setUrl("https://2i.cz/api/1/upload");
    nm.addPostField("key", "1cc098eb545eea3d78ea321a531e7eb0");
    nm.addPostField("format", "json");
    nm.addPostFieldFile("source", FileName, name, mime);
    nm.doUploadMultipartData();

    if (nm.responseCode() > 0) {
        local sJSON = nm.responseBody();
        local t = ParseJSON(sJSON);
        if (t != null) {
            if ("status_code" in t && t.status_code == 200) {
                options.setViewUrl(t.image.url_viewer);
                options.setThumbUrl(t.image.thumb.url);
                options.setDirectUrl(t.image.url);
                return 1;
            } else {
                if ("error" in t && t.error != null) {
                    WriteLog("error", "2i.cz: " + t.error.message);
                }
            }
        }
    }

    return 0;
}