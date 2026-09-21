const BASE_HOST = "https://www.turboimagehost.com/";

function Authenticate() { 
    local login = ServerParams.getParam("Login");
    local password = ServerParams.getParam("Password");

    if(login == "" ) {
        WriteLog("error", "Login should not be empty!");
        return 0;
    }

    nm.addQueryHeader("Origin", BASE_HOST);
    nm.addQueryHeader("Referer", BASE_HOST + "login.tu");
    
    nm.addPostField("username", login);
    nm.addPostField("password", password);
    nm.addPostField("remember", "y");
    nm.addPostField("login", "Login");
    nm.setUrl(BASE_HOST + "login.tu");
    nm.doPost("");
    
    if (nm.responseCode() == 200) {
        if (nm.responseBody().find("You have some errors") == null) {
            return 1;
        }
    }
    WriteLog("error", "[TurboImageHost] authentication failed.");
    
    return 0;        
} 


function UploadFile(fileName, options) {
    // Загружаем главную страницу, чтобы получить endpoint для загрузки
    nm.doGet(BASE_HOST);
    if (nm.responseCode() != 200) {
        WriteLog("error", "[TurboImageHost] Failed to fetch the main page.");
        return 0;
    }

    // Извлекаем адрес для загрузки из JavaScript
    local html = nm.responseBody();
    local uploadEndpointPattern = "endpoint:\\s*['\"]([^'\"]+)['\"]";
    local reg = CRegExp(uploadEndpointPattern, "mi");
    local uploadUrl = "";
    local albumId = options.getFolderID(); 

    if (reg.match(html)) {
        uploadUrl = reg.getMatch(1);
    } else{
        WriteLog("error", "[TurboImageHost] Failed to extract upload endpoint.");
        return 0;
    }

    WriteLog("info", "[TurboImageHost] Upload endpoint: " + uploadUrl);

    local uploadId = _GenerateRandomString(20);
    local qquuid = _GenerateRandomUUID();

    // Устанавливаем URL для загрузки
    nm.setUrl(uploadUrl);

    // Добавляем заголовки
    nm.addQueryHeader("Accept", "application/json");
    nm.addQueryHeader("X-Requested-With", "XMLHttpRequest");
    nm.addQueryHeader("Origin", BASE_HOST);
    nm.addQueryHeader("Referer", BASE_HOST);

    // Добавляем параметры формы
    if (albumId != "") {
        nm.addPostField("album", albumId);
    }
    nm.addPostField("imcontent", "all");
    nm.addPostField("thumb_size", options.getParam("THUMBWIDTH"));
    nm.addPostField("upload_id", uploadId);
    nm.addPostField("qquuid", qquuid);
    nm.addPostField("qqfilename", ExtractFileName(fileName));
    nm.addPostField("qqtotalfilesize", GetFileSize(fileName).tostring());
    nm.addPostFieldFile("qqfile", fileName, ExtractFileName(fileName), GetFileMimeType(fileName));

    // Отправляем запрос
    nm.doUploadMultipartData();
    if (nm.responseCode() == 200) {
        local responseJson = ParseJSON(nm.responseBody());
        if ("success" in responseJson && responseJson.success) {
            local resultUrl = responseJson.newUrl;
            // nm.addQueryHeader("Accept", "application/json");
            nm.doGet(resultUrl);
           
            if (nm.responseCode() == 200) {
                local viewUrl = "";
                local doc = Document(nm.responseBody());
                viewUrl = doc.find("#imgCodeIPSTF").attr("value");
                options.setViewUrl(viewUrl);
                local previewBbCode = doc.find("#imgCodeIPF").attr("value");
                local reg2 = CRegExp("\\[img\\](.+?)\\[/img\\]", "im");
                if (reg2.match(previewBbCode) ) {
                    options.setThumbUrl(reg2.getMatch(1));
                }
                if (viewUrl != "") {
                    return 1;
                } else {
                    WriteLog("error", "[TurboImageHost] Failed to obtain image link.");
                }
                
            } else {
                WriteLog("error", "[TurboImageHost] Upload failed, response code = " + nm.responseCode());
            }
        } else {
            WriteLog("error", "[TurboImageHost] Upload failed, server returned success = false.");
        }
    } else {
        WriteLog("error", "[TurboImageHost] Upload failed, response code = " + nm.responseCode());
    }
    return 0;
}

function GetFolderList(list) {
    nm.addQueryHeader("Accept", "application/json");
    nm.doGet(BASE_HOST + "g.tu");

    if (nm.responseCode() == 200) {
        local doc = Document(nm.responseBody());
        doc.find(".galleryThumbs > div").each(function(index, elem) {
            local id = elem.attr("id");
            id = StrReplace(id, "gall_", "");
            local album = CFolderItem();
            album.setId(id);
            album.setTitle(strip(elem.find(".thumbTitle").text()));
            list.AddFolderItem(album);
        });
    } else {
        WriteLog("error", "[TurboImageHost] Failed to load album list, response code = " + nm.responseCode());
    }

    return 1; //success
}


function CreateFolder(parentAlbum, album) {
    nm.setUrl(BASE_HOST + "index.tu");
    nm.addPostField("addalbum", album.getTitle());
    nm.addPostField("newalbum", "Create a new gallery");
    nm.doPost("");

    if (nm.responseCode() == 200) {
        return 1;
        //album.setId(id);
    } else {
        WriteLog("error", "[TurboImageHost] Failed to create album, response code = " + nm.responseCode());
    }
        
    return 0;
}

function _GenerateRandomString(length) {
    local chars = "abcdefghijklmnopqrstuvwxyz0123456789";
    local result = "";
    for (local i = 0; i < length; i++) {
        local pos = Random() % chars.len();
        result += chars.slice(pos, pos+1);
    }
    return result;
}

function _GenerateRandomUUID() {
    local pattern = "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx";
    local result = "";
    for (local i = 0; i< pattern.len(); i++){
        if (pattern[i]=='x' || pattern[i]=='y'){
            local r = Random()%16;
            local v = (pattern[i] == 'x') ? r : (r & 0x3 | 0x8);
            result+= format("%x", v);
        }
        else{
            result += pattern.slice(i,i+1);
        }
    }
  return result;
}