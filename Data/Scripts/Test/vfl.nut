fileName <- "";
siteRegexp <- CRegExp("http://.*vfl.ru/", "i");
afterUploadRegexp <- CRegExp("index\\.sema?.*sa=kod", "");

opt <- null;
uploadResult <- 0;
function findUploadButton(el) {
	local childs = el.getChildren();
	if ( childs != null ) {
		for ( local i = 0 ; i < childs.len(); i ++ ) {
			local child = childs[i];
			//WriteLog("error", child.getTagName() );
			if ( (child.getTagName() == "A" || child.getTagName() == "a") && child.getAttribute("class") == "zagruzit" ) {
				return child;
			}
			local res = findUploadButton(child);
			if ( res != null ) {
				return res;
			}
		}
	}
	
	return null;
}

function findTextArea(el) {
	local childs = el.getChildren();
	if ( childs != null ) {
		for ( local i = 0 ; i < childs.len(); i ++ ) {
			local child = childs[i];
			WriteLog("error", "Searching for textarea:" + child.getTagName() );
			if ( (child.getTagName() == "TEXTAREA" || child.getTagName() == "textarea") ) {
				
				local text = child.getInnerText();
				WriteLog("error", "Found textarea:" +text );
			
				return text;
			}
			local res = findTextArea(child);
			if ( res != null ) {
				return res;
			}
		}
		
	}
}

function findLinks(document) {
	local thumbReg = CRegExp("\\[IMG\\](.+?)\\[\\/IMG\\]", "i");
	local directReg = CRegExp("^http:\\/\\/images\\.vfl\\.ru.+", "i");
	local thumbUrl = null;
	local directUrl = null;
	local textAreas = document.getElementsByTagName("TEXTAREA");
	if ( textAreas != null ) {
		//WriteLog("error", "textAreas.len()="+textAreas.len());
		for ( local i = 0; i < textAreas.len() ; i++ ) {
			local text = textAreas[i].getInnerText();
			//WriteLog("error", text);
			if ( !thumbUrl && thumbReg.match( text ) ) {
				thumbUrl = thumbReg.getMatch(0);
			}
			if ( !directUrl && directReg.match( text ) ) {
				directUrl = directReg.getMatch(0);
			}
		}
	} else {
		//WriteLog("error", "textAreas is null");
	}
	
	if ( directUrl ) {
		opt.setDirectUrl(directUrl);
		if ( thumbUrl ) {
			opt.setThumbUrl(thumbUrl);
		}
		uploadResult = 1;
		return true;
	}

	return false;
}

function OnLoadFinished(data) {
	local doc = data.browser.document();
	local rootEl = doc.rootElement();
	if ( afterUploadRegexp.match(data.url) ) {
		//WriteLog("warning", "afterUploadRegexp");
		if ( findLinks(doc) ) {
			uploadResult  = 1;
			data.browser.close();
		}
		return;
	} else if ( siteRegexp.match(data.url ) ) {
		data.browser.runJavaScript("tip_change(2)");
		//WriteLog("warning", "siteRegexp");
		local form = doc.getElementById("vflform");
		if ( !form.isNull() ) {
			local formElements = form.getFormElements();
			for ( local i = 0; i < formElements.len(); i++ ) {
				local name = formElements[i].getAttribute("name");
				local type = formElements[i].getAttribute("type");
				if ( type == "file"  ) {
					formElements[i].setValue(fileName);
					//break;
				}
				//print(name+"\r\n");
			}
		}
	}

}



function OnFileInputFilledCallback(data) {
	//data.browser.close();
	//WriteLog("error", "FILLED!!!! " + data.fileName);
	local doc = data.browser.document();
	local tip_zagr = doc.getElementById("tip_zagruzki_2");
	if ( !tip_zagr.isNull() ) {
		//WriteLog("error", "tip_zagruzki_2 found!");
		local button = findUploadButton(tip_zagr);
		if ( button!= null ) {
			//WriteLog("error", "button found!");
			button.click();
		}
	}
}

function OnTimer(data) {
	WriteLog("error", "Timer");
}

function OnNavigateErrorCallback(data) {
	if ( CRegExp("http://.*vfl.ru/$", "i").match(data.url) ) {
		data.browser.close();
	}
}

function  UploadFile(FileName, options)
{
	uploadResult = 0;
	opt = options;
	fileName = FileName;
	local webBrowser = CWebBrowser();
	if ( webBrowser.getMajorVersion() < 8 ) {
		WriteLog("error", "You need to install Internet Explorer 8 or later to upload to this server.");
		return -1;
	}
	webBrowser.setSilent(true);
	webBrowser.addTrustedSite("http://vfl.ru");
	//return -1;
	webBrowser.navigateToUrl("http://vfl.ru");
	webBrowser.setOnLoadFinishedCallback(OnLoadFinished, null);
	webBrowser.setOnFileInputFilledCallback(OnFileInputFilledCallback, null);
	webBrowser.setOnNavigateErrorCallback(OnNavigateErrorCallback, null);

	//webBrowser.exec();
	webBrowser.showModal();
	return uploadResult;
}
