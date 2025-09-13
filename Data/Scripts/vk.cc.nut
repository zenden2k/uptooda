clientId <- "4851603";
apiVersion <- "5.131";
serviceKey <- "d909ba70d909ba70d909ba70ccd943bde3dd909d909ba70825007ca7ecade18db9543b1";

function ShortenUrl(url, options) {
    nm.addQueryParam("url", url);
    nm.addQueryParam("private", "0");
    nm.setUrl("https://api.vk.ru/method/utils.getShortLink?v=" + apiVersion + "&access_token=" + serviceKey);
    nm.doPost("");

    if (nm.responseCode() == 200) {
        local data = ParseJSON(nm.responseBody());

        if ("error" in data) { // if data.error exists
            WriteLog("error", "vk.cc: " + data.error.error_msg);
        } else {
            if ( "response" in data && "short_url" in data.response) {
                local shortUrl = data.response.short_url;
                if ( shortUrl != "" ) {
                    options.setDirectUrl(shortUrl);
                    return 1;
                }
            }
        }

    }
	return 0;
}

