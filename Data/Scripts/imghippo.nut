function UploadFile(fileName, options) {
    local key = ServerParams.getParam("apiKey");
    if (key == "") {
        WriteLog("error", "[imghippo.com] API key not set!");
        return 0;
    }
    local name = ExtractFileName(fileName);
    local mime = GetFileMimeType(name);
    nm.setUrl("https://api.imghippo.com/v1/upload");
    nm.addPostField("api_key", key);
    nm.addPostField("format", "json");
    nm.addPostFieldFile("file", fileName, name, mime);
    nm.doUploadMultipartData();

    if (nm.responseCode() == 200) {
        local sJSON = nm.responseBody();
        local t = ParseJSON(sJSON);
        if (t != null) {
            if ("success" in t && t.success) {
                options.setViewUrl(t.data.view_url);
                options.setDirectUrl(t.data.url);
                return 1;
            } else {
                if ("message" in t && t.message != null) {
                    WriteLog("error", "[imghippo.com] " + t.message);
                }
            }
        }
    }

    return 0;
}

function GetServerParamList() {
	return {
		apiKey = "API key"
	}
}