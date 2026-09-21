const CURLOPT_USERNAME = 10173;
const CURLOPT_PASSWORD = 10174;
const CURLOPT_USE_SSL = 119;
const CURLUSESSL_TRY = 1;
const CURLOPT_FTPPORT = 10017;
const CURLOPT_NOBODY = 44;

const CURLE_OK = 0;
const CURLE_REMOTE_FILE_NOT_FOUND = 78;

const SECURE_CONNECTION_NONE = 0;
const SECURE_CONNECTION_EXPLICIT = 1;
const SECURE_CONNECTION_IMPLICIT = 2;

if(ServerParams.getParam("hostname") == "") {
    ServerParams.setParam("hostname", "ftp.example.com") ;
}

if(ServerParams.getParam("folder") == "") {
    ServerParams.setParam("folder", "/somefolder") ;
}

if(ServerParams.getParam("downloadPath") == "") {
    ServerParams.setParam("downloadPath", "http://dl.example.com/somefolder");
}

function _GetProtocol() {
    local secureConnection = 0;
    try {
        secureConnection = ServerParams.getParam("secure").tointeger();
    } catch(ex) {}

    if (secureConnection == SECURE_CONNECTION_IMPLICIT) {
        return "ftps";
    } 
    return "ftp";
}

function TestConnection() {
    local host = ServerParams.getParam("hostname");
    local login = ServerParams.getParam("Login");
    local pass =  ServerParams.getParam("Password");
    local folder = ServerParams.getParam("folder");
    local secureConnection = 0;
    local activeConnectionPort = ServerParams.getParam("activeConnectionPort");

    try {
        secureConnection = ServerParams.getParam("secure").tointeger();
    } catch(ex) {}

    if(login.len()) {
        nm.setCurlOption(CURLOPT_USERNAME, login);
        nm.setCurlOption(CURLOPT_PASSWORD, pass);
    }

    if(folder.slice(0,1) != "/") {
        folder = "/" + folder;
    }
        
    if(folder.slice(folder.len()-1) != "/") {
        folder += "/";
    }
    folder = StrReplace(folder, " ", "%20");

    local url = _GetProtocol() + "://" + host + folder;

    if (secureConnection) {
        nm.setCurlOptionInt(CURLOPT_USE_SSL, CURLUSESSL_TRY);
    }

    if (activeConnectionPort != "") {
        nm.setCurlOption(CURLOPT_FTPPORT, activeConnectionPort);
    }
    
    if (nm.doGet(url)){
        if (nm.responseBody() != "") {
            return {
                status = 1,
                message = ""
            };
        }
    }

    return {
        status = 0,
        message = nm.errorString()
    };
}

function UploadFile(FileName, options) {
    local task = options.getTask().getFileTask();
    local originalFileName = task.getDisplayName();
    local newFilename = originalFileName;

    local host = ServerParams.getParam("hostname");
    local folder = ServerParams.getParam("folder");
    
    local login = ServerParams.getParam("Login");
    local pass =  ServerParams.getParam("Password");
    local downloadPath = ServerParams.getParam("downloadPath");
    local secureConnection = 0;
    local activeConnectionPort = ServerParams.getParam("activeConnectionPort");

    try {
        secureConnection = ServerParams.getParam("secure").tointeger();
    } catch(ex) {}

    if(login.len()) {
        nm.setCurlOption(CURLOPT_USERNAME, login);
        nm.setCurlOption(CURLOPT_PASSWORD, pass);
    }

    if (secureConnection) {
        nm.setCurlOptionInt(CURLOPT_USE_SSL, CURLUSESSL_TRY);
    }

    if (activeConnectionPort != "") {
        nm.setCurlOption(CURLOPT_FTPPORT, activeConnectionPort);
    }

    if(folder.slice(0,1) != "/")
        folder = "/" + folder;
        
    if(folder.slice(folder.len()-1) != "/")
        folder += "/";

    folder = StrReplace(folder, " ", "%20");
    local url = _GetProtocol() + "://" + host + folder + nm.urlEncode(newFilename);
    nm.setUrl(url);
    nm.setCurlOptionInt(CURLOPT_NOBODY, 1);
    nm.enableResponseCodeChecking(false);
    nm.doPost("");
    nm.enableResponseCodeChecking(true);
    if (nm.getCurlResult() == CURLE_OK) {
        // File exists on remote server
        newFilename = GenerateRandomFilename(originalFileName, 6);
        url = _GetProtocol() + "://" + host + folder + nm.urlEncode(newFilename);
    }
 
    nm.setUrl(url);
    nm.setMethod("PUT");
    nm.doUpload(FileName, "");
    if(nm.responseCode() != 226 && nm.responseCode() != 201) {
        local errorStr = "Code="+nm.responseCode()+"\r\n"+nm.errorString();
        
        if(nm.responseCode() == 530)
        errorStr += " (wrong username/password?)";
        WriteLog("error", errorStr);
        return 0;
    }
    if(downloadPath == ""){
        downloadPath = "http://"+host + folder;
        ServerParams.setParam("downloadPath",downloadPath);
    }
    if(downloadPath.slice(downloadPath.len()-1) != "/")
        downloadPath += "/";
    options.setDirectUrl(downloadPath+StrReplace(nm.urlEncode(newFilename),"%2E","."));

    return 1;
}

function GetServerParamList() {
    return {
        hostname = tr("ftp.server","Server IP or hostname [:port]"),
        folder = tr("ftp.remote_dir", "Remote directory"),
        downloadPath = tr("ftp.download_path", "Download path (ftp or http)"),
        secure = {
            title = tr("ftp.secure_connection", "Secure connection (SSL/TLS)"),
            type = "choice",
            items = [
                {
                    id = "",
                    label = tr("ftp.secure_connection_no", "No")
                },
                {
                    id = SECURE_CONNECTION_EXPLICIT,
                    label = tr("ftp.secure_connection_explicit", "Explicit")
                },
                {
                    id = SECURE_CONNECTION_IMPLICIT,
                    label = tr("ftp.secure_connection_implicit", "Implicit")
                }
            ]
        },
        activeConnectionPort = {
            title = tr("ftp.active_connection_port", "Active connection port"),
            type = "text"
        }
    }
}