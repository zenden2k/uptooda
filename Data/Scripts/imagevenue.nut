function Authenticate() {
    local login = ServerParams.getParam("Login");
    local pass =  ServerParams.getParam("Password");
    
    if (login != "" && pass != "") {
        nm.doGet("https://www.imagevenue.com/auth/login");
        if (nm.responseCode() != 200) {
            return 0;
        }
        
        local doc = Document(nm.responseBody());
        local token = doc.find("input[name=\"_token\"]").at(0).attr("value");
        
        if (token == "") {
            WriteLog("error", "Unable to obtain login form token");
            return 0;
        }
        
        nm.setUrl("https://www.imagevenue.com/auth/login");
        nm.addPostField("_token", token);
        nm.addPostField("email", login);
        nm.addPostField("password", pass);
        nm.setCurlOptionInt(52, 0); //disable CURLOPT_FOLLOWLOCATION 
        nm.doPost("");
        
        if(nm.responseCode() == 302){
            local destUrl = nm.responseHeaderByName("Location");
            if (destUrl == "https://www.imagevenue.com/auth/login") {
                WriteLog("error", "imagevenue.com: authentication failed for username '" + login + "'");
                return 0;
            }
        }
    }
    return 1;
}

function UploadFile(FileName, options) {
    nm.doGet("https://www.imagevenue.com");
    local name = ExtractFileName(FileName);
    local mime = GetFileMimeType(name);
    
    if (nm.responseCode() == 200) {
        local doc = Document(nm.responseBody());
        local csrfToken = doc.find("meta[name=\"csrf-token\"]").at(0).attr("content");
        nm.setUrl("https://www.imagevenue.com/upload/session");
        
        nm.addQueryHeader("X-CSRF-TOKEN", csrfToken);
        nm.addQueryHeader("X-Requested-With", "XMLHttpRequest");

        nm.addPostField("thumbnail_size", "2");
        nm.addPostField("content_type", "sfw");
        nm.addPostField("comments_enabled", "false");
        nm.doPost("");
        
        if (nm.responseCode() == 200) {
            local tt = ParseJSON(nm.responseBody());
            
            if ("data" in tt) {
                nm.setUrl("https://www.imagevenue.com/upload");
                nm.addQueryHeader("X-Requested-With", "XMLHttpRequest");
                nm.addPostField("data", tt.data);
                nm.addPostField("_token", csrfToken);
                nm.addPostFieldFile("files[0]", FileName, name, mime);
                nm.doUploadMultipartData();
                if (nm.responseCode() == 200) {
                    local t = ParseJSON(nm.responseBody());
                    if ("success" in t){
                        nm.doGet(t.success); 
                        if (nm.responseCode() == 200) {
                            local doc2 = Document(nm.responseBody());
                            local bbCode = doc2.find("#bb-code").at(0).text();
                            if (bbCode == "") {
                                WriteLog("error", "imagevenue.com: Unable to find necessary information on page: \r\n" + t.success);
                                return 0;
                            }
                            local reg2 = CRegExp("\\[url=(.+?)\\]\\[img\\](.+?)\\[/img\\]", "mi");
                            local viewUrl = "";
                            local thumbUrl = "";
                            if ( reg2.match(bbCode) ) {
                                viewUrl = reg2.getMatch(1);
                                thumbUrl = reg2.getMatch(2);
                                options.setViewUrl(viewUrl);
                                options.setThumbUrl(thumbUrl);
                            }
                            if ( viewUrl != "") {
                                return 1;
                            }     
                            
                        }
                    }
                }
            } else {
                  WriteLog("error", "imagevenue.com: Invalid response from server.");
            }
        }
        
        return 0;
    }

    return 0;
}