const API_BASE_URL = "https://api.file.kiwi";

function _Base64UrlEncode(data) {
    local result = Base64Encode(data);
    result = StrReplace(result, "+", "-");
    result = StrReplace(result, "/", "_");
    return StrReplace(result, "=", "");
}

function _PrintError(message) {
    local details = nm.responseBody();
    local response = ParseJSON(details);
    if (response != null && "error" in response) {
        details = response.error;
    }
    WriteLog("error", "[file.kiwi] " + message + " HTTP status: " + nm.responseCode()
        + (details == "" ? "" : "\n" + details));
}

function _PadChunkNumber(number) {
    local result = number.tostring();
    while (result.len() < 5) {
        result = "0" + result;
    }
    return result;
}

function _BuildChunkOrder(count) {
    local result = [0];
    if (count > 1) {
        result.append(count - 1);
        for (local index = 1; index < count - 1; index++) {
            result.append(index);
        }
    }
    return result;
}

function _UploadChunk(pathToFile, fileSize, secretKey, fileInfo, chunkIndex) {
    local offset = chunkIndex * fileInfo.chunkSize;
    local bytesToRead = fileInfo.chunkSize;
    if (offset + bytesToRead > fileSize) {
        bytesToRead = fileSize - offset;
    }

    local plaintext = GetFileContentsEx(pathToFile, offset, bytesToRead, false);
    local encrypted = EncryptRfc8188(plaintext, secretKey);
    local uploadUrls = fileInfo.uploadUrls;
    local chunkNumber = _PadChunkNumber(chunkIndex + 1);
    local url = uploadUrls.head + uploadUrls.path + "/" + chunkNumber + "?"
        + uploadUrls.tail + "&X-Amz-Signature=" + uploadUrls.signatures[chunkIndex];

    nm.setUrl(url);
    nm.setMethod("PUT");
    foreach (name, value in uploadUrls.headers) {
        nm.addQueryHeader(name, value);
    }
    nm.addQueryHeader("Content-Type", "application/octet-stream");

    if (!nm.doUpload("", encrypted) || nm.responseCode() < 200 || nm.responseCode() >= 300) {
        _PrintError("Failed to upload chunk " + (chunkIndex + 1) + ".");
        return false;
    }
    return true;
}

function _VerifyUpload(folderId, fileId, uploadAuth) {
    local url = API_BASE_URL + "/v2/folders/" + nm.urlEncode(folderId) + "/files/"
        + nm.urlEncode(fileId) + "/upload-status?uploadAuth=" + nm.urlEncode(uploadAuth);
    if (!nm.doGet(url) || nm.responseCode() != 200) {
        _PrintError("Failed to verify the upload.");
        return false;
    }

    local response = ParseJSON(nm.responseBody());
    if (response == null || !("complete" in response) || !response.complete) {
        WriteLog("error", "[file.kiwi] The server reports that the upload is incomplete.");
        return false;
    }
    return true;
}

function UploadFile(pathToFile, options) {
    local task = options.getTask().getFileTask();
    local fileName = task.getDisplayName();
    local fileSize = GetFileSize(pathToFile);
    if (fileSize <= 0) {
        WriteLog("error", "[file.kiwi] Empty files are not supported by the API.");
        return ResultCode.Failure;
    }

    local secretKey = CryptographicRandomBytes(16);
    local secretKeyBase64 = _Base64UrlEncode(secretKey);
    local encryptedSecretKey = Base64Encode(EncryptRfc8188(secretKeyBase64, secretKey));
    local encryptedFileName = Base64Encode(EncryptRfc8188(fileName, secretKey));
    local title = ServerParams.getParam("title");
    if (title == "") {
        title = fileName;
    }
    if (title.len() > 200) {
        title = title.slice(0, 200);
    }

    local request = {
        title = title,
        mode = "send",
        encryption = {
            ske = encryptedSecretKey
        },
        files = [{
            filename = encryptedFileName,
            filesize = fileSize,
            mimetype = GetFileMimeType(pathToFile)
        }]
    };

    nm.setUrl(API_BASE_URL + "/v2/folders");
    nm.addQueryHeader("Content-Type", "application/json");
    if (!nm.doPost(ToJSON(request)) || nm.responseCode() != 200) {
        _PrintError("Failed to create a WebFolder.");
        return ResultCode.Failure;
    }

    local response = ParseJSON(nm.responseBody());
    if (response == null || !("folderId" in response) || !("uploadAuth" in response)
        || !("files" in response) || response.files.len() != 1) {
        WriteLog("error", "[file.kiwi] Invalid folder creation response.");
        return ResultCode.Failure;
    }

    local fileInfo = response.files[0];
    if (!("fileId" in fileInfo) || !("chunkSize" in fileInfo) || !("chunks" in fileInfo)
        || !("uploadUrls" in fileInfo)) {
        WriteLog("error", "[file.kiwi] The response does not contain upload information.");
        return ResultCode.Failure;
    }

    foreach (chunkIndex in _BuildChunkOrder(fileInfo.chunks)) {
        if (!_UploadChunk(pathToFile, fileSize, secretKey, fileInfo, chunkIndex)) {
            return ResultCode.Failure;
        }
    }

    if (!_VerifyUpload(response.folderId, fileInfo.fileId, response.uploadAuth)) {
        return ResultCode.Failure;
    }

    local folderUrl = "folderUrlBase" in response
        ? response.folderUrlBase
        : ("folderUrl" in response ? response.folderUrl : "https://file.kiwi/" + response.folderId);
    options.setViewUrl(folderUrl + "#" + secretKeyBase64);
    return ResultCode.Success;
}

function GetServerParamList() {
    return {
        title = "WebFolder title"
    };
}
