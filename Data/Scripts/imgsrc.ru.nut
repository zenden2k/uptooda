const BASE_HOST = "https://imgsrc.ru";

function Authenticate() {
    local login = ServerParams.getParam("Login");
    local pass =  ServerParams.getParam("Password");
    if (login == "" || pass == "") {
        return 0;
    }
    nm.doGet(BASE_HOST + "/main/login.php?cnt=%2Fmembers%2F");

    nm.setReferer(BASE_HOST + "/main/login.php?cnt=%2Fmembers%2F");
    nm.setUrl(BASE_HOST + "/main/login.php?cnt=%2Fmembers%2F");
    //nm.addQueryHeader("Accept-Language", "ru-RU,ru;q=0.8,en-US;q=0.5,en;q=0.3");
    //nm.addQueryHeader("Accept", "text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,*/*;q=0.8");
    nm.addQueryHeader("Origin", BASE_HOST);
    
    //nm.addQueryHeader("TE", "trailers");
    nm.addPostField("login", login);
    nm.addPostField("pass", pass);
    nm.addPostField("cnt", nm.urlEncode("/members/?nc=" + time()));
    nm.doPost("");

    if (nm.responseCode() == 200 || nm.responseCode() == 302) {
        return 1;
    }
    return 0;
}

function GetFolderList(list) {
    //nm.addQueryHeader("Accept-Language", "ru-RU,ru;q=0.8,en-US;q=0.5,en;q=0.3");
    nm.addQueryHeader("Origin", BASE_HOST);

    nm.setReferer(BASE_HOST + "/main/login.php?cnt=%2Fmembers%2F");
    nm.doGet(BASE_HOST + "/members/");

    if (nm.responseCode() != 200) {
        return 0;
    }

    local doc = Document(nm.responseBody());
    local albumListRows = doc.find("#album_list tbody tr");

    if (albumListRows.length()) {
        albumListRows.each(function(index, elem) {
            if (index == albumListRows.length() - 1) {
                return;
            }
            local titleCell = elem.find("td").at(0);
            local linkNode = titleCell.find("a").at(0);
            local editUrl = linkNode.attr("href");
            local reg = CRegExp("aid=(\\d+)", "mi");
            local albumId = "";            
            if (reg.match(editUrl) ) {
                albumId = reg.getMatch(1);
            }
            local album = CFolderItem();
            album.setId(albumId);
            album.setTitle(linkNode.ownText());
            // There is no child albums
            album.setItemCount(0);
            list.AddFolderItem(album);
        });
        return 1;
    }
    return 0;
}

function CreateFolder(parentAlbum, album) {
    nm.setReferer(BASE_HOST + "/main/login.php?cnt=%2Fmembers%2F");
    nm.doGet(BASE_HOST + "/members/");

    if (nm.responseCode() != 200) {
        return 0;
    }
    local doc = Document(nm.responseBody());
    local rrocket = doc.find("input[name=rrocket]").attr("value");
    
    if (rrocket == "") {
        WriteLog("error", "[imgsrc.ru] Cannot obtain CSRF token.");
        return 0;
    }
    local title = album.getTitle();
    local summary = album.getSummary();

    nm.doGet(BASE_HOST + "/members/album_sets.php?rrocket=" + nm.urlEncode(rrocket) + "&name=" + nm.urlEncode(title) + "&cat=255");

    local doc = Document(nm.responseBody());
    //local form = doc.find("#album_sets");
    local albumId = doc.find("input[name=id]").attr("value");
    local albumTitle = doc.find("input[name=name]").attr("value");

    if (albumId != "") {
        album.setId(albumId);
        album.setTitle(albumTitle);
        return 1;
    } else {
        WriteLog("error", "[imgsrc.ru] Failed to create album.");
    }

    return 1;
}

function UploadFile(FileName, options) {
    local albumId = options.getFolderID();

    if (albumId == "") {
        WriteLog("error", "[imgsrc.ru] You should choose an album.");
    }

    nm.setReferer(BASE_HOST + "/members/");
    nm.doGet(BASE_HOST + "/members/album_edit.php?aid=" + nm.urlEncode(albumId)+ "&action=upload");

    if (nm.responseCode() != 200) {
        return 0;
    }

    local doc = Document(nm.responseBody());
    local rrocket = doc.find("input[name=rrocket]").attr("value");
    
    if (rrocket == "") {
        WriteLog("error", "[imgsrc.ru] Cannot obtain CSRF token.");
        return 0;
    }

    nm.setUrl(BASE_HOST + "/members/album_upload.php?id=" + nm.urlEncode(albumId));
    nm.addPostField("rrocket", rrocket);
    nm.addPostFieldFile("um[]", FileName, ExtractFileName(FileName), GetFileMimeType(FileName));
    nm.addPostField("album_id", albumId);
    nm.addPostField("album_test", (200 + Random()% 1024) + "$rnd" + (200 + Random() % 768));
    nm.addPostField("upload_type", "4");
    nm.addPostField("b_submit", "Загрузить 1 фото в этот альбом");
    nm.doUploadMultipartData();

    if (nm.responseCode() == 200) {
        local doc = Document(nm.responseBody());
        local bbCode = doc.find("input[name=bb]").attr("value");
        local reg = CRegExp("\\[URL=(.+?)\\]\\[IMG\\](.+?)\\[/IMG\\]", "mi");
        local viewUrl = "";
        local imageUrl = "";
        if (reg.match(bbCode)) {
            viewUrl = reg.getMatch(1);
            imageUrl = reg.getMatch(2);
            options.setViewUrl(viewUrl);
            options.setDirectUrl(imageUrl);
        }
        if (viewUrl != "") {
            return 1;
        } 
        WriteLog("error", "[imgsrc.ru] Failed to obtain data from server's response.");     
    } else {
        WriteLog("error", "[imgsrc.ru] Upload file, response code: " + nm.responseCode());
    }
    
    return 0;
}