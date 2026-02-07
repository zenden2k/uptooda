function UploadFile(FileName, options) {
    local login = ServerParams.getParam("Login");
    local apiKey = ServerParams.getParam("apiKey");

    if (login != "" && apiKey == "") {
        WriteLog("error", "[postimages.org] You must set API key for uploading to account");
        return 0;
    }

    if (apiKey == "") {
        apiKey = "8ca0b57a6bb9c4c33cd9e7ab8e6a7f05";
    }
    nm.setUrl("https://api.postimage.org/1/upload");
    nm.setUserAgent("Mozilla/5.0 (compatible; Postimage/1.0.1; +http://postimage.org/app.php)");
    nm.addQueryParam("key", apiKey);
    nm.addQueryParam("o", md5(GetDeviceName())); // md5("Windows 8 64-bit")
    nm.addQueryParam("m", md5(GetDeviceId())); // md5("MAC1|MAC2|MAC3...") 
    nm.addQueryParam("version", "1.0.1");
    nm.addQueryParam("portable", "1");
    nm.addQueryParam("name", ExtractFileNameNoExt(FileName));
    nm.addQueryParam("type", GetFileExtension(FileName).tolower());
    nm.addQueryParam("image", Base64Encode(GetFileContents(FileName)));
    nm.doPost("");

    if (nm.responseCode() > 0) {
        local xml = SimpleXml();
        if (xml.LoadFromString(nm.responseBody())) {
            local root = xml.GetRoot("data", false);
            if (root.Attribute("success") == "1") {
                local links = root.GetChild("links", false);
                local pageLink = links.GetChild("page", false).Text();
                local hotLink = links.GetChild("hotlink", false).Text();
                local deleteLink =  links.GetChild("delete", false).Text();
                local thumbnailLink = links.GetChild("thumbnail", false).Text();

                if (pageLink != "") {
                    options.setViewUrl(pageLink);
                    options.setDirectUrl(hotLink);
                    options.setDeleteUrl(deleteLink);
                    options.setThumbUrl(thumbnailLink);
                    return 1;
                }
            }
        } 
    }

    return 0;
}

function GetServerParamList() {
    return {
        apiKey = "API key",
    };
}