const BASE_URL = "https://linkme.ufanet.ru";

function UploadFile(fileName, options) {
    nm.doGet(BASE_URL);
    if (nm.responseCode() != 200) {
        WriteLog("error", "[linkme.ufanet.ru] Cannot obtain captcha!");
        return ResultCode.Failure;
    }
    local doc = Document(nm.responseBody());
    local captchaId = doc.find("#captcha-id").attr("value");
    local captchaImgUrl = BASE_URL + "/captcha/" + captchaId + ".png";

    local captchaAnswer = AskUserCaptcha(nm, captchaImgUrl);
    if (captchaAnswer == "") {
        WriteLog("error", "[linkme.ufanet.ru] Cannot continue without captcha answer!");
        return ResultCode.FatalError;
    }

    local task = options.getTask().getFileTask();
    local displayName = task.getDisplayName();
    local mimeType = GetFileMimeType(fileName);

    nm.setUrl(BASE_URL + "/image/upload-simple");
    nm.addPostField("MAX_FILE_SIZE", "20971520");
    nm.addPostFieldFile("file", fileName, displayName, mimeType);
    nm.addPostField("captcha[id]", captchaId);
    nm.addPostField("captcha[input]", captchaAnswer);
    nm.addPostField("Залинковать", "Залинковать");
    nm.doUploadMultipartData();

    if (nm.responseCode() == 200) {
        local data = nm.responseBody();
        if (data != "") {
            local doc2 = Document(nm.responseBody());
            local directUrl = doc2.find("input.bblinks").at(0).attr("value");
            options.setDirectUrl(directUrl);
            local thumbUrl = doc2.find("#tabTorrent input.bblinks").at(0).attr("value");
            if (thumbUrl != "") {
                local reg = CRegExp("\\](.+?)\\[/img\\]", "mi");

                if (reg.match(thumbUrl)) {
                    options.setThumbUrl(reg.getMatch(1));
                }
            }

            if (directUrl != "") {
                return ResultCode.Success;
            } else {
                WriteLog("error", "[linkme.ufanet.ru] Failed to obtain image URL");
            }
        }
    }
    return ResultCode.Failure;
}