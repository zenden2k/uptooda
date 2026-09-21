const BASE_URL = "https://www.upload.ee";

function _GetNonce(url) {
    nm.doGet(url);

    if (nm.responseCode() != 200) {
        WriteLog("error", "[upload.ee] Failed to load the main page!");
        return "";
    }

    local doc = Document(nm.responseBody());
    local nonce = doc.find("input[name=\"___nonce\"]").attribute("value");

    if (nonce == "") {
        WriteLog("error", "[upload.ee] Failed to obtain 'nonce' value!");
    }

    return nonce;
}
function Authenticate() {
    local nonce = _GetNonce(BASE_URL);
    if (nonce == "") {
        return  ResultCode.Failure;
    }
    local login = ServerParams.getParam("Login");
    local password = ServerParams.getParam("Password");
    nm.setUrl(BASE_URL + "/login.html");
    nm.addPostField("u[username]", login);
    nm.addPostField("u[password]", password);
    nm.addPostField("u[autologin]", "1");
    nm.addPostField("u[page]", "");
    nm.addPostField("___nonce", nonce);
    nm.addPostField("login", "Войти");
    nm.doPost("");

    if (nm.responseCode() == 200) {
        return ResultCode.Success;
    }

    WriteLog("error", "[upload.ee]  Failed to authenticate, response code: " + nm.responseCode());
    return ResultCode.Failure;
}

function UploadFile(FileName, options) {
    local login = ServerParams.getParam("Login");
    local albumId = options.getFolderID();

    const finishUrl = "/?page=finished&upload_id=";
    nm.setReferer(BASE_URL);
    nm.doGet(BASE_URL + "/ubr_link_upload.php?rnd_id=" + time());

    if (nm.responseCode() == 200) {
        local reg = CRegExp("startUpload\\(\"(.+?)\"", "mi");
                           
        if ( reg.match(nm.responseBody()) ) {
            local uploadId = reg.getMatch(1);
            
            nm.setReferer(BASE_URL);
            nm.setUrl(BASE_URL + "/cgi-bin/ubr_upload.pl?X-Progress-ID=" + nm.urlEncode(uploadId)+ "&upload_id=" + nm.urlEncode(uploadId));

            nm.addPostFieldFile("upfile_0", FileName, ExtractFileName(FileName), GetFileMimeType(FileName));
            nm.addPostField("link", "");
            nm.addPostField("email", "");
            nm.addPostField("category", "cat_file");
            if (login != "") {
                nm.addPostField("gallery_id", albumId);
            }
            nm.addPostField("big_resize", "none");
            nm.addPostField("small_resize", "120x90");
            
            nm.doUploadMultipartData();

            if (nm.responseCode() == 200) {
                local data = nm.responseBody();

                if (data.find(finishUrl, 0) == null) {
                    WriteLog("error", "[upload.ee] Upload failed");
                    return 0;
                }

                nm.doGet(BASE_URL + finishUrl +  nm.urlEncode(uploadId));

                if (nm.responseCode() == 200) {
                    local doc = Document(nm.responseBody());
                    local downloadUrl = doc.find("input#file_src").at(0).attr("value");
                    local imageUrl = doc.find("input#image_src").at(0).attr("value");
                    local thumbUrl = doc.find("input#thumb_src").at(0).attr("value");

                    if (downloadUrl == "") {
                        WriteLog("error", "[upload.ee] Unable to find File URL on the page");
                        return 0;
                    }
                    options.setViewUrl(downloadUrl);
                    options.setDirectUrl(imageUrl);
                    options.setThumbUrl(thumbUrl);
                    local reg2 = CRegExp("href=\"(.+\\?killcode=\\w+)\"", "i");
                    if (reg2.match(nm.responseBody())) {
                        options.setDeleteUrl(reg2.getMatch(1))
                    }
                    
                    return 1;
                } 
            }
        } else {
            WriteLog("error", "[upload.ee] Unable to obtain Upload ID");
        }
    }

    return 0;
}

function CreateFolder(parentAlbum, album) {
    local nonce = _GetNonce(BASE_URL);
    if (nonce == "") {
        return  ResultCode.Failure;
    }

    nm.setUrl(BASE_URL + "/?page=mygalleries&gid=-1");
    nm.addPostField("g[name]", album.getTitle());
    nm.addPostField("g[private]", "1");
    nm.addPostField("g[pass]", "");
    nm.addPostField("save_g", "Сохранить");
    nm.addPostField("___nonce", nonce);
    nm.doPost("");

    if (nm.responseCode() == 200) {
        return ResultCode.Success;
    }
    WriteLog("error", "[upload.ee] Failed to create the album, response code: " + nm.responseCode());
    return ResultCode.Failure; 
}

function GetFolderList(list) {
    nm.doGet(BASE_URL + "/?page=mygalleries");
    if (nm.responseCode() != 200) {
        WriteLog("error", "[upload.ee] Failed to obtain folder list, response code: " + nm.responseCode());
        return ResultCode.Failure;  
    }

    local doc = Document(nm.responseBody());
    local table = doc.find("#table");

    if (!table.length()) {
        WriteLog("error", "[upload.ee] Cannot find the albums table on the page");
        return ResultCode.Failure;  
    }

    local albumListRows = doc.find("#table tbody tr");

    albumListRows.each(function(index, elem) {
        if (index == 0) {
            return; // Skip header
        }
        local cell = elem.find("td").at(1);
        local linkNode = cell.find("a").at(0);
        local editUrl = linkNode.attr("href");
        local reg = CRegExp("/gallery/(\\d+)/", "mi");
        local albumId = "";            
        if (reg.match(editUrl) ) {
            albumId = reg.getMatch(1);
        }
        local album = CFolderItem();
        album.setId(albumId);
        album.setTitle(linkNode.find("b").ownText());

        // There are no child albums
        album.setItemCount(0);
        list.AddFolderItem(album);
    });
    return ResultCode.Success;
}