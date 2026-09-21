#ifndef IU_CORE_DEFAULTPROXYPROVIDER_H
#define IU_CORE_DEFAULTPROXYPROVIDER_H

#include "atlheaders.h"
#include <Winhttp.h>

#include "Core/Utils/CoreTypes.h"
#include "Core/Network/INetworkClient.h"

class DefaultProxyProvider : public INetworkClient::ProxyProvider {
public:
    DefaultProxyProvider();
    ~DefaultProxyProvider();
    bool provideProxyForUrl(INetworkClient* client, const std::string& url) override;
protected:
    HINTERNET hInternet_;
    WINHTTP_CURRENT_USER_IE_PROXY_CONFIG myProxyConfig_;
    bool configObtained_;
    bool openWinHttpSession();
    void closeWinHttpSession();
    static std::string extractProxyForUrlFromList(const std::string& list, const std::string& url);
    bool obtainProxyConfig();
    static CString proxyForUrlErrorToString(DWORD errorCode);
    DISALLOW_COPY_AND_ASSIGN(DefaultProxyProvider);
};
#endif