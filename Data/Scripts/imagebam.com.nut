const BASE_URL = "https://www.imagebam.com/";

function UploadFile(FileName, options) {
    local name = ExtractFileName(FileName);
    local mime = GetFileMimeType(name);

    nm.doGet(BASE_URL);

    if (nm.responseCode() != 200) {
        WriteLog("error", "[imagebam.com] Cannot obtain CSRF token");
        return 0;
    }

    local doc = Document(nm.responseBody());
    local form = doc.find("form");
    local node = doc.find("meta[name=csrf-token]");
    local csrfToken = "";
    if (node.length()) {
        csrfToken = node.attr("content");
    } else {
        WriteLog("error", "[imagebam.com] Cannot find CSRF token");
        return 0;
    }

    nm.setUrl(BASE_URL + "upload/session");
    nm.setReferer(BASE_URL);
    nm.addQueryHeader("X-Requested-With", "XMLHttpRequest");
    nm.addQueryHeader("X-CSRF-TOKEN", csrfToken);
	
    nm.addPostField("thumbnail_size", "2");
    nm.addPostField("content_type", "sfw");
    nm.addPostField("comments_enabled", "false");
    
    nm.doPost("");

    if (nm.responseCode() != 200) {
        WriteLog("error", "[imagebam.com] Cannot create upload session");
        return 0;
    }

    local sJSON = nm.responseBody();
    local t = ParseJSON(sJSON);
    local data = null;
    local session = "";
    
    if (t != null && "data" in t) {
        data = t.data;
        session = t.session;
    } else {
        WriteLog("error", "[imagebam.com] Invalid response (1)");
        return 0;
    }
    
    nm.setUrl(BASE_URL + "upload");
    nm.addQueryHeader("X-Requested-With", "XMLHttpRequest");
    nm.setReferer(BASE_URL);

    nm.addPostField("data", data);
    nm.addPostField("_token", csrfToken);
    nm.addPostFieldFile("files[0]", FileName, name, mime);
    nm.doUploadMultipartData();

    if (nm.responseCode() != 200) {
        WriteLog("error", "[imagebam.com] Invalid response (2)");
        return 0;
    }

    t = ParseJSON(nm.responseBody());

    if ("success" in t) {
        nm.doGet(t.success);
        
        if (nm.responseCode() != 200) {
            WriteLog("error", "[imagebam.com] Invalid response (3)");
            return 0;
        } 

        local reg = CRegExp(BASE_URL + "view/([A-Za-z0-9]+)", "mi");
                           
        if (reg.match(nm.responseBody()) ) {
            local viewUrl = reg.getMatch(0);
            if (viewUrl != "") {
                options.setViewUrl(viewUrl);
                nm.doGet(viewUrl);
                if (nm.responseCode() == 200) {
                    doc = Document(nm.responseBody());
                    node = doc.find(".main-image");
                    if (node != null) {
                        options.setDirectUrl(node.attr("src"));
                    }
                }
            }

            local reg2 = CRegExp("\\[IMG\\](.+?)\\[/IMG\\]", "mi");
            if (reg2.match(nm.responseBody())) {
                options.setThumbUrl(reg2.getMatch(1));
            }
            local reg3 = CRegExp(BASE_URL + "([A-Za-z0-9]+)/delete/([a-z0-9]+)", "mi");
            if (reg3.match(nm.responseBody())) {
                options.setDeleteUrl(reg3.getMatch(0));
            }
            return 1;
        } else {
            WriteLog("error", "[imagebam.com] Could not find links");
        }
    } else {
        WriteLog("error", "[imagebam.com] Failed to upload");
    }
    return 0;
}
