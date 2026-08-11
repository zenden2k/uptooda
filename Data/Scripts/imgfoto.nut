// Chevereto
function UploadFile(fileName, options) {
    local apiKey = ServerParams.getParam("apiKey");
    if (apiKey == "") {
        WriteLog("error", "[imgfoto.host] API key not set!");
        return 0;
    } 
    local expiration = ServerParams.getParam("expiration");
    local name = ExtractFileName(fileName);
    local mime = GetFileMimeType(name);
    nm.setUrl("https://imgfoto.host/api/1/upload");
    nm.addQueryHeader("X-API-Key", apiKey);
    nm.addPostField("format", "json");
    nm.addPostField("expiration", expiration);
    nm.addPostFieldFile("source", fileName, name, mime);
    nm.doUploadMultipartData();

    if (nm.responseCode() > 0) {
        local sJSON = nm.responseBody();
        local t = ParseJSON(sJSON);
        if (t != null) {
            if ("status_code" in t && t.status_code == 200) {
                options.setViewUrl(t.image.url_viewer);
                options.setThumbUrl(t.image.thumb.url);
                options.setDirectUrl(t.image.url);
                return 1;
            } else {
                if ("error" in t && t.error != null) {
                    WriteLog("error", "[freeimage.host] " + t.error.message);
                }
            }
        }
    }

    return 0;
}

function GetServerParamList() {
	return {
		apiKey = "API key",
        expiration = {
            title = tr("chevereto.expiration", "Expiration"),
            type = "choice",
            items = [
                {
                    id = "",
                    label = tr("chevereto.never", "Never")
                },
                {
                    id = "PT5M",
                    label = "5 minutes"
                },
                {
                    id = "PT15M",
                    label = "15 minutes"
                },
                {
                    id = "PT30M",
                    label = "30 minutes"
                },
                {
                    id = "PT1H",
                    label = "1 hour"
                },
                {
                    id = "PT3H",
                    label = "3 hours"
                },
                {
                    id = "PT6H",
                    label = "6 hours"
                },
                {
                    id = "PT12H",
                    label = "12 hours"
                },
                {
                    id = "P1D",
                    label = "1 day"
                },
                {
                    id = "P2D",
                    label = "2 days"
                },
                {
                    id = "P3D",
                    label = "3 days"
                },
                {
                    id = "P4D",
                    label = "4 days"
                },
                {
                    id = "P5D",
                    label = "5 days"
                },
                {
                    id = "P6D",
                    label = "6 days"
                },
                {
                    id = "P1W",
                    label = "1 week"
                },
                {
                    id = "P2W",
                    label = "2 weeks"
                },
                {
                    id = "P3W",
                    label = "3 weeks"
                },
                {
                    id = "P1M",
                    label = "1 month"
                },
                {
                    id = "P2M",
                    label = "2 months"
                },
                {
                    id = "P3M",
                    label = "3 months"
                },
                {
                    id = "P4M",
                    label = "4 months"
                },
                {
                    id = "P5M",
                    label = "5 months"
                },
                {
                    id = "P6M",
                    label = "6 months"
                }
            ]
        } 
	}
} 