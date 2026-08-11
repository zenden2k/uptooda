// Chevereto uploader
function UploadFile(FileName, options) {
    local name = ExtractFileName(FileName);
    local mime = GetFileMimeType(name);
    nm.setUrl("https://freeimage.host/api/1/upload");
    nm.addPostField("key", "6d207e02198a847aa98d0a2a901485a5");
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
                    WriteLog("error", "[freeimage.host] " + t.error.message);
                }
            }
        }
    }

    return 0;
}