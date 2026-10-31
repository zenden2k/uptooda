const BASE_URL = "https://tempfiles.ninja";

function UploadFile(filePath, options) {
    local task = options.getTask().getFileTask();
    local uploadUrl = BASE_URL + "/api/upload?filename=" + nm.urlEncode(task.getDisplayName());

    nm.setUrl(uploadUrl);
    nm.setMethod("POST");

    if (!nm.doUpload(filePath, "")) {
        WriteLog("error", "[tempfiles.ninja] Upload request failed.");
        return ResultCode.Failure;
    }

    local response = ParseJSON(nm.responseBody());
    if (nm.responseCode() != 201 || response == null || !("id" in response)
            || !("password" in response) || !("delete_password" in response)) {
        WriteLog("error", "[tempfiles.ninja] Invalid upload response. Response code: " + nm.responseCode());
        return ResultCode.Failure;
    }

    local downloadUrl = BASE_URL + "/d/" + response.id + "/" + response.password;
    if ("download_url" in response && response.download_url != "") {
        downloadUrl = response.download_url;
    }

    options.setViewUrl(downloadUrl);
    options.setDirectUrl(downloadUrl);
    options.setDeleteUrl(BASE_URL + "/api/delete/" + response.id + "/" + response.delete_password);
    return ResultCode.Success;
}
