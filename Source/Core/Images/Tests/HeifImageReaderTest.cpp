
#include <gtest/gtest.h>

#include "Core/Images/GdiPlusImage.h"
#include "Core/Images/HeifImageReader.h"
#include "Core/Images/ImageLoader.h"
#include "Core/Images/Utils.h"
#include "Core/Utils/CoreUtils.h"
#include "Func/GdiPlusInitializer.h"
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
        uint8_t data[size] { };

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

TEST_F(HeifImageReaderTest, ReadAnimatedAvifWithoutPrimaryImage) {
    const auto fileName = IuCoreUtils::Utf8ToWstring(TestHelpers::resolvePath("Images/animated-no-primary.avif"));
    ImageLoader loader;
    auto fileImage = loader.loadFromFile(fileName.c_str());
    ASSERT_TRUE(fileImage) << IuCoreUtils::WstringToUtf8(loader.getLastError());
    EXPECT_EQ(480, fileImage->getWidth());
    EXPECT_EQ(270, fileImage->getHeight());
    EXPECT_TRUE(fileImage->isSrcMultiFrame());
    EXPECT_EQ(1u, fileImage->frameCount());

    auto [data, size] = ImageUtils::ExUtilReadFile(fileName.c_str());
    ASSERT_TRUE(data);
    auto memoryImage = loader.loadFromMemory(data.get(), size, ImageFrameMode::ALL_FRAMES);
    ASSERT_TRUE(memoryImage) << IuCoreUtils::WstringToUtf8(loader.getLastError());
    EXPECT_EQ(fileImage->getWidth(), memoryImage->getWidth());
    EXPECT_EQ(fileImage->getHeight(), memoryImage->getHeight());
    EXPECT_TRUE(memoryImage->isSrcMultiFrame());
    ASSERT_EQ(2u, memoryImage->frameCount());
    EXPECT_EQ(480u, memoryImage->frameBitmap(1)->GetWidth());
    EXPECT_EQ(270u, memoryImage->frameBitmap(1)->GetHeight());
    EXPECT_EQ(fileImage->frameDurationMs(0), memoryImage->frameDurationMs(0));
    EXPECT_EQ(1000u, memoryImage->frameDurationMs(0));
    EXPECT_EQ(1000u, memoryImage->frameDurationMs(1));

    CComPtr<IStream> stream;
    stream.Attach(SHCreateMemStream(data.get(), static_cast<UINT>(size)));
    ASSERT_TRUE(stream);
    HeifImageReader reader(ImageFrameMode::ALL_FRAMES);
    auto streamImage = reader.readFromStream(stream);
    ASSERT_TRUE(streamImage) << IuCoreUtils::WstringToUtf8(reader.getLastError());
    EXPECT_EQ(fileImage->getWidth(), streamImage->getWidth());
    EXPECT_EQ(fileImage->getHeight(), streamImage->getHeight());
    EXPECT_TRUE(streamImage->isSrcMultiFrame());
    EXPECT_EQ(memoryImage->frameCount(), streamImage->frameCount());
}

TEST_F(HeifImageReaderTest, DecodeOnlyOneCycleOfRepeatingAvif) {
    const auto fileName = IuCoreUtils::Utf8ToWstring(TestHelpers::resolvePath("Images/animated-no-primary.avif"));
    auto [data, size] = ImageUtils::ExUtilReadFile(fileName.c_str());
    ASSERT_TRUE(data);

    // Double the movie duration so the fixture's repeat edit plays the two-second sequence twice.
    constexpr size_t MOVIE_DURATION_OFFSET = 325;
    ASSERT_GT(size, MOVIE_DURATION_OFFSET + 3);
    ASSERT_EQ(0x07, data[MOVIE_DURATION_OFFSET + 2]);
    ASSERT_EQ(0xd0, data[MOVIE_DURATION_OFFSET + 3]);
    data[MOVIE_DURATION_OFFSET + 2] = 0x0f;
    data[MOVIE_DURATION_OFFSET + 3] = 0xa0;

    ImageLoader loader;
    auto image = loader.loadFromMemory(data.get(), size, ImageFrameMode::ALL_FRAMES);
    ASSERT_TRUE(image) << IuCoreUtils::WstringToUtf8(loader.getLastError());
    EXPECT_EQ(2u, image->frameCount());
}
