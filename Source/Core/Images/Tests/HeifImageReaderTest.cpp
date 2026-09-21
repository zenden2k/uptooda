
#include <gtest/gtest.h>

#include "Core/Images/HeifImageReader.h"
#include "Core/Images/GdiPlusImage.h"
#include "Core/Utils/CoreUtils.h"
#include "Func/GdiPlusInitializer.h"
#include "Core/Images/Utils.h"
#include "Tests/TestHelpers.h"
#include "atlheaders.h"

class HeifImageReaderTest : public ::testing::Test {
private:
    GdiPlusInitializer init;
public:
};

TEST_F(HeifImageReaderTest, ReadFromFile) {
    {
        std::string fileName = TestHelpers::resolvePath("Images/heic-640.heic");
        HeifImageReader reader;

        auto img = reader.readFromFile(IuCoreUtils::Utf8ToWstring(fileName).c_str());
        ASSERT_TRUE(!!img);

        EXPECT_EQ(640, img->getWidth());
        EXPECT_EQ(428, img->getHeight());
    }
    {
        std::string fileName = TestHelpers::resolvePath("Images/avif.avif");
        HeifImageReader reader;

        auto img = reader.readFromFile(IuCoreUtils::Utf8ToWstring(fileName).c_str());
        ASSERT_TRUE(!!img);

        EXPECT_EQ(980, img->getWidth());
        EXPECT_EQ(981, img->getHeight());
    }
}

TEST_F(HeifImageReaderTest, ReadFromMemory) {
    {
        std::string fileName = TestHelpers::resolvePath("Images/heic-640.heic");
       
        auto [dataPtr, size] = ImageUtils::ExUtilReadFile(IuCoreUtils::Utf8ToWstring(fileName).c_str());
       
        ASSERT_TRUE(!!dataPtr);

        HeifImageReader reader;
        auto img = reader.readFromMemory(dataPtr.get(), size);

        ASSERT_TRUE(!!img);

        EXPECT_EQ(640, img->getWidth());
        EXPECT_EQ(428, img->getHeight());
    }
    {
        constexpr auto size = 256;
        uint8_t data[size]{};

        HeifImageReader reader;
        auto img = reader.readFromMemory(data, size);

        ASSERT_TRUE(!img);
        EXPECT_FALSE(reader.getLastError().empty());
    }
}

TEST_F(HeifImageReaderTest, ReadFromStream) {
    {
        std::string fileName = TestHelpers::resolvePath("Images/heic-640.heic");

        auto [dataPtr, size] = ImageUtils::ExUtilReadFile(IuCoreUtils::Utf8ToWstring(fileName).c_str());

        ASSERT_TRUE(!!dataPtr);
        IStream* stream = SHCreateMemStream(dataPtr.get(), size);
        ASSERT_TRUE(stream != nullptr);
        HeifImageReader reader;
        auto img = reader.readFromStream(stream);

        ASSERT_TRUE(!!img);

        EXPECT_EQ(640, img->getWidth());
        EXPECT_EQ(428, img->getHeight());
        stream->Release();
    }
}
