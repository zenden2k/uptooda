const UPLOAD_SERVER_URL = "https://nitroflare.net/plugins/fileupload/getServer";

function UploadFile(fileName, options) {
    nm.enableResponseCodeChecking(false);
    nm.doGet(UPLOAD_SERVER_URL);

    if (nm.responseCode() != 200) {
        WriteLog("error", "[nitroflare.net] Failed to get upload server. Response code: " + nm.responseCode());
        return ResultCode.Failure;
    }

    local uploadUrl = nm.responseBody();
    if (uploadUrl == "") {
        WriteLog("error", "[nitroflare.net] Upload server URL is empty.");
        return ResultCode.Failure;
    }

    local task = options.getTask().getFileTask();
    nm.setUrl(uploadUrl);
    nm.addPostFieldFile("files", fileName, task.getDisplayName(), GetFileMimeType(fileName));
    nm.addPostField("user", ServerParams.getParam("Password"));
    nm.doUploadMultipartData();

    local responseBody = nm.responseBody();
    local response = ParseJSON(responseBody);
    if (response == null) {
        WriteLog("error", "[nitroflare.net] Failed to decode JSON response:\n" + responseBody);
        return ResultCode.Failure;
    }

    if (nm.responseCode() < 200 || nm.responseCode() >= 300) {
        WriteLog("error", "[nitroflare.net] Upload failed. Response code: " + nm.responseCode());
        return ResultCode.Failure;
    }

    if (typeof response != "table" || !("files" in response) || typeof response.files != "array"
            || response.files.len() == 0 || typeof response.files[0] != "table" || !("url" in response.files[0])) {
        WriteLog("error", "[nitroflare.net] File URL is missing in the server response.");
        return ResultCode.Failure;
    }

    options.setViewUrl(response.files[0].url);
    return ResultCode.Success;
}
