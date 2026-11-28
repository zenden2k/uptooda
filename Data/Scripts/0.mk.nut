function ShortenUrl(url, options) {
    local apiToken = ServerParams.getParam("apiToken");
    if (apiToken == "") {
        WriteLog("error", "[0.mk] Set the API token in the server settings before shortening URLs.");
        return ResultCode.Failure;
    }

    nm.setUrl("https://0.mk/api/links");
    nm.addQueryHeader("Content-Type", "application/json");
    nm.addQueryHeader("Authorization", "Bearer " + apiToken);
    if (!nm.doPost(ToJSON({ url = url })) || nm.responseCode() < 200 || nm.responseCode() >= 300) {
        WriteLog("error", "[0.mk] URL shortening failed. Response code: " + nm.responseCode());
        return ResultCode.Failure;
    }

    local response = ParseJSON(nm.responseBody());
    if (typeof response != "table" || !("shortUrl" in response) || typeof response.shortUrl != "string" || response.shortUrl == "") {
        WriteLog("error", "[0.mk] The response is missing the short URL.");
        return ResultCode.Failure;
    }

    options.setDirectUrl(response.shortUrl);
    return ResultCode.Success;
}

function GetServerParamList() {
    return {
        apiToken = { title = "API token", required = true }
    };
}
