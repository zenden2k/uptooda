#include "DefaultProxyProvider.h"

#include <algorithm>
#include <map>

#include <curl/curl.h>
#include <Winhttp.h>
#include "atlheaders.h"
#include "Core/CommonDefs.h"
#include "Core/Utils/CoreUtils.h"
#include "Core/Utils/StringUtils.h"
#include "Core/3rdpart/UriParser.h"
#include "Func/WinUtils.h"

#ifndef WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY
    #define WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY 4
#endif

DefaultProxyProvider::DefaultProxyProvider() {
    hInternet_ = nullptr;  
    configObtained_ = false;
    memset(&myProxyConfig_, 0, sizeof(myProxyConfig_));
}

DefaultProxyProvider::~DefaultProxyProvider() {
    closeWinHttpSession();

    if (myProxyConfig_.lpszAutoConfigUrl) {
        GlobalFree(myProxyConfig_.lpszAutoConfigUrl);
    }

    if (myProxyConfig_.lpszProxy) {
        GlobalFree(myProxyConfig_.lpszProxy);
    }

    if (myProxyConfig_.lpszProxyBypass) {
        GlobalFree(myProxyConfig_.lpszProxyBypass);
    }
}

bool DefaultProxyProvider::provideProxyForUrl(INetworkClient* client, const std::string& url) {
    bool autoProxy = false;
    bool result = false;
    WINHTTP_AUTOPROXY_OPTIONS autoProxyOptions;
    WINHTTP_PROXY_INFO proxyInfo;

    memset(&autoProxyOptions, 0, sizeof(autoProxyOptions));
    memset(&proxyInfo, 0, sizeof(proxyInfo));

    if (obtainProxyConfig()) {
        if (myProxyConfig_.fAutoDetect) {
            autoProxy = true;
        }
        if (myProxyConfig_.lpszAutoConfigUrl != nullptr) {
            autoProxy = TRUE;
            autoProxyOptions.lpszAutoConfigUrl = myProxyConfig_.lpszAutoConfigUrl;
        }

        if (autoProxy) {
            if (autoProxyOptions.lpszAutoConfigUrl != nullptr) {
                autoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_CONFIG_URL;
            } else {
                autoProxyOptions.dwFlags = WINHTTP_AUTOPROXY_AUTO_DETECT;
                autoProxyOptions.dwAutoDetectFlags = WINHTTP_AUTO_DETECT_TYPE_DHCP | WINHTTP_AUTO_DETECT_TYPE_DNS_A;
            }

            // basic flags you almost always want
            autoProxyOptions.fAutoLogonIfChallenged = FALSE;

            if (!hInternet_ && !openWinHttpSession()) {
                goto cleanup;
            }

            // here we reset fAutoProxy in case an auto-proxy isn't actually
            // configured for this url
            autoProxy = WinHttpGetProxyForUrl(hInternet_, U2W(url), &autoProxyOptions, &proxyInfo) != FALSE;
            if (!autoProxy) {
                if (ERROR_WINHTTP_LOGIN_FAILURE == GetLastError()) {
                    autoProxyOptions.fAutoLogonIfChallenged = TRUE;
                    autoProxy = WinHttpGetProxyForUrl(hInternet_, U2W(url), &autoProxyOptions, &proxyInfo) != FALSE;
                }
            
                if (!autoProxy) {
                    DWORD Err = GetLastError();
                    // If we got here because of RPC timeout during out of process PAC
                    // resolution, no further requests on this session are going to work.
                    if (ERROR_WINHTTP_TIMEOUT == Err || ERROR_WINHTTP_AUTO_PROXY_SERVICE_ERROR == Err) {
                        closeWinHttpSession();
                    }
                    LOG(WARNING) << "WinHttpGetProxyForUrl failed. Error code:" << Err << std::endl << proxyForUrlErrorToString(Err);
                }
            }
        } 

        if (autoProxy) {
            if (proxyInfo.lpszProxy) {
                //LOG(INFO) << "Auto proxy detected for url " << url << std::endl;
                std::string proxyList = W2U(proxyInfo.lpszProxy);
                std::string proxy = extractProxyForUrlFromList(proxyList, url);
                //LOG(INFO) << "Proxy: " << proxy << std::endl;
                curl_easy_setopt(client->getCurlHandle(), CURLOPT_PROXY, proxy.c_str());
                result = true;
            }
            if (proxyInfo.lpszProxyBypass) {
                std::string bypass = W2U(proxyInfo.lpszProxyBypass);
                std::replace(bypass.begin(), bypass.end(), ';', ',');
                std::replace(bypass.begin(), bypass.end(), ' ', ',');
                curl_easy_setopt(client->getCurlHandle(), CURLOPT_NOPROXY, bypass.c_str());
            }
        } else {
            // If autoproxy detection failed, we fallback to explicitly set proxies
            if (myProxyConfig_.lpszProxy != nullptr) {
                std::string proxyList = W2U(myProxyConfig_.lpszProxy);
                std::string proxy = extractProxyForUrlFromList(proxyList, url);
                if (!proxy.empty()) {
                    //LOG(INFO) << "Explicit proxy is being used " << std::endl;
                    curl_easy_setopt(client->getCurlHandle(), CURLOPT_PROXY, proxy.c_str());
                    //LOG(INFO) << "Proxy: " << proxy << std::endl;
                    result = true;
                }
                if (myProxyConfig_.lpszProxyBypass) {
                    std::string bypass = W2U(myProxyConfig_.lpszProxyBypass);
                    std::replace(bypass.begin(), bypass.end(), ';', ',');
                    curl_easy_setopt(client->getCurlHandle(), CURLOPT_NOPROXY, bypass.c_str());
                }
            } else {
                // there is no auto proxy and no manually configured proxy
                client->clearProxy();
            }
        }
    }

    
    cleanup:
    if (proxyInfo.lpszProxy) {
        GlobalFree(proxyInfo.lpszProxy);
    }
    if (proxyInfo.lpszProxyBypass) {
        GlobalFree(proxyInfo.lpszProxyBypass);
    }
    return result;
}

bool DefaultProxyProvider::openWinHttpSession() {
    hInternet_ = WinHttpOpen(APP_NAME, IsWindows8OrGreater() ? WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY : WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
        WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!hInternet_) {
        LOG(ERROR) << "Call to WinHttpOpen failed" << std::endl;
        return false;
    }
    WinHttpSetTimeouts(hInternet_, 10000, 10000, 5000, 5000);

    return true;
}

void DefaultProxyProvider::closeWinHttpSession() {
    if (hInternet_) {
        WinHttpCloseHandle(hInternet_);
    }
    hInternet_ = nullptr;
}

std::string DefaultProxyProvider::extractProxyForUrlFromList(const std::string& proxyList, const std::string& url) {
    uriparser::Uri uri(url);
    std::string uriProtocol = uri.scheme();
    if (uriProtocol.empty()) {
        uriProtocol = "http";
    }
    std::vector<std::string> tokens;
    IuStringUtils::Split(proxyList, "; ", tokens, 10);
    std::map<std::string, std::string> proxies;
    std::string defaultProxy;
    for (const auto& token : tokens) {
        std::string tokenTrimmed = IuStringUtils::Trim(token);
        if (!tokenTrimmed.empty()) {
            auto it = tokenTrimmed.find('=');
            if (it == std::string::npos) {
                proxies["http"] = tokenTrimmed;
                defaultProxy = tokenTrimmed;
            } else {
                std::string protocol = IuStringUtils::ToLower(tokenTrimmed.substr(0, it));
                proxies[protocol] = tokenTrimmed.substr(it + 1);
            }
        }
    }
    auto it2 = proxies.find(uriProtocol);
    std::string proxy;
    if (it2 != proxies.end()) {
        proxy = it2->first + "://" + it2->second;
    } else {
        auto it3 = proxies.find("socks");
        if (it3 != proxies.end()) {
            proxy = "socks4://" + it3->second;
        } else {
            proxy = defaultProxy;
        }
    }
    return proxy;
}

bool DefaultProxyProvider::obtainProxyConfig() {
    if (configObtained_) {
        return true;
    }
    memset(&myProxyConfig_, 0, sizeof(myProxyConfig_));
    if (!WinHttpGetIEProxyConfigForCurrentUser(&myProxyConfig_)) {
        DWORD Err = GetLastError();
        LOG(ERROR) << "WinHttpGetIEProxyConfigForCurrentUser failed. " << std::endl << WinUtils::ErrorCodeToString(Err);
        return false;
    }
    configObtained_ = true;
    return true;
}

CString DefaultProxyProvider::proxyForUrlErrorToString(DWORD errorCode) {
    return WinUtils::ErrorCodeToString(errorCode, GetModuleHandle(_T("winhttp.dll")));
}
