function UploadFile(FileName, options) {
    nm.setUrl("https://picabox.ru/image/upload");
    local mimeType = GetFileMimeType(FileName);
    nm.addPostField("ImagesForm[text]", "");
    nm.addPostFieldFile("ImagesForm[imageFiles][]", FileName, ExtractFileName(FileName), mimeType);
    nm.doUploadMultipartData();
    if (nm.responseCode() == 200) {
        local data = nm.responseBody();
        if (data != "") {
            local js = ParseJSON(data);
            local img_view = js.loaded_pics[0].texts.url;
            local img_direct = js.loaded_pics[0].texts.fileName;
            options.setDirectUrl(img_direct);
            options.setViewUrl(img_view);
            return 1;
        }
    }
    return 0;
}