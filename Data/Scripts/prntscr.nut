const CURLOPT_CONNECTTIMEOUT = 78;

function UploadFile(fileName, options) {
    local task = options.getTask().getFileTask();
    
    nm.setReferer("https://prnt.sc/");
    nm.addQueryHeader("Origin", "https://prnt.sc");
    nm.setUrl("https://prntscr.com/upload.php");
    nm.addPostFieldFile("image", fileName, task.getDisplayName(), GetFileMimeType(fileName));
    nm.doUploadMultipartData();
    local t = ParseJSON(nm.responseBody());
    
    if (nm.responseCode() == 200) {
        if (t != null) {
			if (t.status == "success") {
				local viewUrl = t.data;
				if (viewUrl != "") {
					nm.setCurlOptionInt(CURLOPT_CONNECTTIMEOUT, 5);
					nm.doGet(viewUrl);
					nm.setCurlOptionInt(CURLOPT_CONNECTTIMEOUT, 30);
					if (nm.responseCode() == 200) {
						local doc = Document(nm.responseBody());
						local directUrl = doc.find("#screenshot-image").attr("src");
						if (directUrl != "") {
							options.setDirectUrl(directUrl);
						} else {
							WriteLog("warning", "[prntscr.com] Failed to find the direct link on page");
						}
					} else {
						WriteLog("warning", "[prntscr.com] Failed to obtain the direct link, response code: " + nm.responseCode());
					}
					options.setViewUrl(viewUrl);
					return ResultCode.Success;
				}
			}
        }
        WriteLog("error", "[prntscr.com] Server response does not contain required data.");
    } else {
		WriteLog("error", "[prntscr.com] Failed to upload, response code: " + nm.responseCode());
    }      
    
    return 0;
}