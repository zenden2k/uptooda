/**
 * FEX.net anonymous file uploader.
 *
 * The upload protocol is based on the FEX.net Thunderbird Filelink extension:
 * https://api.fex.net/api/v1
 */

const API_BASE_URL = "https://api.fex.net/api/v1";
const DOWNLOAD_BASE_URL = "https://fex.net/s/";
const CHUNK_SIZE = 4194304; // 4 MiB

function _LogError(message) {
    local details = nm.responseBody();
    local errorMessage = "[fex.net] " + message + " Response code: " + nm.responseCode();
    if (details != "") {
        errorMessage += "\n" + details;
    }
    WriteLog("error", errorMessage);
}

function min(a,b) {
    return a < b ? a : b;
}


function _GetUploadToken() {
    nm.doGet(API_BASE_URL + "/anonymous/upload-token");
    if (nm.responseCode() != 200) {
        _LogError("Failed to obtain an anonymous upload token.");
        return "";
    }

    local response = ParseJSON(nm.responseBody());
    if (response == null || !("token" in response) || response.token == "") {
        WriteLog("error", "[fex.net] The upload-token response does not contain a token.");
        return "";
    }
    return response.token;
}

function _InitializeUpload(token, fileName, fileSize) {
    local request = {
        directory_id = null,
        anon_upload_root_id = null,
        size = fileSize,
        name = fileName
    };

    nm.setUrl(API_BASE_URL + "/anonymous/file");
    nm.addQueryHeader("Authorization", "Bearer " + token);
    nm.addQueryHeader("Content-Type", "application/json");
    nm.doPost(ToJSON(request));

    if (nm.responseCode() != 200 && nm.responseCode() != 201) {
        _LogError("Failed to initialize the upload.");
        return null;
    }

    local response = ParseJSON(nm.responseBody());
    if (response == null || !("location" in response) || !("anon_upload_link" in response)
        || response.location == "" || response.anon_upload_link == "") {
        WriteLog("error", "[fex.net] The upload initialization response is incomplete.");
        return null;
    }
    return response;
}

function _CreateUploadResource(uploadUrl, token, fileName, fileSize) {
    nm.setUrl(uploadUrl);
    nm.setMethod("POST");
    nm.addQueryHeader("Authorization", "Bearer " + token);
    nm.addQueryHeader("fsp-filename", nm.urlEncode(fileName));
    nm.addQueryHeader("fsp-size", fileSize.tostring());
    nm.addQueryHeader("fsp-version", "1.0.0");
    nm.doUpload("", "");

    if (nm.responseCode() != 201) {
        _LogError("Failed to create the upload resource.");
        return false;
    }
    return true;
}

function _UploadFileChunks(pathToFile, uploadUrl, token, fileSize) {
    local offset = 0;
    while (offset < fileSize) {
        local chunkSize = min(CHUNK_SIZE, fileSize - offset).tointeger();

        nm.setUrl(uploadUrl);
        nm.setMethod("PATCH");
        nm.addQueryHeader("Authorization", "Bearer " + token);
        nm.addQueryHeader("Content-Type", "application/offset+octet-stream");
        nm.addQueryHeader("fsp-offset", offset.tostring());
        nm.addQueryHeader("fsp-version", "1.0.0");
        nm.setChunkOffset(offset);
        nm.setChunkSize(chunkSize);

        if (!nm.doUpload(pathToFile, "")) {
            _LogError("Failed to upload a file chunk at offset " + offset + ".");
            return false;
        }

        local isLastChunk = offset + chunkSize >= fileSize;
        if ((!isLastChunk && nm.responseCode() != 204)
            || (isLastChunk && nm.responseCode() != 200 && nm.responseCode() != 204)) {
            _LogError("Unexpected response while uploading a file chunk at offset " + offset + ".");
            return false;
        }
        offset += chunkSize;
    }
    return true;
}

// Uploads a file anonymously. FEX.net download links expire after seven days.
function UploadFile(pathToFile, options) {
    local task = options.getTask().getFileTask();
    local fileName = task.getDisplayName();
    local fileSize = GetFileSize(pathToFile);
    if (fileSize < 0) {
        WriteLog("error", "[fex.net] Failed to determine the file size.");
        return ResultCode.Failure;
    }

    local token = _GetUploadToken();
    if (token == "") {
        return ResultCode.Failure;
    }

    local upload = _InitializeUpload(token, fileName, fileSize);
    if (upload == null) {
        return ResultCode.Failure;
    }
    if (!_CreateUploadResource(upload.location, token, fileName, fileSize)) {
        return ResultCode.Failure;
    }
    if (!_UploadFileChunks(pathToFile, upload.location, token, fileSize)) {
        return ResultCode.Failure;
    }

    options.setViewUrl(DOWNLOAD_BASE_URL + upload.anon_upload_link);
    return ResultCode.Success;
}
