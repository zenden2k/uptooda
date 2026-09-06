#pragma once

#include <gmock/gmock.h>

#include "../INetworkClient.h"

class MockINetworkClient : public INetworkClient {
 public:
  MOCK_METHOD2(addQueryParam,
      void(const std::string& name, const std::string& value));
  MOCK_METHOD4(addQueryParamFile,
      void(const std::string& name, const std::string& fileName, const std::string& displayName, const std::string& contentType));
  MOCK_METHOD2(addPostField,
      void(const std::string& name, const std::string& value));
  MOCK_METHOD4(addPostFieldFile,
      void(const std::string& name, const std::string& fileName, const std::string& displayName, const std::string& contentType));
  MOCK_METHOD2(addQueryHeader,
      void(const std::string& name, const std::string& value));
  MOCK_METHOD1(setUrl,
      void(const std::string& url));
  MOCK_METHOD1(doPost,
      bool(const std::string& data));
  MOCK_METHOD0(doUploadMultipartData,
      bool());
  MOCK_METHOD2(doUpload,
      bool(const std::string& fileName, const std::string &data));
  MOCK_METHOD1(doGet,
      bool(const std::string& url));
  MOCK_METHOD0(responseBody,
      std::string());
  MOCK_METHOD0(responseCode,
      int());
  MOCK_METHOD0(errorString,
      std::string());
  MOCK_METHOD1(setUserAgent,
      void(const std::string& userAgentStr));
  MOCK_METHOD0(responseHeaderText,
      std::string());
  MOCK_METHOD1(responseHeaderByName,
      std::string(const std::string& name));
  MOCK_METHOD2(responseHeaderByIndex,
      std::string(int index, std::string& name));
  MOCK_METHOD0(responseHeaderCount,
      int());
  MOCK_METHOD0(getCurlResultString,
      std::string());
  MOCK_METHOD2(setCurlOption,
      void(int option, const std::string &value));
  MOCK_METHOD2(setCurlOptionInt,
      void(int option, long value));
  MOCK_METHOD1(getCurlInfoString,
      std::string(int option));
  MOCK_METHOD1(getCurlInfoInt,
      int(int option));
  MOCK_METHOD1(getCurlInfoDouble,
      double(int option));
  MOCK_METHOD1(setMethod,
      void(const std::string &str));
  MOCK_METHOD3(setProxy,
      void(const std::string&, int, int));
  MOCK_METHOD2(setProxyUserPassword,
      void(const std::string &username, const std::string& password));
  MOCK_METHOD0(clearProxy,
      void());
  MOCK_METHOD1(setReferer,
      void(const std::string &str));
  MOCK_METHOD1(setOutputFile,
      void(const std::string &str));
  MOCK_METHOD1(setChunkOffset,
      void(int64_t offset));
  MOCK_METHOD1(setChunkSize,
      void(int64_t size));
  MOCK_METHOD0(getCurlResult,
      int());
  MOCK_METHOD0(getCurlHandle,
      CURL*());
  MOCK_METHOD1(setCurlShare,
      void(CurlShare* share));
  MOCK_METHOD1(setTimeout,
      void(uint32_t timeout));
  MOCK_METHOD1(setConnectionTimeout,
      void(uint32_t connection_timeout));
  MOCK_METHOD1(enableResponseCodeChecking,
      void(bool enable));
  MOCK_METHOD1(setErrorLogId,
      void(const std::string &str));
  MOCK_METHOD1(setProgressCallback,
      void(const ProgressCallback& func));
  MOCK_METHOD1(setTreatErrorsAsWarnings,
      void(bool treat));
  MOCK_METHOD1(setProxyProvider,
      void(ProxyProvider* provider));
  MOCK_METHOD1(setDebugger,
      void(std::shared_ptr<Debugger> debugger));
  MOCK_METHOD1(urlEncode,
      std::string(const std::string& url));
  MOCK_METHOD1(urlDecode,
      std::string(const std::string& url));
  MOCK_METHOD0(setUploadAction,
      void());
};
