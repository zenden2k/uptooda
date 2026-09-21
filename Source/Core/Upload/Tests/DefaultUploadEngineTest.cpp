#include <gtest/gtest.h>
#include <json/json.h>

#include "Core/Upload/DefaultUploadEngine.h"
#include "Core/Upload/ServerSync.h"
#include "Core/Upload/FileUploadTask.h"
#include "Core/Upload/UrlShorteningTask.h"
#include "Core/Network/Tests/NetworkClientMock.h"
#include "Tests/TestHelpers.h"

using namespace ::testing;

class DefaultUploadEngineTest : public ::testing::Test {
public:
    DefaultUploadEngineTest() : constSizeFileName(TestHelpers::resolvePath("file_with_const_size.png")) {

    }
protected:
    const std::string constSizeFileName;
};

TEST_F(DefaultUploadEngineTest, doUpload)
{
    using ::testing::_;
    // NiceMock is used to ignore uninterested calls
    NiceMock<MockINetworkClient> networkClient;
    auto sync = std::make_shared<ServerSync>();
    CDefaultUploadEngine engine(sync, CAbstractUploadEngine::ErrorMessageCallback());
    engine.setNetworkClient(&networkClient);

    // Prepare CUploadEngineData instance
    CUploadEngineData ued;
    ued.Name = "test server";
    ued.TypeMask = CUploadEngineData::TypeImageServer;
    
    UploadAction action1;
    action1.Index = 0;
    action1.Type = "get";
    //action1.Referer = "http://referer.com/";
    action1.Url = "https://example.com";
    ued.Actions.push_back(action1);

    UploadAction action2;
    action2.Index = 1;
    action2.Type = "upload";
    action2.Url = "https://example.com/upload";
    action2.PostParams = "someparam=1;file=%filename%;thumbwidth=$(_THUMBWIDTH);thumbheight=$(_THUMBHEIGHT);"
        "rnd=$(_RAND16BITS);th=$(_THREADID);fname=$(_FILENAME);fnamenoext=$(_FILENAMEWITHOUTEXT);fileext=$(_FILEEXT);createthumb=$(_THUMBCREATE);"
        "thumbtext=$(_THUMBADDTEXT);serverthumbs=$(_THUMBUSESERVER);";
    ActionFunc regExp(ActionFunc::FUNC_REGEXP);
    regExp.setArg(1,"https://example\\.com/(.+)");
    regExp.Required = true;
    ActionVariable variable("fileId", 0);
    regExp.Variables.push_back(variable);
    action2.FunctionCalls.push_back(regExp);
    ued.Actions.push_back(action2);
    ued.ImageUrlTemplate = "https://serv2.example.com/$(fileId)";
    ued.DownloadUrlTemplate = "https://serv3.example.com/view/$(fileId)";
    ued.ThumbUrlTemplate = "https://serv4.example.com/thumb/$(fileId)";
    engine.setUploadData(&ued);

    std::string displayName = IuCoreUtils::ExtractFileName(constSizeFileName);
    std::string fileNameNoExt = IuCoreUtils::ExtractFileNameNoExt(constSizeFileName);
    std::string fileExt = IuCoreUtils::ExtractFileExt(constSizeFileName);

    ON_CALL(networkClient, doGet(_)).WillByDefault(Return(true));
    ON_CALL(networkClient, doUploadMultipartData()).WillByDefault(Return(true));
    ON_CALL(networkClient, responseCode()).WillByDefault(Return(200));
    {
        testing::InSequence dummy;
        //EXPECT_CALL(networkClient, setReferer(action1.Referer));
        EXPECT_CALL(networkClient, doGet(action1.Url));
        EXPECT_CALL(networkClient, responseBody()).Times(AnyNumber());
        EXPECT_CALL(networkClient, addPostField("someparam", "1"));
        EXPECT_CALL(networkClient, addPostFieldFile("file", constSizeFileName, _, _));
        EXPECT_CALL(networkClient, addPostField("thumbwidth", "160"));
        EXPECT_CALL(networkClient, addPostField("thumbheight", "120"));
        EXPECT_CALL(networkClient, addPostField("rnd", StrNe("")));
        EXPECT_CALL(networkClient, addPostField("th", StrNe("")));
        EXPECT_CALL(networkClient, addPostField("fname", displayName));
        EXPECT_CALL(networkClient, addPostField("fnamenoext", fileNameNoExt));
        EXPECT_CALL(networkClient, addPostField("fileext", fileExt));
        EXPECT_CALL(networkClient, addPostField("createthumb", "1"));
        EXPECT_CALL(networkClient, addPostField("thumbtext", "1"));
        EXPECT_CALL(networkClient, addPostField("serverthumbs", "0"));

        EXPECT_CALL(networkClient, doUploadMultipartData());
        EXPECT_CALL(networkClient, responseBody()).WillOnce(Return("https://example.com/file_with_const_size.png"));
    }
    
    auto fileTask = std::make_shared<FileUploadTask>(constSizeFileName, displayName);
    ServerSettingsStruct serverSettings;
    engine.setServerSettings(&serverSettings);
    UploadParams uploadParams;
    uploadParams.thumbWidth = 160;
    uploadParams.thumbHeight = 120;
    uploadParams.createThumbnail = true;
    uploadParams.addTextOnThumb = true;
    uploadParams.useServerSideThumbnail = false;
    int res = engine.processTask(fileTask, uploadParams);
    EXPECT_EQ(1, res);
    EXPECT_EQ("https://serv2.example.com/file_with_const_size.png", uploadParams.getDirectUrl());
    EXPECT_EQ("https://serv4.example.com/thumb/file_with_const_size.png", uploadParams.getThumbUrl());
    EXPECT_EQ("https://serv3.example.com/view/file_with_const_size.png", uploadParams.getViewUrl());
}

TEST_F(DefaultUploadEngineTest, login)
{
    using ::testing::_;
    // NiceMock is used to ignore uninterested calls
    NiceMock<MockINetworkClient> networkClient;
    auto sync = std::make_shared<ServerSync>();
    CDefaultUploadEngine engine(sync, CAbstractUploadEngine::ErrorMessageCallback());
    engine.setNetworkClient(&networkClient);

    // Prepare CUploadEngineData instance
    CUploadEngineData ued;
    ued.NeedAuthorization = CUploadEngineData::naAvailable;
    ued.Name = "test server2";
    ued.TypeMask = CUploadEngineData::TypeImageServer;

    UploadAction action1;
    action1.Index = 0;
    action1.Type = "login";
    action1.Url = "https://example.com/login";
    action1.Referer = "https://test.com/somepage";
    action1.PostParams = "password=$(_PASSWORD);login=$(_LOGIN);submit=1";

    ActionFunc regExp{ ActionFunc::FUNC_REGEXP };
    regExp.setArg(1, "success");
    regExp.Required = true;
    action1.FunctionCalls.push_back(regExp);

    ued.Actions.push_back(action1);

    ued.ImageUrlTemplate = "https://serv2.example.com/$(directUrl)";
    ued.DownloadUrlTemplate = "https://serv3.example.com/view/$(directUrl)";
    ued.ThumbUrlTemplate = "https://serv4.example.com/thumb/$(directUrl)";
    engine.setUploadData(&ued);

    ON_CALL(networkClient, doGet(_)).WillByDefault(Return(true));
    ON_CALL(networkClient, doPost(_)).WillByDefault(Return(true));
    ON_CALL(networkClient, responseCode()).WillByDefault(Return(200));
    //{
        //testing::InSequence dummy;
        EXPECT_CALL(networkClient, setReferer(action1.Referer)).Times(AtLeast(1));
        EXPECT_CALL(networkClient, addPostField("password", "qwerty"));
        EXPECT_CALL(networkClient, addPostField("login", "username"));
        EXPECT_CALL(networkClient, addPostField("submit", "1"));
        EXPECT_CALL(networkClient, doPost("")).WillOnce(Return(true));
        EXPECT_CALL(networkClient, responseBody()).WillOnce(Return("<result>success</result>"));
        EXPECT_CALL(networkClient, setReferer("")).Times(AtLeast(1));
    //}
    std::string displayName = IuCoreUtils::ExtractFileName(constSizeFileName);
    auto fileTask = std::make_shared<FileUploadTask>(constSizeFileName, displayName);
    ServerSettingsStruct serverSettings;
    serverSettings.authData.DoAuth = true;
    serverSettings.authData.Login = "username";
    serverSettings.authData.Password = "qwerty";

    engine.setServerSettings(&serverSettings);
    UploadParams uploadParams;
    int res = engine.processTask(fileTask, uploadParams);
    EXPECT_EQ(1, res);
}

TEST_F(DefaultUploadEngineTest, shortenUrl)
{
    using ::testing::_;
    // NiceMock is used to ignore uninterested calls
    NiceMock<MockINetworkClient> networkClient;
    auto sync = std::make_shared<ServerSync>();
    CDefaultUploadEngine engine(sync, CAbstractUploadEngine::ErrorMessageCallback());
    engine.setNetworkClient(&networkClient);

    // Prepare CUploadEngineData instance
    CUploadEngineData ued;
    ued.NeedAuthorization = CUploadEngineData::naNotAvailable;
    ued.Name = "test server3";
    ued.TypeMask = CUploadEngineData::TypeUrlShorteningServer;

    UploadAction action1;
    action1.Index = 0;
    action1.Type = "post";
    action1.Url = "https://example.com/shorten?url=$(_ORIGINALURL|urlencode)";
    action1.Referer = "https://test.com/somepage";
    action1.PostParams = "url_second_time=$(_ORIGINALURL);submit=1;";

    ActionFunc regExp{ ActionFunc::FUNC_REGEXP };
    regExp.setArg(1, "<url>(.+)</url>");
    regExp.Required = true;
    ActionVariable variable("url", 0);
    regExp.Variables.push_back(variable);
    action1.FunctionCalls.push_back(regExp);
    ued.Actions.push_back(action1);

    ued.ImageUrlTemplate = "$(url)";

    engine.setUploadData(&ued);
    std::string urlToShorten = "http://example.com/hello?someparam=1";
    ON_CALL(networkClient, doGet(_)).WillByDefault(Return(true));
    ON_CALL(networkClient, doPost(_)).WillByDefault(Return(true));
    ON_CALL(networkClient, responseCode()).WillByDefault(Return(200));
    ON_CALL(networkClient, responseBody()).WillByDefault(Return("<url>http://te.st/qwe1234</url>"));
    ON_CALL(networkClient, urlEncode(_)).WillByDefault(Return("http%3A%2F%2Fexample.com%2Fhello%3Fsomeparam%3D1"));

    EXPECT_CALL(networkClient, setReferer(action1.Referer));
    EXPECT_CALL(networkClient, setReferer(""));
    EXPECT_CALL(networkClient, setUrl("https://example.com/shorten?url=http%3A%2F%2Fexample.com%2Fhello%3Fsomeparam%3D1"));
    {
        testing::InSequence dummy;
        EXPECT_CALL(networkClient, addPostField("url_second_time", urlToShorten));
        EXPECT_CALL(networkClient, addPostField("submit", "1"));
        EXPECT_CALL(networkClient, doPost("")).WillOnce(Return(true)); 
    }

    auto fileTask = std::make_shared<UrlShorteningTask>(urlToShorten);
    ServerSettingsStruct serverSettings;
    engine.setServerSettings(&serverSettings);
    UploadParams uploadParams;
    int res = engine.processTask(fileTask, uploadParams);
    EXPECT_EQ(1, res);
    EXPECT_EQ("http://te.st/qwe1234", uploadParams.getDirectUrl());
    EXPECT_EQ("", uploadParams.getThumbUrl());
    EXPECT_EQ("", uploadParams.getViewUrl());
}

TEST_F(DefaultUploadEngineTest, json)
{
    using ::testing::_;
    // NiceMock is used to ignore uninterested calls
    NiceMock<MockINetworkClient> networkClient;
    auto sync = std::make_shared<ServerSync>();
    CDefaultUploadEngine engine(sync, CAbstractUploadEngine::ErrorMessageCallback());
    engine.setNetworkClient(&networkClient);

    // Prepare CUploadEngineData instance
    CUploadEngineData ued;
    ued.NeedAuthorization = CUploadEngineData::naNotAvailable;
    ued.Name = "test server3";
    ued.TypeMask = CUploadEngineData::TypeUrlShorteningServer;

    UploadAction action1;
    action1.Index = 0;
    action1.Type = "post";
    action1.Url = "https://example.com/shorten?url=$(_ORIGINALURL|urlencode)";
    action1.Referer = "https://test.com/somepage";
    action1.PostParams = "url_second_time=$(_ORIGINALURL);submit=1;";

    ActionFunc regExp{ ActionFunc::FUNC_JSON };
    regExp.setArg(1, "files[0].url");
    regExp.Required = true;
    ActionVariable variable("url", 0);
    regExp.Variables.push_back(variable);
    action1.FunctionCalls.push_back(regExp);
    ued.Actions.push_back(action1);

    ued.ImageUrlTemplate = "$(url)";

    engine.setUploadData(&ued);
    std::string urlToShorten = "http://example.com/hello?someparam=1";
    ON_CALL(networkClient, doGet(_)).WillByDefault(Return(true));
    ON_CALL(networkClient, doPost(_)).WillByDefault(Return(true));
    ON_CALL(networkClient, responseCode()).WillByDefault(Return(200));
    ON_CALL(networkClient, responseBody()).WillByDefault(Return(R"({
    "success": true,
    "files": [
        {
            "hash": "605edc801b4599ff2390153fb959254c4ecb53fd",
            "name": "screenshot 2024-01-14 16-44-15 001.png",
            "url": "https:\/\/ab.cd\/mxqo.png",
            "size": "1575"
        }
    ]
})"));
    ON_CALL(networkClient, urlEncode(_)).WillByDefault(Return("http%3A%2F%2Fexample.com%2Fhello%3Fsomeparam%3D1"));

    EXPECT_CALL(networkClient, setReferer(action1.Referer));
    EXPECT_CALL(networkClient, setReferer(""));
    EXPECT_CALL(networkClient, setUrl("https://example.com/shorten?url=http%3A%2F%2Fexample.com%2Fhello%3Fsomeparam%3D1"));
    {
        testing::InSequence dummy;
        EXPECT_CALL(networkClient, addPostField("url_second_time", urlToShorten));
        EXPECT_CALL(networkClient, addPostField("submit", "1"));
        EXPECT_CALL(networkClient, doPost("")).WillOnce(Return(true));
    }

    auto fileTask = std::make_shared<UrlShorteningTask>(urlToShorten);
    ServerSettingsStruct serverSettings;
    engine.setServerSettings(&serverSettings);
    UploadParams uploadParams;
    int res = engine.processTask(fileTask, uploadParams);
    EXPECT_EQ(1, res);
    EXPECT_EQ("https://ab.cd/mxqo.png", uploadParams.getDirectUrl());
    EXPECT_EQ("", uploadParams.getThumbUrl());
    EXPECT_EQ("", uploadParams.getViewUrl());
}
