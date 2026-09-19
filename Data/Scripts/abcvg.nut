function UploadFile(FileName, options) {
    nm.doGet("https://abcvg.org/");
    if (nm.responseCode() != 200) {
        WriteLog("error", "abcvg.org: Failed to load the main page. Server status code: " + nm.responseCode());
        return 0;
    }

    local doc = Document(nm.responseBody());
    local csrfInput = doc.find("input[name=\"csrf_token\"]");
    if (!csrfInput.length()) {
        WriteLog("error", "abcvg.org: Failed to obtain CSRF token.");
        return 0;
    }

    local csrfToken = csrfInput.attr("value");
    if (csrfToken == "") {
        WriteLog("error", "abcvg.org: CSRF token is empty.");
        return 0;
    }

    local task = options.getTask().getFileTask();
    local name = task.getDisplayName();
    local mime = GetFileMimeType(FileName);

    nm.setUrl("https://abcvg.org/server/php/");
    nm.setReferer("https://abcvg.org/");
    nm.addQueryHeader("Origin", "https://abcvg.org/");

    nm.addPostField("csrf_token", csrfToken);
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
