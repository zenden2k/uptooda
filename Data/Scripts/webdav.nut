const CURLOPT_USERNAME = 10173;
const CURLOPT_PASSWORD = 10174;

if (ServerParams.getParam("hostname") == "") {
    ServerParams.setParam("hostname", "webdav.example.com");
}

if (ServerParams.getParam("folder") == "") {
    ServerParams.setParam("folder", "/") ;
}

if(ServerParams.getParam("downloadPath") == "") {
    ServerParams.setParam("downloadPath", "http://dl.example.com/somefolder");
}

function _GetHostString() {
    local host = ServerParams.getParam("hostname");
    local secureConnection = 0;
    try {
        secureConnection = ServerParams.getParam("secure").tointeger();
    } catch(ex) {}

    if (host.find("http://") == null && host.find("https://") == null) {
        host = (secureConnection ? "https://" : "http://") + host;
    }
    return host;
}

function TestConnection() {
    local host = _GetHostString();
    local login = ServerParams.getParam("Login");
    local pass =  ServerParams.getParam("Password");
    local folder = ServerParams.getParam("folder");
        
    if (login.len()) {
        nm.setCurlOption(CURLOPT_USERNAME, login);
        nm.setCurlOption(CURLOPT_PASSWORD, pass);
    }

    if (folder.slice(0,1) != "/") {
        folder = "/" + folder;
    }
        
    if (folder.slice(folder.len()-1) != "/") {
        folder += "/";
    }

    if (nm.doGet(host + folder)){
        if (nm.responseBody() != "") {
            return {
                status = 1,
                message = ""
            };
        }
    }

    return {
        status = 0,
        message = ""
    };
}

function UploadFile(FileName, options) {	
    local newFilename = ExtractFileName(FileName);
    newFilename = random() + "_" + newFilename;
    local ansiFileName = newFilename;
    local host = _GetHostString();
    local folder = ServerParams.getParam("folder");
    
    local login = ServerParams.getParam("Login");
    local pass =  ServerParams.getParam("Password");
    local downloadPath = ServerParams.getParam("downloadPath");
        
    if (login.len()) {
        nm.setCurlOption(CURLOPT_USERNAME, login);
        nm.setCurlOption(CURLOPT_PASSWORD, pass);
    }

    if (folder.slice(0,1) != "/") {
        folder = "/" + folder;
    }
        
    if (folder.slice(folder.len() - 1) != "/") {
        folder += "/";
    }
    
    local url = host + folder + nm.urlEncode(ansiFileName);

    //nm.enableResponseCodeChecking(false);
    nm.setUrl(url);
    nm.setMethod("PUT");
    local result = nm.doUpload(FileName, "");
    
    if (!result || (nm.responseCode() >= 400 && nm.responseCode() <= 599)) {
        local errorStr = "Code=" + nm.responseCode() + "\r\n" + nm.errorString();
        
        if (nm.responseCode() == 401) {
            errorStr += " (wrong username/password?)";
        }
        WriteLog("error", "[webdav] " + errorStr);
        return 0;
    }

    if (nm.responseCode() == 201 || nm.responseCode() == 200){
        if (downloadPath == ""){
            downloadPath = "http://" + host + folder;
            ServerParams.setParam("downloadPath", downloadPath);
        }
        if (downloadPath.slice(downloadPath.len() - 1) != "/") {
            downloadPath += "/";
        }
        options.setDirectUrl(downloadPath + StrReplace(nm.urlEncode(newFilename), "%2E", "."));
        return 1;
    }
    
    return 0;
}

function GetServerParamList() {
    return {
        hostname = tr("ftp.server","Server IP or hostname [:port]"),
        folder = tr("ftp.remote_dir", "Remote directory"),
        downloadPath = tr("ftp.download_path", "Download path (ftp or http)"),
        secure = {
            title = tr("ftp.secure_connection", "Secure connection (SSL/TLS)"),
            type = "boolean"
        }
    }
}