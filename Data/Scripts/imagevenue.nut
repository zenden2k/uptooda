const BASE_URL = "https://imagevenue.com";

function LogError(message) {
    WriteLog("error", "[imagevenue.com] " + message);
}

function Authenticate() {
    local login = ServerParams.getParam("Login");
    local password = ServerParams.getParam("Password");

    if (login == "" || password == "") {
        return ResultCode.Success;
    }

    if (!nm.doGet(BASE_URL + "/auth/login") || nm.responseCode() != 200) {
        LogError("Cannot open the login page. Response code: " + nm.responseCode());
        return ResultCode.Failure;
    }

    local doc = Document(nm.responseBody());
    local tokenNode = doc.find("input[name=\"_token\"]");
    if (!tokenNode.length()) {
        LogError("Cannot obtain the login form token.");
        return ResultCode.Failure;
    }

    local token = tokenNode.at(0).attr("value");
    if (token == "") {
        LogError("The login form token is empty.");
        return ResultCode.Failure;
    }

    nm.setUrl(BASE_URL + "/auth/login");
    nm.setReferer(BASE_URL + "/auth/login");
    nm.addPostField("_token", token);
    nm.addPostField("email", login);
    nm.addPostField("password", password);
    nm.setCurlOptionInt(52, 0); // Disable CURLOPT_FOLLOWLOCATION to inspect the login redirect.
    local requestSucceeded = nm.doPost("");
    nm.setCurlOptionInt(52, 1);

    if (!requestSucceeded) {
        LogError("The authentication request failed.");
        return ResultCode.Failure;
    }

    if (nm.responseCode() != 302) {
        LogError("Authentication failed for username '" + login + "'. Response code: " + nm.responseCode());
        return ResultCode.Failure;
    }

    local destinationUrl = nm.responseHeaderByName("Location");
    if (destinationUrl == BASE_URL + "/auth/login" || destinationUrl == "/auth/login") {
        LogError("Authentication failed for username '" + login + "'.");
        return ResultCode.Failure;
    }

    return ResultCode.Success;
}

function UploadFile(fileName, options) {
    if (!nm.doGet(BASE_URL + "/") || nm.responseCode() != 200) {
        LogError("Cannot open the upload page. Response code: " + nm.responseCode());
        return ResultCode.Failure;
    }

    local doc = Document(nm.responseBody());
    local tokenNode = doc.find("meta[name=\"csrf-token\"]");
    if (!tokenNode.length()) {
        LogError("Cannot obtain the CSRF token.");
        return ResultCode.Failure;
    }

    local csrfToken = tokenNode.at(0).attr("content");
    if (csrfToken == "") {
        LogError("The CSRF token is empty.");
        return ResultCode.Failure;
    }

    nm.setUrl(BASE_URL + "/upload/session");
    nm.setReferer(BASE_URL + "/");
    nm.addQueryHeader("X-CSRF-TOKEN", csrfToken);
    nm.addQueryHeader("X-Requested-With", "XMLHttpRequest");
    nm.addPostField("thumbnail_size", "2");
    nm.addPostField("content_type", "sfw");
    nm.addPostField("comments_enabled", "false");

    if (!nm.doPost("") || nm.responseCode() != 200) {
        LogError("Cannot create an upload session. Response code: " + nm.responseCode());
        return ResultCode.Failure;
    }

    local sessionResponse = ParseJSON(nm.responseBody());
    if (sessionResponse == null || !("data" in sessionResponse)) {
        LogError("The upload session response does not contain the data field.");
        return ResultCode.Failure;
    }

    local session = "";
    if ("session" in sessionResponse) {
        session = sessionResponse.session;
    }

    local remoteName = options.getTask().getFileTask().getDisplayName();
    if (remoteName == "") {
        remoteName = ExtractFileName(fileName);
    }

    nm.setUrl(BASE_URL + "/upload");
    nm.setReferer(BASE_URL + "/");
    nm.addQueryHeader("X-CSRF-TOKEN", csrfToken);
    nm.addQueryHeader("X-Requested-With", "XMLHttpRequest");
    nm.addPostField("data", sessionResponse.data);
    nm.addPostField("_token", csrfToken);
    nm.addPostFieldFile("files[0]", fileName, remoteName, GetFileMimeType(remoteName));

    if (!nm.doUploadMultipartData() || nm.responseCode() != 200) {
        LogError("The upload request failed. Response code: " + nm.responseCode());
        return ResultCode.Failure;
    }

    local uploadResponse = ParseJSON(nm.responseBody());
    local resultUrl = "";
    if (uploadResponse != null && "success" in uploadResponse) {
        resultUrl = uploadResponse.success;
    } else if (session != "") {
        resultUrl = BASE_URL + "/upload/complete?session=" + nm.urlEncode(session);
    }

    if (resultUrl == "") {
        LogError("The upload response contains neither a result URL nor a session identifier.");
        return ResultCode.Failure;
    }

    if (!nm.doGet(resultUrl) || nm.responseCode() != 200) {
        LogError("Cannot open the upload result page. Response code: " + nm.responseCode());
        return ResultCode.Failure;
    }

    local resultDocument = Document(nm.responseBody());
    local bbCodeNode = resultDocument.find("#bb-code");
    if (!bbCodeNode.length()) {
        LogError("Cannot find BBCode on the upload result page: " + resultUrl);
        return ResultCode.Failure;
    }

    local bbCode = bbCodeNode.at(0).text();
    local resultRegExp = CRegExp("\\[url=(.+?)\\]\\[img\\](.+?)\\[/img\\]", "mi");
    if (!resultRegExp.match(bbCode)) {
        LogError("Cannot extract the view and thumbnail URLs from the upload result page: " + resultUrl);
        return ResultCode.Failure;
    }

    local viewUrl = resultRegExp.getMatch(1);
    local thumbUrl = resultRegExp.getMatch(2);
    if (viewUrl == "" || thumbUrl == "") {
        LogError("The upload result contains an empty view or thumbnail URL.");
        return ResultCode.Failure;
    }

    options.setViewUrl(viewUrl);
    options.setThumbUrl(thumbUrl);
    return ResultCode.Success;
}
