function UploadFile(FileName, options) {
    local task = options.getTask().getFileTask();
    local newFilename = task.getDisplayName();
    local directory = ServerParams.getParam("directory") + "/";
    local convertUncPath = 0;
    try {
        convertUncPath = ServerParams.getParam("convertUncPath").tointeger();
    } catch (ex) {}

    local targetFile = directory + newFilename;

    if (FileExists(targetFile)) {
        local ext = GetFileExtension(newFilename);
        newFilename = ExtractFileNameNoExt(newFilename) + "_" + random() + (ext == "" ? "" : ("." + ext));
        targetFile = directory + newFilename;
    }
    local res = CopyFile(FileName, targetFile, true);
    if (!res) {
        WriteLog("error", "Copying file from \r\n" + FileName + " to \r\n" + targetFile + " failed");
        return 0;
    }

    local downloadUrl = ServerParams.getParam("downloadUrl");
    if (downloadUrl == "") {
        WriteLog("error", "downloadUrl parameter should not be empty");
        return 0;
    }
    local encodedFileName = newFilename;
    if (downloadUrl.find("://") != null) {
        encodedFileName = StrReplace(nm.urlEncode(newFilename), "%2E", ".");
    }

    options.setDirectUrl(downloadUrl + encodedFileName);

    if (downloadUrl.find("\\\\") == 0) {
        downloadUrl = downloadUrl.slice(2);
        local convertedUrl = "file://" + StrReplace(downloadUrl, "\\", "/") + StrReplace(nm.urlEncode(newFilename), "%2E", ".");
        if (convertUncPath == 1) {
            options.setDirectUrl(convertedUrl);
        } else {
            options.setViewUrl(convertedUrl);
        }
    }

    return 1;
}

function GetServerParamList() {
    return {
        directory = {
            title = "Directory",
            type = "filename",
            directory = true
        },
        downloadUrl = "Download path (ftp or http)",
        convertUncPath = {
            title = "Convert UNC path \"\\\\\" to file://",
            type = "boolean",
        }
    }
}