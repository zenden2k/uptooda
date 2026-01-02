function _PrintError(t, txt) {
    local errorMessage = "[postimages.org] " + txt + " Response code: " + nm.responseCode();
    if (t != null && "error" in t) {
        errorMessage += "\n" + t.error;
    }
    WriteLog("error", errorMessage);
}

function Authenticate() {
    nm.doGet("https://postimages.org/login");

    if (nm.responseCode() != 200) {
        _PrintError(null, "Failed to obtain CSRF token");
        return ResultCode.Failure;
    }
    
    local doc = Document(nm.responseBody());
    local csrf = doc.find("input[name=\"csrf_hash\"]").attr("value");

    if (csrf == "") {
        _PrintError(null, "Failed to obtain CSRF token");
        return ResultCode.Failure;
    }
    local email = ServerParams.getParam("Login");
    local password = ServerParams.getParam("Password");
    nm.setUrl("https://postimages.org/login");
    nm.addPostField("csrf_hash", csrf);
    nm.addPostField("email", email);
    nm.addPostField("password", password);
    nm.doPost("");

    if (nm.responseCode() == 200) {
        local doc = Document(nm.responseBody());
        if (doc.find("#gallery").length()) {
            return ResultCode.Success;
        } else {
            _PrintError(null, "Failed to authenticate");
        }
    } else {
        _PrintError(null, "Failed to authenticate");
    }
    return ResultCode.Failure;
}

function UploadFile(pathToFile, options) {
    local task = options.getTask().getFileTask();
    local login = ServerParams.getParam("Login");
    local password = ServerParams.getParam("Password");

    if (login != "" && password == "") {
        WriteLog("error", "[postimages.org] You must specify the password");
        return 0;
    }

    nm.doGet("https://postimg.cc/");
    local uploadSession = time().tostring() + "." + rand().tostring();
    nm.setUrl("https://postimages.org/json");
    nm.addQueryHeader("X-Requested-With", "XMLHttpRequest");
    nm.addQueryParam("gallery", "");
    nm.addQueryParam("optsize", "0"); 
    nm.addQueryParam("expire", "0"); 
    nm.addQueryParam("numfiles", "1");
    nm.addQueryParam("upload_session", uploadSession);
    nm.addQueryParamFile("file", pathToFile, task.getDisplayName(), GetFileMimeType(pathToFile));
    nm.doUploadMultipartData();
    local t = ParseJSON(nm.responseBody());

    if (nm.responseCode() == 200) {
        if ("url" in t) {
            local viewUrl = "";
            local directUrl = "";
            if ("image" in t) {
                viewUrl =  t.image;
                options.setViewUrl(viewUrl);
            }

            nm.doGet(t.url);

            if (nm.responseCode() == 200) {
                local doc = Document(nm.responseBody());
                local directUrl = doc.find("#direct").attr("value");
                options.setDirectUrl(directUrl);
                options.setDeleteUrl(doc.find("#remove").attr("value"));
                local bbCode = doc.find("#bb_thumb").attr("value");
                if (bbCode != "") {
                    local reg = CRegExp("\\[img\\](.+?)\\[/img\\]", "i");
                    if (reg.match(bbCode)) {
                        options.setThumbUrl(reg.getMatch(1));
                    }
                }
            }
            if (directUrl != "" || viewUrl != "") {
                return ResultCode.Success;
            }
        } else {
            _PrintError(t, "Failed to upload");
            return ResultCode.Failure;
        }
    } else {
        _PrintError(t, "Failed to upload");
    }

    return ResultCode.Failure;
}