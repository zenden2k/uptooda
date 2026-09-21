/* by Alex_Qwerty */
// API for authorization and upload https://www.mirrored.to/p/api-doc
const API_URL = "https://www.mirrored.to";

function _RegexSimple(data,regStr,start) {
    local ex = regexp(regStr);
    local res = ex.capture(data, start);
    local resultStr = "";
    if(res != null){
        resultStr = data.slice(res[1].begin, res[1].end);
    }
    return resultStr;
}

function _JoinArray(arr, separator) {
    local result = "";
    foreach (element in arr) {
        result = result + element + separator;
    }
    if (arr.len() > 0) {
        result = result.slice(0, -separator.len());
    }
    return result;
}

function _ChooseMirrors(fileName, fileSize, availableMirrors, defaultMirrors, userMirrors, maxMirrors) {
    local mirrors = [];

    if (typeof userMirrors == "array") {
        foreach(mirror in userMirrors) {
            if (mirror[1] * 1048576 >= fileSize) {
                mirrors.push(mirror[0]);
                if (mirrors.len() >= maxMirrors) {
                    break;
                }
            }
        }
        return mirrors;
    }

    foreach(mirror in defaultMirrors) {
        if (mirror[1] * 1048576 >= fileSize && mirrors.find(mirror[0]) == null) {
            mirrors.push(mirror[0]);
            if (mirrors.len() >= maxMirrors) {
                break;
            }
        }
    }
    
    foreach(mirror in availableMirrors) {
        if (mirror[1] * 1048576 >= fileSize && mirrors.find(mirror[0])==null) {
            mirrors.push(mirror[0]);
            if (mirrors.len() >= maxMirrors) {
                break;
            }
        }
    }
    return mirrors;
}

function _UploadToAccount(fileName, options) {
    local task = options.getTask().getFileTask();
    local apiKey = ServerParams.getParam("Password");
    nm.setUrl(API_URL + "/api/v1/get_upload_info");
    nm.addPostField("api_key", apiKey);
    nm.doPost("");

    if (nm.responseCode() == 200) {
        local t = ParseJSON(nm.responseBody());
        if (!"status" in t || !t.status) {
            WriteLog("error", "[mirrorcreator.com] API error");
            return 0;
        }

        local mirrors = _ChooseMirrors(fileName, task.getFileSize(), t.message.available_mirrors, 
                                        t.message.default_mirrors, t.message.user_mirrors, t.message.max_mirrors
                                        );
        /*WriteLog("info", "[mirrorcreator.com] Mirrors to be used for upload: " + ToJSON(mirrors));
            return 0;
        */
        if (mirrors.len() == 0) {
            WriteLog("error", "[mirrorcreator.com] No mirrors available for upload.");
            return 0;
        } else {
            local fileUploadUrl = t.message.file_upload_url; 
            local uploadId = t.message.upload_id;
            nm.setUrl(fileUploadUrl);
            nm.addPostField("api_key", apiKey);
            nm.addPostField("upload_id", uploadId);
            nm.addPostFieldFile("Filedata", fileName, ExtractFileName(fileName), GetFileMimeType(ExtractFileName(fileName)));
            nm.doUploadMultipartData();

            if (nm.responseCode() == 200) {
                local t3 = ParseJSON(nm.responseBody());
                if (t3.status) {
                    nm.setUrl(API_URL + "/api/v1/finish_upload");
                    nm.addPostField("api_key", apiKey);
                    nm.addPostField("upload_id", uploadId);
                    nm.addPostField("mirrors", _JoinArray(mirrors, ","));
                    nm.doPost("");
                    
                    if (nm.responseCode() == 200) {
                        local t2 = ParseJSON(nm.responseBody());
                        if (t2.status) {
                            if ("file_id" in t2.message && t2.message.file_id != "") {
                                options.setViewUrl(t2.message.short_url);
                                return 1;
                            }
                        } else {
                            WriteLog("error", "[mirrorcreator.com] Upload failed. API error: " + t2.message);
                        }
                    } else {
                        WriteLog("error", "[mirrorcreator.com] Failed to finish upload: API error. Response code: " + nm.responseCode());
                    }
                } else {
                    WriteLog("error", "[mirrorcreator.com] Upload failed. API error: " + t3.message);
                }
            } else {
                WriteLog("error", "[mirrorcreator.com] API error. Response code: " + nm.responseCode());
            }   
        }
    } else {
        local errorMessage = "[mirrorcreator.com] API error: " + nm.responseCode();
        local t = ParseJSON(nm.responseBody());
        if (t && "message" in t) {
            errorMessage += "\n" + t.message;
        }
        WriteLog("error", errorMessage);
    }
    return 0;
}

function UploadFile(FileName, options) {
    local apiKey = ServerParams.getParam("Password");

    if (apiKey != "") {
        return _UploadToAccount(FileName, options);
    }

    local fid = format("%c%c%c%c%c%c", rand()%26+'A', rand()%26+'A', rand()%26+'A', rand()%26+'A', rand()%26+'A', rand()%26+'A');
    local fsize = GetFileSize(FileName);
    local fn = ExtractFileName(FileName);
    local url = "https://www.mirrored.to/fnvalidator.php?fn=" + nm.urlEncode(fn) + "%20(" + fsize + ");&fid=" + fid + ";"
    nm.addQueryHeader("X-Requested-With", "XMLHttpRequest");
    nm.doGet(url);

    local fnv = nm.responseBody();

    nm.setUrl("https://www.mirrored.to/uploadify/uploadify.php");
    nm.addQueryHeader("X-Requested-With", "");
    nm.addQueryHeader("User-Agent", "Shockwave Flash");
    nm.addPostField("Filename", fn);
    nm.addPostField("folder", "/uploads");
    nm.addPostFieldFile("Filedata", FileName, fn, "");
    nm.addPostField("Upload", "Submit Query");
    nm.doUploadMultipartData();
    
    if (nm.responseCode() == 200){
        local data = nm.responseBody();
        local t = ParseJSON(data);
        local fn2 = t.fileName;
        local pd = Base64Encode( fn2 + "#0#"+fsize+";0;@e@#H#downloadgg;usersdrive;clicknupload;dailyuploads;turbobit;megaupnet;krakenfiles;sendcm;gofileio;onefichier;#P##SC#" );
        nm.addQueryHeader("User-Agent", "");
        nm.doGet("https://www.mirrored.to/process.php?data=" + pd);

        if (nm.responseCode() == 200) {
            local data2 = nm.responseBody();

            url = _RegexSimple(data2, "(https://mir.cr/[0-9A-Z]+)", 0);

            if (url != "") {
                options.setViewUrl(url);
                return 1;
            } 
        }

    }

    return 0;
}
