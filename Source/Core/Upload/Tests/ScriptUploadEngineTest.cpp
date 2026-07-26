#include <gtest/gtest.h>

#include "Core/Upload/ServerSync.h"
#include "Core/Upload/FileUploadTask.h"
#include "Core/Upload/UrlShorteningTask.h"
#include "Core/Network/Tests/NetworkClientMock.h"
#include "Tests/TestHelpers.h"
#include "Core/Upload/ScriptUploadEngine.h"
#include "Core/Network/NetworkClientFactory.h"

using namespace ::testing;

template <typename Map>
bool map_compare(Map const &lhs, Map const &rhs) {
    // No predicate needed because there is operator== for pairs already.
    return lhs.size() == rhs.size()
        && std::equal(lhs.begin(), lhs.end(),
        rhs.begin());
}
class ScriptUploadEngineTest : public ::testing::Test {
public:
    ScriptUploadEngineTest() : constSizeFileName(TestHelpers::resolvePath("file_with_const_size.png")) {

    }
protected:
    const std::string constSizeFileName;
};

TEST_F(ScriptUploadEngineTest, doUpload)
{
    using ::testing::_;
    // NiceMock is used to ignore uninterested calls
    NiceMock<MockINetworkClient> networkClient;
    auto sync = std::make_shared<ServerSync>();
    std::string scriptFileName = TestHelpers::resolvePath("Scripts/upload_test_1.nut");
    ASSERT_TRUE(IuCoreUtils::FileExists(scriptFileName));

    ServerSettingsStruct serverSettings;
    CScriptUploadEngine engine(scriptFileName, sync, &serverSettings,
        std::make_shared<NetworkClientFactory>(), CAbstractUploadEngine::ErrorMessageCallback());
    ASSERT_TRUE(engine.isLoaded());
    // Prepare CUploadEngineData instance
    CUploadEngineData ued;
    ued.Name = "test server";
    ued.TypeMask = CUploadEngineData::TypeImageServer;

    ued.ImageUrlTemplate = "stub";
    ued.DownloadUrlTemplate = "stub";
    ued.ThumbUrlTemplate = "stub";
    engine.setUploadData(&ued);
    engine.setNetworkClient(&networkClient);

    std::string displayName = IuCoreUtils::ExtractFileName(constSizeFileName);

    ON_CALL(networkClient, doUploadMultipartData()).WillByDefault(Return(true));
    ON_CALL(networkClient, responseCode()).WillByDefault(Return(200));
    {
        testing::InSequence dummy;
        //EXPECT_CALL(networkClient, setReferer(action1.Referer));
        //EXPECT_CALL(networkClient, responseBody()).Times(AnyNumber());
        EXPECT_CALL(networkClient, setUrl("https://example.com/upload"));
        EXPECT_CALL(networkClient, addQueryParam("key", "somekey"));
        EXPECT_CALL(networkClient, addQueryParam("format", "json"));
        EXPECT_CALL(networkClient, addQueryParamFile("source", constSizeFileName, _, _));

        EXPECT_CALL(networkClient, doUploadMultipartData());
        const char jsonData[] = 
        "{ \"image\" : { "
        " \"url_viewer\": \"https://serv3.example.com/view/file_with_const_size.png\","
        " \"thumb\": {\"url\" : \"https://serv4.example.com/thumb/file_with_const_size.png\"},"
        " \"url\": \"https://serv2.example.com/file_with_const_size.png\"" 
        "}, \"status_code\": 200}";
        EXPECT_CALL(networkClient, responseBody()).WillOnce(Return(jsonData));
    }
    
    auto fileTask = std::make_shared<FileUploadTask>(constSizeFileName, displayName);
   
    engine.setServerSettings(&serverSettings);
    UploadParams uploadParams;
    uploadParams.thumbWidth = 160;
    uploadParams.thumbHeight = 120;
    int res = engine.processTask(fileTask, uploadParams);
    EXPECT_EQ(1, res);
    EXPECT_EQ("https://serv2.example.com/file_with_const_size.png", uploadParams.getDirectUrl());
    EXPECT_EQ("https://serv4.example.com/thumb/file_with_const_size.png", uploadParams.getThumbUrl());
    EXPECT_EQ("https://serv3.example.com/view/file_with_const_size.png", uploadParams.getViewUrl());
}

TEST_F(ScriptUploadEngineTest, getFolderList)
{
    using ::testing::_;
    // NiceMock is used to ignore uninterested calls
    NiceMock<MockINetworkClient> networkClient;
    auto sync = std::make_shared<ServerSync>();
    std::string scriptFileName = TestHelpers::resolvePath("Scripts/upload_test_1.nut");
    ASSERT_TRUE(IuCoreUtils::FileExists(scriptFileName));

    ServerSettingsStruct serverSettings;
    CScriptUploadEngine engine(scriptFileName, sync, &serverSettings,
        std::make_shared<NetworkClientFactory>(), CAbstractUploadEngine::ErrorMessageCallback());
    ASSERT_TRUE(engine.isLoaded());
    engine.setServerSettings(&serverSettings);

    // Prepare CUploadEngineData instance
    CUploadEngineData ued;
    ued.Name = "test server";
    ued.TypeMask = CUploadEngineData::TypeImageServer;

    ued.ImageUrlTemplate = "stub";
    ued.DownloadUrlTemplate = "stub";
    ued.ThumbUrlTemplate = "stub";
    engine.setUploadData(&ued);
    engine.setNetworkClient(&networkClient);
    CFolderList folders;
    int res = engine.getFolderList(folders);
    EXPECT_EQ(1, res);
    ASSERT_EQ(2, folders.GetCount());
    CFolderItem& folder1 = folders[0];
    EXPECT_EQ("123", folder1.getId());
    EXPECT_EQ("Test", folder1.getTitle());
    EXPECT_EQ("Long description", folder1.getSummary());
    EXPECT_EQ(1, folder1.getAccessType());
    EXPECT_EQ("https://example.com/album123", folder1.getViewUrl());
    CFolderItem& folder2 = folders[1];
    EXPECT_EQ("456", folder2.getId());
    EXPECT_EQ("Test2", folder2.getTitle());
    EXPECT_EQ("Long description2", folder2.getSummary());
    EXPECT_EQ(0, folder2.getAccessType());
    EXPECT_EQ("https://example.com/album456", folder2.getViewUrl());

    std::vector<std::string> types;
    res = engine.getAccessTypeList(types);
    EXPECT_EQ(1, res);
    ASSERT_EQ(3, types.size());
    EXPECT_EQ("Private", types[0]);
    EXPECT_EQ("For friends", types[1]);
    EXPECT_EQ("Public", types[2]);

    ParameterList params;
    res = engine.getServerParamList(params);
    EXPECT_EQ(1, res);

    /* std::map<std::string, std::string> expected = {
        { "Param1", "Description1" },
        { "Param2", "Description 2" },
        { "Param3", "Description 3"}
    };

    EXPECT_TRUE(map_compare(params, expected));*/
}

TEST_F(ScriptUploadEngineTest, shortenUrl)
{
    using ::testing::_;
    // NiceMock is used to ignore uninterested calls
    NiceMock<MockINetworkClient> networkClient;
    auto sync = std::make_shared<ServerSync>();
    std::string scriptFileName = TestHelpers::resolvePath("Scripts/upload_test_1.nut");
    ASSERT_TRUE(IuCoreUtils::FileExists(scriptFileName));

    ServerSettingsStruct serverSettings;
    CScriptUploadEngine engine(scriptFileName, sync,
        &serverSettings, std::make_shared<NetworkClientFactory>(), CAbstractUploadEngine::ErrorMessageCallback());
    ASSERT_TRUE(engine.isLoaded());
    // Prepare CUploadEngineData instance
    CUploadEngineData ued;
    ued.Name = "test server";
    ued.TypeMask = CUploadEngineData::TypeUrlShorteningServer;

    ued.ImageUrlTemplate = "stub";
    ued.DownloadUrlTemplate = "stub";
    ued.ThumbUrlTemplate = "stub";
    engine.setUploadData(&ued);
    engine.setNetworkClient(&networkClient);

    std::string longUrl = "http://some.site.com/long/long/long.html?hello=world";
    ON_CALL(networkClient, doPost(_)).WillByDefault(Return(true));
    ON_CALL(networkClient, responseCode()).WillByDefault(Return(200));
    {
        testing::InSequence dummy;
        //EXPECT_CALL(networkClient, setReferer(action1.Referer));
        //EXPECT_CALL(networkClient, responseBody()).Times(AnyNumber());
        EXPECT_CALL(networkClient, setUrl("https://example.com/shorten"));
        EXPECT_CALL(networkClient, addQueryHeader("Content-Type", "application/json"));
        EXPECT_CALL(networkClient, addQueryParam("url", longUrl));

        EXPECT_CALL(networkClient, doPost(""));
        const char jsonData[] =
            "{ \"id\": \"https://ex.am/plecom\"}";
        EXPECT_CALL(networkClient, responseBody()).WillOnce(Return(jsonData));
    }

    auto fileTask = std::make_shared<UrlShorteningTask>(longUrl);

    engine.setServerSettings(&serverSettings);
    UploadParams uploadParams;
    uploadParams.thumbWidth = 160;
    uploadParams.thumbHeight = 120;
    int res = engine.processTask(fileTask, uploadParams);
    EXPECT_EQ(1, res);
    EXPECT_EQ("https://ex.am/plecom", uploadParams.getDirectUrl());
}

TEST_F(ScriptUploadEngineTest, authenticateTest)
{
    using ::testing::_;
    // NiceMock is used to ignore uninterested calls
    NiceMock<MockINetworkClient> networkClient;
    auto sync = std::make_shared<ServerSync>();
    std::string scriptFileName = TestHelpers::resolvePath("Scripts/upload_test_2.nut");
    ASSERT_TRUE(IuCoreUtils::FileExists(scriptFileName));

    ServerSettingsStruct serverSettings;
    serverSettings.authData.DoAuth = true;
    serverSettings.authData.Login = "testuser";
    serverSettings.authData.Password = "testpassword";
    CScriptUploadEngine engine(scriptFileName, sync, &serverSettings,
        std::make_shared<NetworkClientFactory>(), CAbstractUploadEngine::ErrorMessageCallback());

    ASSERT_TRUE(engine.isLoaded());
    // Prepare CUploadEngineData instance
    CUploadEngineData ued;
    ued.Name = "test server";
    ued.TypeMask = CUploadEngineData::TypeImageServer;

    ued.ImageUrlTemplate = "stub";
    ued.DownloadUrlTemplate = "stub";
    ued.ThumbUrlTemplate = "stub";
    ued.NeedAuthorization = CUploadEngineData::naAvailable;
    engine.setUploadData(&ued);
    engine.setNetworkClient(&networkClient);

    std::string displayName = IuCoreUtils::ExtractFileName(constSizeFileName);

    ON_CALL(networkClient, doUploadMultipartData()).WillByDefault(Return(true));
    ON_CALL(networkClient, doPost(_)).WillByDefault(Return(true));
    ON_CALL(networkClient, responseCode()).WillByDefault(Return(200));
    const char jsonData[] =
        "{ \"image\" : { "
        " \"url_viewer\": \"https://serv3.example.com/view/file_with_const_size.png\","
        " \"thumb\": {\"url\" : \"https://serv4.example.com/thumb/file_with_const_size.png\"},"
        " \"url\": \"https://serv2.example.com/file_with_const_size.png\""
        "}, \"status_code\": 200}";
    ON_CALL(networkClient, responseBody()).WillByDefault(Return(jsonData));
    {
        testing::InSequence dummy;
        EXPECT_CALL(networkClient, setUrl("https://example.com/login"));
        EXPECT_CALL(networkClient, addQueryParam("login", "testuser"));
        EXPECT_CALL(networkClient, addQueryParam("password", "testpassword"));
        EXPECT_CALL(networkClient, doPost(""));

        EXPECT_CALL(networkClient, setUrl("https://example.com/refresh_token"));
        EXPECT_CALL(networkClient, doPost(""));

        //EXPECT_CALL(networkClient, setReferer(action1.Referer));
        //EXPECT_CALL(networkClient, responseBody()).Times(AnyNumber());
        EXPECT_CALL(networkClient, setUrl("https://example.com/upload"));
        EXPECT_CALL(networkClient, addQueryParamFile("source", constSizeFileName, _, _));

        EXPECT_CALL(networkClient, doUploadMultipartData());

        EXPECT_CALL(networkClient, setUrl("https://example.com/refresh_token"));
        EXPECT_CALL(networkClient, doPost(""));

        EXPECT_CALL(networkClient, setUrl("https://example.com/upload"));
        EXPECT_CALL(networkClient, addQueryParamFile("source", constSizeFileName, _, _));
        EXPECT_CALL(networkClient, doUploadMultipartData());
    }

    // Upload a file twice to ensure that authentication is performed just once
    auto fileTask = std::make_shared<FileUploadTask>(constSizeFileName, displayName);

    engine.setServerSettings(&serverSettings);
    UploadParams uploadParams;
    uploadParams.thumbWidth = 160;
    uploadParams.thumbHeight = 120;
    int res = engine.processTask(fileTask, uploadParams);
    EXPECT_EQ(1, res);
    EXPECT_EQ("https://serv2.example.com/file_with_const_size.png", uploadParams.getDirectUrl());
    EXPECT_EQ("https://serv4.example.com/thumb/file_with_const_size.png", uploadParams.getThumbUrl());
    EXPECT_EQ("https://serv3.example.com/view/file_with_const_size.png", uploadParams.getViewUrl());

    auto fileTask2 = std::make_shared<FileUploadTask>(constSizeFileName, displayName);
    int res2 = engine.processTask(fileTask, uploadParams);
    EXPECT_EQ(1, res2);
    EXPECT_EQ("https://serv2.example.com/file_with_const_size.png", uploadParams.getDirectUrl());
    EXPECT_EQ("https://serv4.example.com/thumb/file_with_const_size.png", uploadParams.getThumbUrl());
    EXPECT_EQ("https://serv3.example.com/view/file_with_const_size.png", uploadParams.getViewUrl());
}
