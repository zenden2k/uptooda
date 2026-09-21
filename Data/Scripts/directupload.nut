const BASE_HOST = "https://www.directupload.eu";

function Authenticate() {
    nm.doGet(BASE_HOST + "/mitglieder/");
    if (nm.responseCode() != 200) {
        WriteLog("error", "[directupload.eu] Failed to load the login page.");
        return ResultCode.Failure;
    }
    local doc = Document(nm.responseBody());
    local inputElement = doc.find("input[name=\"csrf_token\"]");
    if (!inputElement.length()) {
        WriteLog("error", "[directupload.eu] Failed to obtain CSRF token.");
        return ResultCode.Failure;
    }
    local csrfToken = inputElement.attribute("value");

    local login = ServerParams.getParam("Login");
    local pass = ServerParams.getParam("Password");
    if (login == "" || pass == "") {
        return ResultCode.Failure;
    }
    nm.doGet(BASE_HOST + "/");

    nm.setReferer(BASE_HOST + "/");
    nm.setUrl(BASE_HOST + "/index.php?mode=user");
    nm.addPostField("benutzername", login);
    nm.addPostField("csrf_token", csrfToken);
    nm.addPostField("passwort", pass);
    nm.addPostField("everlasting", "");
    nm.addPostField("anmelden", "Einloggen");
    nm.doPost("");

    if (nm.responseCode() == 200 || nm.responseCode() == 302) {
        doc = Document(nm.responseBody());
        if (doc.find("div.message.error").length()) {
            WriteLog("error", "[directupload.eu] Failed to authenticate. Invalid login or password.");
            return ResultCode.Failure;
        }
        return ResultCode.Success;
    } else {
        WriteLog("error", "[directupload.eu] Failed to authenticate. Response code: " + nm.responseCode());
    }
    return ResultCode.Failure;
}

function _AnonymousUpload(fileName, options) {
    local mimeType = GetFileMimeType(fileName);
    local fname = ExtractFileName(fileName);
    nm.setReferer(BASE_HOST + "/");
    nm.setUrl(BASE_HOST + "/api/upload_http_resize.php");
    nm.addPostField("file", "data:" + mimeType + ";base64," + Base64Encode(GetFileContents(fileName)));
    nm.addPostField("filename", fname);
    nm.addPostField("showtext", "0");
    nm.setUploadAction();
    nm.doUploadMultipartData();

    if (nm.responseCode() != 200) {
        WriteLog("error", "[directupload.eu] Failed to upload file, response code=" + nm.responseCode());
        return 0;
    }
    local imgId = nm.responseBody();

    nm.setUrl(BASE_HOST + "/upload_a2/");
    nm.addPostField("img_id[]", imgId);
    nm.addPostField("file_name[]", fname);
    nm.addPostField("img_resize", "0");
    nm.addPostField("autodel", "0");
    nm.addPostField("showtext", "0");
    
    nm.doPost("");
    if (nm.responseCode() == 200) {
        local reg = CRegExp("\\[url=(.+?)\\]\\[img\\](.+?)\\[/img\\]", "mi");
        local viewUrl = "";
        local thumbUrl = "";
        local arr = reg.findAll(nm.responseBody());
        if (arr.len() >= 2) {
            viewUrl = arr[0][1];
            thumbUrl = arr[0][2];
            options.setViewUrl(viewUrl);
            options.setThumbUrl(thumbUrl);
            options.setDirectUrl(arr[1][2]);
            
            local reg2 = CRegExp("/delfile/(\\w+?)/", "mi");
            if (reg2.match(nm.responseBody())) {
                options.setDeleteUrl(BASE_HOST + "/delfile/" + reg2.getMatch(1) + "/");
            }
            return 1;
        } else {
            WriteLog("error", "[directupload.eu] Failed to obtain data from server's response.");
        }
    }
    return 0;
}

function UploadFile(fileName, options) {
    local login = ServerParams.getParam("Login");

    if (login == "") {
        return _AnonymousUpload(fileName, options);
    }

    local task = options.getTask().getFileTask();
    local album = options.getFolder();
    local albumId = options.getFolderID();
    local thumbAddText = options.getParam("THUMBADDTEXT").tointeger() && options.getParam("THUMBCREATE").tointeger();

    if (albumId == "") {
        WriteLog("error", "[directupload.eu] You should choose an album before uploading into account.");
        return 0;
    }
    local mimeType = GetFileMimeType(fileName);
    local fname = ExtractFileName(fileName);
    nm.setReferer("");
    nm.addQueryHeader("Origin", BASE_HOST);
    nm.addQueryHeader("Pragma", "no-cache");
    nm.setUrl(BASE_HOST + "/api/upload_http_usrmulti.php");

    /*if (albumId == "") {
        nm.addPostField("new_gallery_name", "New album");
    }*/

    nm.addPostField("file", "data:" + mimeType +";base64," + Base64Encode(GetFileContents(fileName)));
    nm.addPostField("filename", task.getDisplayName());
    nm.addPostField("showtext", thumbAddText? "1" : "0");
    nm.addPostField("gal_id", albumId);
    nm.addPostField("gal_nm", album.getTitle());
    nm.addPostField("st_g", "0");
    nm.setUploadAction();
    nm.doUploadMultipartData();

    if (nm.responseCode() != 200) {
        WriteLog("error", "[directupload.eu] Failed to upload file, response code=" + nm.responseCode());
        return 0;
    }

    local imgId = nm.responseBody();

    nm.setUrl(BASE_HOST + "/index.php?mode=user&act=m_upload2");
    nm.addPostField("img_id[]", imgId);
    nm.addPostField("file_name[]", task.getDisplayName());
    nm.addPostField("img_resize", "0");
    nm.addPostField("showtext", thumbAddText? "1" : "0");
    nm.addPostField("gal_id", albumId);
    nm.addPostField("new_gallery_name", album.getTitle());
    nm.doPost("");

    if (nm.responseCode() != 200) {
        WriteLog("error", "[directupload.eu] Failed to upload file on step 3, response code=" + nm.responseCode());
        return 0;
    }

    local reg = CRegExp("\\[url=(.+?)\\]\\[img\\](.+?)\\[/img\\]", "mi");
    local viewUrl = "";
    local thumbUrl = "";
    local arr = reg.findAll(nm.responseBody());
    if (arr.len() >= 2) {
        viewUrl = arr[0][1];
        thumbUrl = arr[0][2];
        options.setViewUrl(viewUrl);
        options.setThumbUrl(thumbUrl);
        options.setDirectUrl(arr[1][2]);    
        return 1;
    } else {
        WriteLog("error", "[directupload.eu] Failed to obtain data from server's response.");
    }
    return 0;
}

function GetFolderList(list) {
    nm.setReferer(BASE_HOST + "/");
    nm.doGet(BASE_HOST + "/index.php?mode=user&act=gal");

    if (nm.responseCode() != 200) {
        WriteLog("error", "[directupload.eu] Failed to load album list, response code=" + nm.responseCode());
        return 0;
    }

    local doc = Document(nm.responseBody());
    local albumDivs = doc.find("#gallerybox .galleryblock");

    if (albumDivs.length()) {
        albumDivs.each(function(index, elem) {
            local folder = CFolderItem();
            local openLink = elem.find("a[title='Album bearbeiten']");
            folder.setTitle(strip(openLink.text()));
            folder.setItemCount(0);
            local openUrl = openLink.attribute("href");
            local reg = CRegExp("id=(\\d+)", "mi");
            if (reg.match(openUrl) ) {
                folder.setId(reg.getMatch(1));
                local viewLink = elem.find("a[title='fertiges Album ansehen']");
                folder.setViewUrl(BASE_HOST + "/" + viewLink.attribute("href"));
                list.AddFolderItem(folder);
            }
        });
        return 1;
    }
    return 0;
}

function CreateFolder(parentAlbum, album) {
    nm.setReferer(BASE_HOST + "/");
    nm.setUrl(BASE_HOST + "/index.php?mode=user&act=gal");
    nm.addPostField("new_gal_name", album.getTitle());
    nm.addPostField("new_gal", "Neues+Album+anlegen");
    nm.doPost("");

    if (nm.responseCode() != 200) {
        WriteLog("error", "[directupload.eu] Failed to create an album, response code=" + nm.responseCode());
        return 0;
    }

    return 1;
}
