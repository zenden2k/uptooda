const BASE_URL = "https://pixeldrain.com";
//const CURLOPT_USERNAME = 10173;
//const CURLOPT_PASSWORD = 10174;
const CURLOPT_HTTPAUTH = 107;
const CURLOPT_USERPWD = 10005;
const CURLAUTH_BASIC = 1;

function UploadFile(FileName, options) {
    local name = ExtractFileName(FileName);
    local mime = GetFileMimeType(name);

    local login = ServerParams.getParam("Login");
    local apiKey = ServerParams.getParam("apiKey");

    if (login != "" && apiKey == "") {
        WriteLog("error", "You must provide valid API key");
        return 0;
    }
    nm.setUrl(BASE_URL + "/api/file/" + nm.urlEncode(name) + (apiKey.len() ? "" : "?anonymous=true"));
    nm.setMethod("PUT");

    if (apiKey.len()) {
        nm.setCurlOptionInt(CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
        nm.setCurlOption(CURLOPT_USERPWD, ":" + apiKey);
    }

    nm.doUpload(FileName, "");

    local sJSON = nm.responseBody();
    local t = ParseJSON(sJSON);
    if (nm.responseCode() == 201) {
        if (t != null && "id" in t) {
            options.setThumbUrl(BASE_URL + "/api/file/" + t.id + "/thumbnail");
            options.setViewUrl(BASE_URL + "/u/" + t.id);
            return 1;
        } 
    }
    if ("message" in t) {
        WriteLog("error", "[pixeldrain.com] Upload error: " + t.message);
    }
    return 0;
}

function GetServerParamList() {
    return {
        apiKey = { title = "API key", required = true },
    };
}