const BASE_URL = "https://dubz.co";

function _PrintError(t, txt) {
    local errorMessage = "[dubz.co] " + txt + " Response code: " + nm.responseCode();
    if (t != null && "message" in t) {
        errorMessage += "\n" + t.message;
        if ("errors" in t) {
            foreach (key, value in t.errors) {
                if (typeof(value) == "array") {
                    foreach (s in value) {
                        errorMessage += "\n" + s;
                    }
                } else {
                    errorMessage += "\n" + value;
                }
            }
        }
    }
    WriteLog("error", errorMessage);
}

function Authenticate() {
    local login = ServerParams.getParam("Login");
    local pass = ServerParams.getParam("Password");
    if (login == "" || pass == "") {
        return 0;
    }
    nm.doGet(BASE_URL + "/");
    if (nm.responseCode() != 200) {
        WriteLog("error", "[dubz.co] Failed to obtain CSRF token.\nResponse code:" + nm.responseCode());
        return 0;
    }
    local doc = Document(nm.responseBody());
    local csrfToken = doc.find("meta[name=\"csrf-token\"]").at(0).attr("content");

    nm.setReferer(BASE_URL + "/");
    nm.setUrl(BASE_URL + "/login");
    nm.addQueryParam("_token", csrfToken);
    nm.addQueryParam("email", login);
    nm.addQueryParam("password", pass);

    nm.doPost("");

    if (nm.responseCode() == 200 || nm.responseCode() == 302) {
        return 1;
    }
    return 0;
}

function _ObtainDirectVideoLink(fileName, videoUrl) {
    if (videoUrl == "") {
        return "";
    }
    nm.doGet(videoUrl);
    if (nm.responseCode() != 200) {
        return "";
    }
    local doc = Document(nm.responseBody());
    local videoTag = doc.find("video");
    local res = videoTag.attr("src");
    if (res == "") {
        res = videoTag.find("source").at(0).attr("src");
    }

    local pos = res.find("#");
    if (pos != null) {
        return res.slice(0, pos);
    }
    return res;
}

function _UploadToAccount(FileName, options) {
    nm.doGet(BASE_URL + "/manage");

    if (nm.responseCode() == 200) {
        local doc = Document(nm.responseBody());
        //local csrfToken = doc.find("meta[name=\"csrf-token\"]").at(0).attr("content");
        local uploadForm = doc.find("#video_upload");
        local token = uploadForm.find("input[name=\"_token\"]").at(0).attr("value");
        local userId = uploadForm.find("input[name=\"userId\"]").at(0).attr("value");
        local videoUrl = uploadForm.find("input[name=\"video_url\"]").at(0).attr("value");
        //nm.addQueryHeader("X-CSRF-TOKEN", csrfToken);
        nm.setReferer(BASE_URL + "/manage");
        nm.addQueryHeader("X-Requested-With", "XMLHttpRequest");
        nm.addQueryParam("_token", token);
        nm.addQueryParam("userId", userId);
        nm.addQueryParamFile("video", FileName, ExtractFileName(FileName), GetFileMimeType(FileName));
        nm.addQueryParam("video_url", videoUrl);
        nm.setUrl(BASE_URL + "/upload");
        nm.doUploadMultipartData();
        local t = null;
        if (nm.responseHeaderByName("Content-Type") == "application/json") {
            t = ParseJSON(nm.responseBody());
        }

        if (nm.responseCode() == 200) {
            options.setViewUrl(videoUrl);
            options.setDirectUrl(_ObtainDirectVideoLink(FileName, videoUrl));
            return 1;
        } else {
            _PrintError(t, "Failed to upload.");
        }
    } else {
        _PrintError(null, "Got error from server. ");
        return 0;
    }

    return 0;
}

function UploadFile(FileName, options) {
    local login = ServerParams.getParam("Login");

    if (login != "") {
        return _UploadToAccount(FileName, options);
    }

    nm.doGet(BASE_URL + "/guest");

    if (nm.responseCode() == 200) {
        local doc = Document(nm.responseBody());
        local uploadForm = doc.find("#myForm");
        local token = uploadForm.find("input[name=\"_token\"]").at(0).attr("value");
        local videoUrl = uploadForm.find("input[name=\"video_url\"]").at(0).attr("value");
        local videoId = uploadForm.find("input[name=\"video_id\"]").at(0).attr("value");
        
        nm.setReferer(BASE_URL + "/guest");
        nm.addQueryHeader("X-Requested-With", "XMLHttpRequest");
        nm.addQueryParam("_token", token);
        nm.addQueryParam("video_id", videoId);
        nm.addQueryParamFile("video", FileName, ExtractFileName(FileName), GetFileMimeType(FileName));
        nm.addQueryParam("video_url", videoUrl);
        nm.setUrl(BASE_URL + "/upload2");
        nm.doUploadMultipartData();
        local t = null;
        if (nm.responseHeaderByName("Content-Type") == "application/json") {
            t = ParseJSON(nm.responseBody());
        }

        if (nm.responseCode() == 200) {
            options.setViewUrl(videoUrl);
            options.setDirectUrl(_ObtainDirectVideoLink(FileName, videoUrl));
            return 1;
        } else {
             _PrintError(t, "Failed to upload.");
        }
    } else {
        _PrintError(null, "Got error from server. ");
        return 0;
    }
    return 0;
}