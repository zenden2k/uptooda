#include <cstring>
#include <fstream>

#include <curl/curl.h>
#include <gtest/gtest.h>
#include <json/json.h>

#include "Core/Network/NetworkClient.h"
#include "Tests/TestHelpers.h"

constexpr int SERVER_PORT = 5000;

class NetworkClientTest : public ::testing::Test {
public:
    void SetUp() override {
        GTEST_SKIP();
        serverAddress_ = "http://127.0.0.1:" + std::to_string(SERVER_PORT);
    }

    static std::string resolvePath(const std::string& relPath) {
        return TestHelpers::resolvePath(relPath);
    }

    void configureNetworkClient(NetworkClient& client) {
        //client.setProxy("127.0.0.1", 8888, CURLPROXY_HTTP);
    }
protected:
    std::string serverAddress_;
};

TEST_F(NetworkClientTest, Get) {
    NetworkClient nc;
    configureNetworkClient(nc);
    Json::Reader reader;
    {
        ASSERT_TRUE(nc.doGet(serverAddress_ + "/get_hello?name=John"));
        EXPECT_EQ(200, nc.responseCode());
        EXPECT_GT(nc.responseHeaderCount(), 0);
        EXPECT_EQ("application/json", nc.responseHeaderByName("Content-Type"));
        std::string headerName;
        EXPECT_FALSE(nc.responseHeaderByIndex(1, headerName).empty());
        EXPECT_FALSE(headerName.empty());
        EXPECT_FALSE(nc.responseHeaderText().empty());
        EXPECT_TRUE(nc.errorString().empty());
        Json::Value root;

        EXPECT_TRUE(reader.parse(nc.responseBody(), root, false));
        EXPECT_STREQ("John", root["hello"].asCString());
    }
    {
        nc.setMethod("GET");
        nc.addQueryHeader("X-Hello-World", "Hello");
        ASSERT_TRUE(nc.doGet(serverAddress_ + "/get_full?first=John&last=Smith"));
        EXPECT_EQ(200, nc.responseCode());
        Json::Value root2;

        EXPECT_TRUE(reader.parse(nc.responseBody(), root2, false));

        EXPECT_STREQ("John", root2["first"].asCString());
        EXPECT_STREQ("Smith", root2["last"].asCString());
        EXPECT_STREQ("Hello", root2["custom_header"].asCString());
    }
}

TEST_F(NetworkClientTest, Error) {
    NetworkClient nc;
    configureNetworkClient(nc);
    EXPECT_FALSE(nc.doGet("unknown_protocol://127.0.0.1/"));
    EXPECT_FALSE(nc.getCurlResultString().empty());

    ASSERT_TRUE(nc.doGet(serverAddress_ + "/not_existing"));
    EXPECT_EQ(404, nc.responseCode());
}

TEST_F(NetworkClientTest, Post) {
    NetworkClient nc;
    configureNetworkClient(nc);
    Json::Reader reader;

    {
        nc.setUrl(serverAddress_ + "/post");
        nc.addPostField("name", "John");
        ASSERT_TRUE(nc.doPost(""));
        Json::Value root;

        ASSERT_TRUE(reader.parse(nc.responseBody(), root, false));

        EXPECT_STREQ("John", root["hello"].asCString());
    }
    {
        Json::Value root;
        nc.doPost("name=Billy");
        ASSERT_TRUE(reader.parse(nc.responseBody(), root, false));
        EXPECT_STREQ("Billy", root["hello"].asCString());

        nc.setUrl(serverAddress_ + "/empty_post_response");
        nc.addPostField("name", "John");
        EXPECT_TRUE(nc.doPost(""));
        EXPECT_EQ("", nc.responseBody());
        EXPECT_EQ(204, nc.responseCode());
    }
}

TEST_F(NetworkClientTest, Put) {
    NetworkClient nc;
    configureNetworkClient(nc);
    Json::Value root;
    Json::Reader reader;

    nc.setUrl(serverAddress_ + "/put");
    nc.addQueryHeader("Content-Type", "application/json");
    nc.setMethod("PUT");
    bool res = nc.doUpload("", R"({"update": "Aurora"})");
    ASSERT_TRUE(res);
    ASSERT_TRUE(reader.parse(nc.responseBody(), root, false));
    EXPECT_STREQ("Aurora", root["update"].asCString());
}

TEST_F(NetworkClientTest, Upload) {
    NetworkClient nc;
    Json::Reader reader;
    configureNetworkClient(nc);
    {
        Json::Value root;

        nc.setMethod("PUT");
        nc.setUrl(serverAddress_ + "/upload");
        EXPECT_TRUE(nc.doUpload(resolvePath("webp-supported.webp"), ""));
        EXPECT_EQ(201, nc.responseCode());
        ASSERT_TRUE(reader.parse(nc.responseBody(), root, false));
        EXPECT_STREQ("f12d51ae11430d960899775f9627578b", root["hash"].asCString());
    }

    {
        Json::Value root;
        nc.setUrl(serverAddress_ + "/upload_multipart");
        nc.addPostField("name", "Bill");
        nc.addPostFieldFile("file", resolvePath("webp-supported.webp"), "display_name.webp", "image/webp");
        ASSERT_TRUE(nc.doUploadMultipartData());
        EXPECT_EQ(200, nc.responseCode());
        ASSERT_TRUE(reader.parse(nc.responseBody(), root, false));
        EXPECT_STREQ("f12d51ae11430d960899775f9627578b", root["hash"].asCString());
        EXPECT_STREQ("Bill", root["name"].asCString());
    }
}

TEST_F(NetworkClientTest, UploadUnicodePath) {
    Json::Reader reader;
    NetworkClient nc;
    configureNetworkClient(nc);

    std::string fileName = "\u0067\u0072\u0061\u0062\u005F\u044E\u043D\u0438\u043A\u043E\u0434\u005F\u4F60\u597D\u002E\u006A\u0070\u0067";
    {
        Json::Value root;

        nc.setMethod("PUT");
        nc.setUrl(serverAddress_ + "/upload");

        bool result = nc.doUpload(resolvePath(fileName), "");
        ASSERT_TRUE(result);
        EXPECT_EQ(201, nc.responseCode());
        ASSERT_TRUE(reader.parse(nc.responseBody(), root, false));
        EXPECT_STREQ("eae2675a29f1c6090d1ea64dca6ba031", root["hash"].asCString());
    }

    {
        Json::Value root;
        nc.setUrl(serverAddress_ + "/upload_multipart");
        nc.addPostFieldFile("file", resolvePath(fileName), "test_" + fileName, "");
        nc.addPostField("name", "John");

        ASSERT_TRUE(nc.doUploadMultipartData());
        EXPECT_EQ(200, nc.responseCode());
        ASSERT_TRUE(reader.parse(nc.responseBody(), root, false));
        EXPECT_STREQ("eae2675a29f1c6090d1ea64dca6ba031", root["hash"].asCString());
        EXPECT_EQ("test_" + fileName, root["filename"].asCString());
        EXPECT_STREQ("John", root["name"].asCString());
    }
}


TEST_F(NetworkClientTest, OutputToFile) {
    NetworkClient nc;
    configureNetworkClient(nc);
    Json::Reader reader;
    const char* fileName = "out.txt";
    {   std::remove(fileName);
    nc.setOutputFile(fileName);
    ASSERT_TRUE(nc.doGet(serverAddress_ + "/get_hello?name=John"));
    EXPECT_EQ(200, nc.responseCode());
    EXPECT_GT(nc.responseHeaderCount(), 0);
    EXPECT_EQ("application/json", nc.responseHeaderByName("Content-Type"));

    std::ifstream t("out.txt");
    ASSERT_TRUE(!!t);
    std::stringstream buffer;
    buffer << t.rdbuf();

    Json::Value root;

    EXPECT_TRUE(reader.parse(buffer.str(), root, false));
    EXPECT_STREQ("John", root["hello"].asCString());

    ASSERT_TRUE(nc.doGet(serverAddress_ + "/get_full?first=John&last=Smith"));
    EXPECT_EQ(200, nc.responseCode());
    }

    {
        std::remove(fileName);
        nc.setOutputFile("");
        ASSERT_TRUE(nc.doGet(serverAddress_ + "/get_hello?name=Elena"));
        EXPECT_EQ(200, nc.responseCode());
        EXPECT_GT(nc.responseHeaderCount(), 0);
        EXPECT_EQ("application/json", nc.responseHeaderByName("Content-Type"));

        Json::Value root;

        ASSERT_TRUE(reader.parse(nc.responseBody(), root, false));
        EXPECT_STREQ("Elena", root["hello"].asCString());

        ASSERT_TRUE(nc.doGet(serverAddress_ + "/get_full?first=John&last=Smith"));
        EXPECT_EQ(200, nc.responseCode());
    }
}

TEST_F(NetworkClientTest, CustomRequest) {
    NetworkClient nc;
    configureNetworkClient(nc);
    Json::Reader reader;

    {
        nc.setUrl(serverAddress_ + "/delete");
        nc.setMethod("DELETE");
        ASSERT_TRUE(nc.doPost(""));
        ASSERT_EQ(200, nc.responseCode());
    }
}

TEST_F(NetworkClientTest, UploadChunk) {
    NetworkClient nc;
    configureNetworkClient(nc);
    Json::Reader reader;
    configureNetworkClient(nc);
    {
        Json::Value root;

        nc.setMethod("PUT");
        nc.setUrl(serverAddress_ + "/upload");
        // Upload entire file
        nc.setChunkOffset(0);
        nc.setChunkSize(6812);
        EXPECT_TRUE(nc.doUpload(resolvePath("webp-supported.webp"), ""));
        EXPECT_EQ(201, nc.responseCode());
        ASSERT_TRUE(reader.parse(nc.responseBody(), root, false));
        EXPECT_STREQ("f12d51ae11430d960899775f9627578b", root["hash"].asCString());
    }

    {
        Json::Value root;

        nc.setMethod("PUT");
        nc.setUrl(serverAddress_ + "/upload");
        nc.setChunkOffset(1000);
        nc.setChunkSize(2000);
        EXPECT_TRUE(nc.doUpload(resolvePath("webp-supported.webp"), ""));
        EXPECT_EQ(201, nc.responseCode());
        ASSERT_TRUE(reader.parse(nc.responseBody(), root, false));
        EXPECT_STREQ("a60e4df55cdba901aa93860f21e7784e", root["hash"].asCString());
    }

    {
        Json::Value root;

        nc.setMethod("PUT");
        nc.setUrl(serverAddress_ + "/upload");
        // Upload entire file
        nc.setChunkOffset(-1);
        nc.setChunkSize(-1);
        EXPECT_TRUE(nc.doUpload(resolvePath("webp-supported.webp"), ""));
        EXPECT_EQ(201, nc.responseCode());
        ASSERT_TRUE(reader.parse(nc.responseBody(), root, false));
        EXPECT_STREQ("f12d51ae11430d960899775f9627578b", root["hash"].asCString());
    }
}

TEST_F(NetworkClientTest, UrlEncode) {
    NetworkClient nc;
    configureNetworkClient(nc);
    std::string res = nc.urlEncode("https://github.com/?test=true");
    EXPECT_EQ("https%3A%2F%2Fgithub.com%2F%3Ftest%3Dtrue", res);
    res = nc.urlEncode("");
    EXPECT_EQ("", res);
}

