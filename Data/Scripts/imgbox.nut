const BASE_URL = "https://imgbox.com";
const CURLOPT_FOLLOWLOCATION = 52;

function _GetRequiredData() {
	nm.doGet(BASE_URL);
	local ret = {cookie="",token=""};
	local sBody = nm.responseBody();
	if (nm.responseCode() == 200) {
		local sCookie = nm.responseHeaderByName("Set-Cookie");
		ret["cookie"] = sCookie;
		local re = CRegExp("content=\"(.+)\" name=\"csrf-token\"", "gim");
		if (re.search(sBody)) {
			local s = re.getMatch(1);
			ret["token"] = s;
		}	
	}
	return ret;
}

function Authenticate() {
	local username = ServerParams.getParam("Login");
	local password = ServerParams.getParam("Password");

	local reqData = _GetRequiredData();
	local csrfToken = reqData["token"];

	if (csrfToken == "") {
		WriteLog("error", "[imgbox.com] Can not obtain CSRF token required by authentication process.");
		return ResultCode.Failure;
	}

	nm.setUrl(BASE_URL + "/login");
	nm.addQueryParam("utf8", "✓");
	nm.addQueryParam("authenticity_token", csrfToken);
	nm.addQueryParam("user[login]", username);
	nm.addQueryParam("user[password]", password);
	nm.setCurlOptionInt(CURLOPT_FOLLOWLOCATION, 0);
	nm.doPost("");
	nm.setCurlOptionInt(CURLOPT_FOLLOWLOCATION, 1);

	if (nm.responseCode() != 302) {
		local doc = Document(nm.responseBody());
		local errorMessage = doc.find(".alert-error").text();
		WriteLog("error", "[8upload.com] Login request failed. Response code: " + nm.responseCode() + "\n" + errorMessage);
		return ResultCode.Failure;
	}

	return ResultCode.Success;
}

function UploadFile(FileName, options) {
	local username = ServerParams.getParam("Login");
	local reqData = _GetRequiredData();

	local sImgboxCookie = reqData["cookie"];
	local sCSRFToken = reqData["token"];
	if (sImgboxCookie == "" || sCSRFToken == "") {
		WriteLog("error", "[imgbox.com] Can not obtain cookie and CSRF token required by uploading process.");
		return -1; //error, no cookie, no token :(
	}
	nm.setUrl(BASE_URL+"/ajax/token/generate");
	nm.addQueryHeader("Accept", "application/json, text/javascript, */*; q=0.01");
	nm.addQueryHeader("X-CSRF-Token", sCSRFToken);
	nm.addQueryHeader("X-Requested-With", "XMLHttpRequest");
	nm.addQueryHeader("Origin", "https://imgbox.com");
	/*if (username != "") {
		nm.addPostField("gallery", "true");
		nm.addPostField("gallery_title", "");
		nm.addPostField("comments_enabled", "1");
	}*/
	nm.setReferer("https://imgbox.com/");
	nm.doPost("");

	local sBody = nm.responseBody();
	if (nm.responseCode() != 200) {
		WriteLog("warning", "[imgbox.com] Server response code is "+nm.responseCode()+" at \"generate\" stage.");
		return ResultCode.Failure;
	}
	local json = ParseJSON(sBody);
	if (json == null) {
		WriteLog("error", "[imgbox.com] json cant be decoded at \"generate\" stage.");
		return ResultCode.Failure;
	}
	if (json.ok) {
		local sTokenId = json.token_id;
		local sTokenSecret = json.token_secret;
		local galleryId = "gallery_id" in json ?  json.gallery_id : "null";
		local gallerySecret = "gallery_secret" in json ? json.gallery_secret : "null";
		nm.setUrl(BASE_URL+"/upload/process");
		nm.addQueryHeader("X-CSRF-Token", sCSRFToken);
		nm.addQueryHeader("Cookie", "request_method=POST; "+sImgboxCookie);
		nm.addQueryHeader("X-Requested-With", "XMLHttpRequest");
		nm.addQueryHeader("Referer", "https://imgbox.com/");

		nm.addQueryParam("token_id", sTokenId);
		nm.addQueryParam("token_secret", sTokenSecret);
		nm.addQueryParam("content_type", "1");
		nm.addQueryParam("thumbnail_size", "200c");
		nm.addQueryParam("gallery_id", galleryId);
		nm.addQueryParam("gallery_secret", gallerySecret);
		nm.addQueryParam("comments_enabled", "0");
		
		nm.addQueryParamFile("files[]", FileName, ExtractFileName(FileName),"");
		nm.doUploadMultipartData();
		if (nm.responseCode() != 200) {
			WriteLog("warning", "[imgbox.com] Server response code is "+nm.responseCode()+" at \"upload\" stage.");
			return ResultCode.Failure;
		}
		
		local sBody = nm.responseBody();
		json = ParseJSON(sBody);
		if (json == null) {
			WriteLog("error", "[imgbox.com] json cant be decoded at \"upload\" stage.");
			return ResultCode.Failure;
		}

		if (!("files" in json )  || !("url" in json.files[0])) {
			WriteLog("error", "[imgbox.com] Could not obtain the links.");
			return ResultCode.Failure;
		}
		local sView = json.files[0].url;
		local sDirect = json.files[0].original_url;
		local sTrumb = json.files[0].thumbnail_url;
		
		options.setThumbUrl(sTrumb);
		options.setViewUrl(sView);
		options.setDirectUrl(sDirect);

		nm.doGet(sView);

		local editUrl = "";
		if (galleryId == "null") {
			editUrl = BASE_URL + "/upload/edit/" + sTokenId + "/" + sTokenSecret;
		}

		options.setEditUrl(editUrl);
		return ResultCode.Success;
	}	
}

/*

Also need gallery_secret for upload

function GetFolderList(list) {
	nm.addQueryHeader("X-Requested-With", "XMLHttpRequest");
	nm.addQueryHeader("Referer", BASE_URL + "/galleries");
	nm.doGet(BASE_URL + "/api/v1/galleries?orderby=title&page=1");

	local arr = ParseJSON(nm.responseBody());

	if (arr != null) {
		foreach (item in arr) {
			local folder = CFolderItem();
			folder.setId(item.id);
			folder.setTitle(item.name);
			folder.setParentId("/");
			list.AddFolderItem(folder);
		}
		return ResultCode.Success;
	} else if (nm.responseCode() != 200){
		WriteLog("error", "[imgbox.com] Failed to get folder list. Response code:" + nm.responseCode());
	}

	return ResultCode.Failure;
}*/
