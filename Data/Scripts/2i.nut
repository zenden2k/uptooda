// Chevereto 4 uploader
const BASE_URL = "https://2i.cz";

function _ObtainAuthToken() {
    nm.doGet(BASE_URL + "/");
    if (nm.responseCode() != 200) {
        WriteLog("error", "[2i.cz] Unable to load the main page");
        return "";
    }

    local reg = CRegExp("PF\\.obj\\.config\\.auth_token\\s*=\\s*\"([^\"]+)\"", "");
    if (reg.match(nm.responseBody())) {
        return reg.getMatch(1);
    }

    WriteLog("error", "[2i.cz] Unable to obtain auth token");
    return "";
}

function UploadFile(FileName, options) {
    nm.enableResponseCodeChecking(false);
    nm.setUserAgent("Mozilla/5.0 (Windows NT 10.0; Win64; x64; rv:154.0) Gecko/20100101 Firefox/154.0");

    local token = _ObtainAuthToken();
    if (token == "") {
        return ResultCode.Failure;
    }

    local name = ExtractFileName(FileName);
    local mime = GetFileMimeType(name);
    local checksum = XXH64FromFile(FileName, 0, 0);
    if (checksum == "") {
        WriteLog("error", "[2i.cz] Unable to calculate file checksum");
        return ResultCode.Failure;
    }

    nm.setUrl(BASE_URL + "/json");
    nm.setReferer(BASE_URL + "/");
    nm.addQueryHeader("Accept", "application/json");
    nm.addQueryHeader("Origin", BASE_URL);
    nm.addPostFieldFile("source", FileName, name, mime);
    nm.addPostField("type", "file");
    nm.addPostField("action", "upload");
    nm.addPostField("timestamp", time() + "000");
    nm.addPostField("auth_token", token);
    nm.addPostField("expiration", "0");
    nm.addPostField("nsfw", "0");
    nm.addPostField("mimetype", mime);
    nm.addPostField("checksum", checksum);
    nm.doUploadMultipartData();

    local t = ParseJSON(nm.responseBody());
    if (nm.responseCode() == 200 && t != null && "status_code" in t && t.status_code == 200 && "image" in t) {
        options.setViewUrl(t.image.url_viewer);
        options.setThumbUrl(t.image.thumb.url);
        options.setDirectUrl(t.image.url);
        if ("delete_url" in t.image) {
            options.setDeleteUrl(t.image.delete_url);
        }
        return ResultCode.Success;
    }

    local message = "Upload failed";
    if (t != null && "error" in t && t.error != null && "message" in t.error) {
        message = t.error.message;
    } else if (t != null && "success" in t && t.success != null && "message" in t.success) {
        message = t.success.message;
    }
    WriteLog("error", "[2i.cz] " + message + " (HTTP " + nm.responseCode() + ")");

    return nm.responseCode() == 400 ? ResultCode.FatalError : ResultCode.Failure;
}
