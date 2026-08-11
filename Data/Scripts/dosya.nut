const BASE_URL = "https://dosya.co";

function Authenticate() {
    local login = ServerParams.getParam("Login");
    local pass = ServerParams.getParam("Password");
    if (login == "" || pass == "") {
        return 0;
    }

    nm.setReferer(BASE_URL + "/login.html");
    nm.setUrl(BASE_URL + "/");
    nm.addPostField("op", "login");
    nm.addPostField("redirect", BASE_URL + "/");
    nm.addPostField("login", login);
    nm.addPostField("password", pass);

    nm.doPost("");

    if (nm.responseCode() == 200 || nm.responseCode() == 302) {
        return 1;
    }
    return 0;
}

function UploadFile(FileName, options) {
    local justFileName = ExtractFileName(FileName);
    local login = ServerParams.getParam("Login");
    local toAccount = (login != "");

    local uid = "";
    for(local i = 0; i < 12; i++) {
        uid += Random() % 10;
    }

    nm.doGet(BASE_URL + "/");

    if (nm.responseCode() == 200) {
        local doc = Document(nm.responseBody());
        local uploadForm = doc.find("form[name=\"file\"]");
        local actionUrl = uploadForm.attr("action");
        local pos = actionUrl.find("?");
        if (pos != null) {
            actionUrl = actionUrl.slice(0, pos);
        }
        local uploadUrl = actionUrl + "?upload_id=" + uid + "&js_on=1&utype=anon&upload_type=file";
        local tmpUrl = uploadForm.find("input[name=\"srv_tmp_url\"]").attr("value");  
        local sessId = uploadForm.find("input[name=\"sess_id\"]").attr("value");  
        nm.doGet(tmpUrl + "/status.html?" + uid + "=" + nm.urlEncode(justFileName) + "=" + BASE_URL + "/");

        nm.addQueryHeader("upload_type", "file");
        nm.addPostField("sess_id", sessId);
        nm.addPostField("srv_tmp_url", tmpUrl);
        nm.addPostFieldFile("file_0", FileName, justFileName, GetFileMimeType(FileName));
        nm.addPostField("file_0_descr", "");
        nm.addPostField("file_0_public", "1");
        nm.addPostField("link_rcpt", "");
        nm.addPostField("link_pass", "");
        nm.addPostField("to_folder", "")
        nm.addPostField("submit_btn", " Yüklemeye Başla ")
        nm.setUrl(uploadUrl);
        nm.doUploadMultipartData();

        if (nm.responseCode() == 200) {
            local doc2 = Document(nm.responseBody());
            local form = doc2.find("form");
            local action = form.attr("action");
            nm.setUrl(action);
            form.find("textarea").each(function(index, elem) {
                nm.addPostField(elem.attr("name"), strip(elem.text()));
            });
            nm.doPost("");
            
            if (nm.responseCode() == 200) {
                local doc3 = Document(nm.responseBody());
                local link = strip(doc3.find("textarea#ic0-").text());
                options.setViewUrl(link); 
                if (link != "") {
                    return 1;
                }
                WriteLog("error", "[dosya.co] Cannot obtain link to the uploaded file.");  
            } else {
                WriteLog("error", "[dosya.co] Cannot download page.\nResponse code:" + nm.responseCode());    
            }       
        } else {
            WriteLog("error", "[dosya.co] Failed to upload.\nResponse code:" + nm.responseCode());
        }
    } else {
        WriteLog("error", "[dosya.co] got error from server: \nResponse code:" + nm.responseCode());
    }
    return 0;
}