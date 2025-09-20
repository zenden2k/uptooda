const BASE_URL = "https://imgbox.com";

function getRequiredData() {
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

function UploadFile(FileName, options) {
	local reqData = getRequiredData();
	local sImgboxCookie = reqData["cookie"];
	local sCSRFToken = reqData["token"];
	if (sImgboxCookie == "" || sCSRFToken == "") {
		WriteLog("error", "Can not obtain cookie and token valuest, required by uploading process.");
		return -1; //error, no cookie, no token :(
	}
	nm.setUrl(BASE_URL+"/ajax/token/generate");
	nm.addQueryHeader("Accept", "application/json, text/javascript, */*; q=0.01");
	nm.addQueryHeader("X-CSRF-Token", sCSRFToken);
	//nm.addQueryHeader("Cookie", "request_method=GET; "+sImgboxCookie);
	nm.addQueryHeader("X-Requested-With", "XMLHttpRequest");
	nm.addQueryHeader("Origin", "https://imgbox.com");
	nm.setReferer("https://imgbox.com/");
	nm.doPost("");
	local sBody = nm.responseBody();
	if (nm.responseCode() != 200) {
		WriteLog("warning", "Server response code is "+nm.responseCode()+" at \"generate\" stage.");
		return 0; //try again later
	}
	local json = ParseJSON(sBody);
	if (json == null) {
		WriteLog("error", "json cant be decoded at \"generate\" stage.");
		return -1; //internal json parser fail
	}
	if (json.ok) {
		local sTokenId = json.token_id;
		local sTokenSecret = json.token_secret;
		nm.setUrl(BASE_URL+"/upload/process");
		nm.addQueryHeader("X-CSRF-Token", sCSRFToken);
		nm.addQueryHeader("Cookie", "request_method=POST; "+sImgboxCookie);
		nm.addQueryHeader("X-Requested-With", "XMLHttpRequest");
		nm.addQueryHeader("Referer", "https://imgbox.com/");
		
		
		nm.addQueryParam("token_id", sTokenId);
		nm.addQueryParam("token_secret", sTokenSecret);
		nm.addQueryParam("content_type", "1");
		nm.addQueryParam("thumbnail_size", "200c");
		nm.addQueryParam("gallery_id", "null");
		nm.addQueryParam("gallery_secret", "null");
		nm.addQueryParam("comments_enabled", "0");
		
		nm.addQueryParamFile("files[]", FileName, ExtractFileName(FileName),"");
		nm.doUploadMultipartData();
		if (nm.responseCode() != 200) {
			WriteLog("warning", "Server response code is "+nm.responseCode()+" at \"upload\" stage.");
			return 0; //try again later
		}
		
		local sBody = nm.responseBody();
		json = ParseJSON(sBody);
		if (json == null) {
			WriteLog("error", "json cant be decoded at \"upload\" stage.");
			return -1; //internal json parser fail
		}
		local sView = json.files[0].original_url;
		local sDirect = json.files[0].original_url;
		local sTrumb = json.files[0].thumbnail_url;
		
		options.setThumbUrl(sTrumb);
		options.setViewUrl(sView);
		options.setDirectUrl(sDirect);
		return 1;	
	}	
}
