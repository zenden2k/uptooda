const CURLOPT_SSH_PRIVATE_KEYFILE = 10153;
const CURLOPT_USERNAME = 10173;
const CURLOPT_PASSWORD = 10174;

if(ServerParams.getParam("hostname") == "") {
    ServerParams.setParam("hostname", "sftp.example.com") ;
}

if(ServerParams.getParam("folder") == "") {
    ServerParams.setParam("folder", "/somefolder") ;
}

if(ServerParams.getParam("downloadPath") == "") {
    ServerParams.setParam("downloadPath", "http://dl.example.com/somefolder");
}

function TestConnection() {
    local host = ServerParams.getParam("hostname");
    local login = ServerParams.getParam("Login");
    local pass =  ServerParams.getParam("Password");
    local folder = ServerParams.getParam("folder");
        
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

    local privateKeyPath = ServerParams.getParam("privateKeyPath");
    if (privateKeyPath.len()) {
        nm.setCurlOption(CURLOPT_SSH_PRIVATE_KEYFILE, privateKeyPath); //
    }

    if (nm.doGet("sftp://" + host + folder)){
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
    local task = options.getTask().getFileTask();
    local newFilename = task.getDisplayName();	
    newFilename = GenerateRandomFilename(newFilename, 6);
    local ansiFileName = newFilename;
    local host = ServerParams.getParam("hostname");
    local folder = ServerParams.getParam("folder");
    
    local login = ServerParams.getParam("Login");
    local pass =  ServerParams.getParam("Password");
    local downloadPath = ServerParams.getParam("downloadPath");
        
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
    
    local url = "sftp://" + host + folder + nm.urlEncode(ansiFileName);
    local privateKeyPath = ServerParams.getParam("privateKeyPath");
    if (privateKeyPath.len()) {
        nm.setCurlOption(CURLOPT_SSH_PRIVATE_KEYFILE, privateKeyPath);
    }

    nm.enableResponseCodeChecking(false);
    nm.setUrl(url);
    nm.setMethod("PUT");
    local result = nm.doUpload(FileName, "");
    
    if(!result) {
        local errorStr = "Code=" + nm.responseCode()+"\r\n"+nm.errorString();
        
        if(nm.responseCode() == 530) {
            errorStr += " (wrong username/password?)";
        }
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
        hostname = "Server ip or hostname [:port]"
        folder = "Remote folder"
        downloadPath = "Download path (ftp or http)",
        privateKeyPath = {
            title = "Private key path",
            type = "filename"
        }
    }
}