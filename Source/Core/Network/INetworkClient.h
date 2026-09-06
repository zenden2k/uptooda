/*

Uptooda - free application for uploading images/files to the Internet

Copyright 2007-2025 Sergey Svistunov (zenden2k@gmail.com)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.

*/

#ifndef SRC_CORE_NETWORK_INETWORK_CLIENT_H_
#define SRC_CORE_NETWORK_INETWORK_CLIENT_H_

//
// NetworkClient interface
//
#include <string>
#include <memory>
#include <functional>

#include <curl/curl.h>
#include "Core/Utils/CoreTypes.h"

typedef void CURL;
class CurlShare;

class INetworkClient {
    public:
        virtual ~INetworkClient() = default;

        typedef std::function<int(INetworkClient*, int64_t, int64_t, int64_t, int64_t)> ProgressCallback;

        class ProxyProvider {
        public:
            virtual bool provideProxyForUrl(INetworkClient* client, const std::string& url) = 0;
            virtual ~ProxyProvider() = default;
        };

        class Logger {
        public:
            virtual void logNetworkError(bool error, const std::string & msg) = 0;
            virtual ~Logger() = default;
        };

        class Debugger {
        public:
            virtual int debugCallback(INetworkClient* client, curl_infotype type, char* data, size_t size) = 0;
            virtual bool isDebugEnabled() const = 0;
            virtual void configureClient(INetworkClient* client) = 0;
            virtual ~Debugger() = default;
        };

        class AbortedException : public std::runtime_error {
        public:
            explicit AbortedException(const std::string& msg) : std::runtime_error(msg) {}
            AbortedException(const AbortedException& ex) = default;
        };

        [[deprecated("Use addPostField() instead")]]
        virtual void addQueryParam(const std::string& name, const std::string& value){}
        [[deprecated("Use addPostFieldFile() instead")]]
        virtual void addQueryParamFile(const std::string& name, const std::string& fileName, const std::string& displayName, const std::string& contentType){};

        virtual void addPostField(const std::string& name, const std::string& value) { }
        virtual void addPostFieldFile(const std::string& name, const std::string& fileName, const std::string& displayName, const std::string& contentType) { };

        virtual void addQueryHeader(const std::string& name, const std::string& value) {};
        virtual void setUrl(const std::string& url){};
        virtual bool doPost(const std::string& data) { return false; }
        virtual bool doUploadMultipartData(){ return false; }
        virtual bool doUpload(const std::string& fileName, const std::string &data) { return false; }
        virtual bool doGet(const std::string &url){ return false; }
        virtual std::string responseBody() { return {}; };
        virtual int responseCode(){ return 0; }
        virtual std::string errorString(){ return {}; }
        virtual void setUserAgent(const std::string& userAgentStr) {}
        virtual std::string responseHeaderText() { return {}; }
        virtual std::string responseHeaderByName(const std::string& name){ return {}; }
        virtual std::string responseHeaderByIndex(int index, std::string& name){ return {}; }
        virtual int responseHeaderCount(){ return 0; }
        virtual std::string getCurlResultString(){ return {}; }
        virtual void setCurlOption(int option, const std::string &value){}
        virtual void setCurlOptionInt(int option, long value){}
        virtual std::string getCurlInfoString(int option){ return {}; }
        virtual int getCurlInfoInt(int option){ return 0; };
        virtual double getCurlInfoDouble(int option){ return 0.0; }
        virtual void setMethod(const std::string &str){};
        virtual void setProxy(const std::string &host, int port, int type){}
        virtual void setProxyUserPassword(const std::string &username, const std::string& password){}
        virtual void clearProxy(){};
        virtual void setReferer(const std::string &str){}
        virtual void setOutputFile(const std::string &str) {}
        virtual void setChunkOffset(int64_t offset) { }
        virtual void setChunkSize(int64_t size) { }
        virtual int getCurlResult(){ return 0; /* CURLE_OK */ }
        virtual CURL* getCurlHandle() { return nullptr;  }
        virtual void setCurlShare(CurlShare* share) {}
        virtual void setTimeout(uint32_t timeout){}
        virtual void setConnectionTimeout(uint32_t connection_timeout){}
        virtual void enableResponseCodeChecking(bool enable) {}
        virtual void setErrorLogId(const std::string &str){}
        virtual void setProgressCallback(const ProgressCallback& func){}
        virtual void setTreatErrorsAsWarnings(bool treat){}
        virtual void setProxyProvider(std::shared_ptr<ProxyProvider> provider){}
        virtual void setDebugger(std::shared_ptr<Debugger> debugger) {};
        virtual void setLogger(Logger* logger){}
        virtual std::string urlEncode(const std::string& str){ return {}; }
        virtual std::string urlDecode(const std::string& str){ return {}; }
        virtual void setMaxUploadSpeed(uint64_t speed){}
        virtual void setMaxDownloadSpeed(uint64_t speed) {}
        virtual void cleanupAfter() {}
        virtual void setUploadAction() {};
};

class INetworkClientFactory {
public:
    virtual std::unique_ptr<INetworkClient> create() = 0;
    virtual ~INetworkClientFactory() = default;
};

#endif
