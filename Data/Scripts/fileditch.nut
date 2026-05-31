const BASE_HOST = "https://new.fileditch.com";

function UploadFile(filePath, options) {
    local task = options.getTask().getFileTask();
    nm.setUrl(BASE_HOST + "/upload.php?filename=" + nm.urlEncode(task.getDisplayName()));
    nm.addQueryHeader("Origin", BASE_HOST);
    nm.addQueryHeader("Referer", BASE_HOST + "/");

    local startTime = clock();
    nm.doUpload(filePath, "");
    local duration = clock() - startTime;

    if (nm.responseCode() == 200) {
        local sJSON = nm.responseBody();
        local t = ParseJSON(sJSON);
        if (t != null) {
            if ("success" in t && t.success == true) {
                nm.setUrl(BASE_HOST + "/upload.php?action=notify&size=" + t.size + "&dur=" + format("%.3f", duration));
                nm.doPost("");
                options.setViewUrl(t.url);
                return 1;
            } 
        }

        WriteLog("error", "[fileditch.com] Cannot obtain file URL from server's response.");
    } else {
        WriteLog("error", "[fileditch.com] Upload failed. Response code: " + nm.responseCode());
    }

    return 0;
}