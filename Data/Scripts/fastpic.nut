function UploadFile(FileName, options) {
    local thumbUseServerText = (options.getParam("THUMBCREATE") == "1" && options.getParam("THUMBADDTEXT") == "1" && options.getParam("THUMBUSESERVER") == "1");

    nm.setUrl("https://fastpic.org/upload?api=1");
    //nm.addQueryHeader("User-Agent","FPUploader");
    nm.addPostField("method", "file");
    nm.addPostFieldFile("file1", FileName, ExtractFileName(FileName), "");
    nm.addPostField("check_thumb", thumbUseServerText ? "size" : "no");
    nm.addPostField("uploading", "1");
    nm.addPostField("orig_rotate", "0");
    nm.addPostField("thumb_size", options.getParam("THUMBWIDTH"));
    nm.doUploadMultipartData();

    if (nm.responseCode() == 200) {
        local xml = SimpleXml();
        xml.LoadFromString(nm.responseBody());
        local root = xml.GetRoot("UploadSettings", false);

        if (!root.IsNull()) {
            local statusNode = root.GetChild("status", false);
            if (!statusNode.IsNull()) {
                if(statusNode.Text()=="ok"){
                    local imgUrlNode = root.GetChild("imagepath", false);
                    local thumbUrlNode = root.GetChild("thumbpath", false);
                    local viewUrl = root.GetChild("viewfullurl", false);
                    options.setDirectUrl(imgUrlNode.Text());
                    options.setThumbUrl(thumbUrlNode.Text());
                    local viewUrlStr = viewUrl.Text();
                    viewUrlStr = StrReplace(viewUrlStr, "fastpic.org/view/", "fastpic.org/fullview/");
                    options.setViewUrl(viewUrlStr);
                    return 1;
                } else {
                    local errorNode = root.GetChild("error", false);
                    if (!errorNode.IsNull()){
                        WriteLog("error", "Fastpic.ru error: "+errorNode.Text());
                    }
                }
            }
        }
    }

    return 0;
}
