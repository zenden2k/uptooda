// This test script is used in ScriptUploadEngineTest
function GetFolderList(list)
{
    local album = CFolderItem();
	album.setId("123");
	album.setTitle("Test");
	album.setSummary("Long description");
    album.setAccessType(1);
	album.setViewUrl("https://example.com/album123");
	list.AddFolderItem(album);
    
    local album2 = CFolderItem();
	album2.setId("456");
	album2.setTitle("Test2");
	album2.setSummary("Long description2");
    album2.setAccessType(0);
	album2.setViewUrl("https://example.com/album456");
    list.AddFolderItem(album2);
	return 1;
}

function UploadFile(FileName, options) {
	local name = ExtractFileName(FileName);
	local mime = GetFileMimeType(name);
	nm.setUrl("https://example.com/upload");
    nm.addQueryParam("key", "somekey");
    nm.addQueryParam("format", "json");
    nm.addQueryParamFile("source", FileName, name, mime);
	nm.doUploadMultipartData();
    
    if (nm.responseCode() > 0) {       
        local sJSON = nm.responseBody();
        local t = ParseJSON(sJSON);
        if (t != null) {
            if ("status_code" in t && t.status_code == 200){
                options.setViewUrl(t.image.url_viewer );
                options.setThumbUrl(t.image.thumb.url);
                options.setDirectUrl(t.image.url);
                return 1;
            } else {
                if("error" in t && t.error!=null) {
                    WriteLog("error", "Upload Test 1: " + t.error.message);
                }
            }
        } 
    }
    
    return 0;
}


function GetFolderAccessTypeList()
{
	return [
		"Private",
        "For friends",
        "Public"
	];
}
function GetServerParamList()
{
	local a =
	{
        Param1 = "Description1",
        Param2 = {
            title = "Description 2",
            type = "text",
            required = false
        },
        Param3 = {
            title = "Description 3",
            type = "boolean",
            required = true
        }
	}
	return a;
}

function  ShortenUrl(url, options)
{	
	nm.setUrl("https://example.com/shorten");
	nm.addQueryHeader("Content-Type", "application/json");

	nm.addQueryParam("url", url);
	nm.doPost("");


	local id = ParseJSON(nm.responseBody()).id;

	options.setDirectUrl(id);
	options.setViewUrl(id);
	return 1;
}