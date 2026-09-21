clientId <- "62e3f09c348d5696c9805ef1f200d5663b026283";
clientSecret <- "8acf539a452aae12f0d8156140f9cc3d05bd7cf6";
redirectUri <- "https://svistunov.dev/blank.html";;
redirectUrlEscaped <- "https:\\/\\/svistunov\\.dev\\/blank\\.html";
code <- "";
login <- "";
//Docs: https://dev.bitly.com/get_started.html

function IsAuthenticated() {
    if (ServerParams.getParam("token") != "") {
        return 1;
    }
    return 0;
}

function OnUrlChangedCallback(data) {
    local reg = CRegExp("^" +redirectUrlEscaped, "");
    if ( reg.match(data.url) ) {
        local br = data.browser;
        local regError = CRegExp("error=([^&]+)", "");
        if ( regError.match(data.url) ) {
            WriteLog("warning", regError.getMatch(1));
        } else {
            local regToken = CRegExp("code=([^&]+)", "");
            if ( regToken.match(data.url) ) {
                code = regToken.getMatch(1);
            }
        }
        br.close();
    }
}

function Authenticate() {
    local token = ServerParams.getParam("token");

    if ( token != ""){
        return 1;
    }

    local browser = CWebBrowser();
    browser.setTitle("Bit.ly authorization");
    browser.setOnUrlChangedCallback(OnUrlChangedCallback, null);

    local url = "https://bitly.com/oauth/authorize?" +
            "client_id=" + clientId  +
            "&redirect_uri=" + nm.urlEncode(redirectUri);

    browser.navigateToUrl(url);
    browser.showModal();

    if (code != ""){
        nm.setUrl("https://api-ssl.bitly.com/oauth/access_token");
        nm.addPostField("client_id", clientId);
        nm.addPostField("client_secret", clientSecret);
        nm.addPostField("code", code);
        nm.addPostField("redirect_uri", redirectUri);
        nm.addQueryHeader("Accept", "application/json");
        nm.doPost("");
        if (nm.responseCode() == 200){
            local t = ParseJSON(nm.responseBody());
            token = t.access_token ;
            ServerParams.setParam("token", token);
            login = t.login;
            ServerParams.setParam("actual_login", login);
            return 1;
        }
    }
    return 0;
}

function ShortenUrl(url, options) {
    local token = ServerParams.getParam("token");
    nm.setUrl("https://api-ssl.bitly.com/v4/shorten");
    nm.addQueryHeader("Content-Type", "application/json");
    nm.addQueryHeader("Authorization", "Bearer " + token );
    local req = {
        group_guid = "",
        domain = "bit.ly",
        long_url = url
    };
    nm.doPost(ToJSON(req));
    if (nm.responseCode() == 200 || nm.responseCode() == 201) {
        local t = ParseJSON(nm.responseBody());
        if ("link" in t) {
             options.setDirectUrl(t.link);
             return 1;
        }
    }

    return 0;
}

function GetServerParamList() {
    return {
        token = "Token"
    };
}