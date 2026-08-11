function regex_simple(data,regStr,start)
{
	local ex = regexp(regStr);
	local res = ex.capture(data, start);
	local resultStr = "";
	if(res != null){	
		resultStr = data.slice(res[1].begin, res[1].end);
	}
	return resultStr;
}

function UploadFile(FileName, options)
{
    print("hello");
/*
	nm.setUrl("http://localhost/ImageUploader/Web/upload.php");
	nm.addPostFieldFile("file",FileName, ExtractFileName(FileName),"");
	local data = "";
	nm.doUploadMultipartData();
	data = nm.responseBody();

	local directUrl = regex_simple(data,"\\[url=(.+)\\]",0);
	local thumbUrl = regex_simple(data,"\\[img\\](.+)\\[/img\\]",0);

	options.setDirectUrl(directUrl);
	options.setThumbUrl(thumbUrl);
	
	if ( directUrl != "" ) {	
		return 1;
	} else {
		return 0;
	}*/
    
}

function CreateFolder(parentAlbum,album)
{
	WriteLog("error", "Folder created! " + album.getTitle());
    album.setId("cool_id");
    return 1;
}