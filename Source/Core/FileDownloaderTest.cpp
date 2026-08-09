
#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include "Core/FileDownloader.h"
#include "Core/Network/Tests/NetworkClientMock.h"

class FileDownloaderTest : public ::testing::Test {
public:
    FileDownloaderTest() {
    }
protected:
};

class MockNetworkClientFactory: public INetworkClientFactory {

public:
    // Use legacy workarounds for move-only types
    // https://github.com/abseil/googletest/blob/master/googlemock/docs/CookBook.md#legacy-workarounds-for-move-only-types
    MOCK_METHOD0(doCreate, INetworkClient*());

    std::unique_ptr<INetworkClient> create() override {
        return std::unique_ptr<INetworkClient>(doCreate());
    }
};

TEST_F(FileDownloaderTest, Download)
{
    using ::testing::AnyNumber;
    using ::testing::Return;
    using ::testing::_;
    std::string url = "http://example.com/test.png";
    std::string referer = "http://example2.com/";
    auto callback = [&]() {
        auto client = new MockINetworkClient();
        EXPECT_CALL(*client, doGet(url));
        EXPECT_CALL(*client, setOutputFile(_));
        EXPECT_CALL(*client, setReferer(referer));
        // Ignore unexpected calls
        ON_CALL(*client, responseCode()).WillByDefault(Return(200));
        EXPECT_CALL(*client, setProgressCallback(_)).Times(AnyNumber());
        EXPECT_CALL(*client, responseCode()).Times(AnyNumber());
        return client;
    };

    auto factory = std::make_shared<MockNetworkClientFactory>();
    ON_CALL(*factory, doCreate()).WillByDefault(testing::Invoke(callback));
    EXPECT_CALL(*factory, doCreate());
    CFileDownloader downloader(factory, std::string(), false);
    int fileFinishedCallbackCalls = 0;
    downloader.setOnFileFinishedCallback([&](bool ok, int responseCode, const CFileDownloader::DownloadFileListItem& item)
    {
        fileFinishedCallbackCalls++;
        EXPECT_TRUE(ok);
        EXPECT_EQ(url, item.url);
        EXPECT_EQ("test.png", item.displayName);
        EXPECT_EQ(123, reinterpret_cast<size_t>(item.id));
        EXPECT_EQ(referer, item.referer);
        EXPECT_EQ(200, responseCode);
        EXPECT_FALSE(item.fileName.empty());
        return true;
    });
    downloader.addFile(url, reinterpret_cast<void*>(123), referer);
    downloader.start();
    downloader.waitForFinished();
    EXPECT_EQ(1, fileFinishedCallbackCalls);
}

