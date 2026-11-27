
#include <gtest/gtest.h>

#include "Core/Images/ImageConverter.h"
#include "Core/Utils/CoreUtils.h"
#include "Func/GdiPlusInitializer.h"
#include "Tests/TestHelpers.h"
#include "3rdpart/GdiPlusH.h"
#include "Core/CommonDefs.h"
#include "atlheaders.h"

class ImageConverterTest : public ::testing::Test {
private:
    GdiPlusInitializer init;
public:
};

TEST_F(ImageConverterTest, Convert)
{
    {
        std::string fileName = TestHelpers::resolvePath("file_with_const_size.png");
        ImageConverter converter;
        converter.setGenerateThumb(false);
        ImageConvertingParams params;
        params.strNewWidth = "100";
        converter.setImageConvertingParams(params);
        bool res = converter.convert(fileName);
        EXPECT_TRUE(res);

        std::string outFileName = converter.getImageFileName();
        ASSERT_TRUE(IuCoreUtils::FileExists(outFileName));

        {
            Gdiplus::Image img(U2W(outFileName));
            ASSERT_TRUE(img.GetLastStatus() == Gdiplus::Ok);
            EXPECT_EQ(100, img.GetWidth());
            EXPECT_EQ(146, img.GetHeight());
        }
        ASSERT_TRUE(outFileName != fileName);
        EXPECT_TRUE(IuCoreUtils::RemoveFile(outFileName));
    }

    {
        std::string fileName = TestHelpers::resolvePath("Images/resize_test.jpg");
        ImageConverter converter;
        converter.setGenerateThumb(true);
        ThumbCreatingParams thumbParams;
        thumbParams.Width = 320; // Generate thumbnail with width = 320
        converter.setThumbCreatingParams(thumbParams);
        Thumbnail thumb;
        ASSERT_TRUE(thumb.loadFromFile(TestHelpers::resolvePath("classic.xml")));
        thumb.setParam("DrawFrame", 0);
        thumb.setParam("DrawText", 0);

        converter.setThumbnail(&thumb);
        ImageConvertingParams params;
        converter.setEnableProcessing(false);
        converter.setImageConvertingParams(params);
        bool res = converter.convert(fileName);
        EXPECT_TRUE(res);

        std::string outFileName = converter.getImageFileName();
        ASSERT_TRUE(outFileName == fileName);
        ASSERT_TRUE(IuCoreUtils::FileExists(outFileName));
        ASSERT_TRUE(IuCoreUtils::FileExists(converter.getThumbFileName()));

        {
            Gdiplus::Image img(U2W(converter.getThumbFileName()));
            ASSERT_TRUE(img.GetLastStatus() == Gdiplus::Ok);
            EXPECT_EQ(320, img.GetWidth());
            EXPECT_EQ(240, img.GetHeight());
        }

        EXPECT_TRUE(IuCoreUtils::RemoveFile(converter.getThumbFileName()));
    }

    {
        std::string fileName = TestHelpers::resolvePath("Images/resize_test.jpg");
        ImageConverter converter;
        converter.setGenerateThumb(true);
        ThumbCreatingParams thumbParams;
        thumbParams.Height = 240; // Generate thumbnail with height = 240
        thumbParams.ResizeMode = ThumbCreatingParams::trByHeight;
        converter.setThumbCreatingParams(thumbParams);
        Thumbnail thumb;
        ASSERT_TRUE(thumb.loadFromFile(TestHelpers::resolvePath("classic.xml")));
        thumb.setParam("DrawFrame", 0);
        thumb.setParam("DrawText", 0);

        converter.setThumbnail(&thumb);
        ImageConvertingParams params;
        params.strNewWidth = "320";
        converter.setEnableProcessing(true);
        converter.setImageConvertingParams(params);
        bool res = converter.convert(fileName);
        EXPECT_TRUE(res);

        std::string outFileName = converter.getImageFileName();
        ASSERT_TRUE(outFileName != fileName);
        ASSERT_TRUE(IuCoreUtils::FileExists(outFileName));
        ASSERT_TRUE(IuCoreUtils::FileExists(converter.getThumbFileName()));

        {
            Gdiplus::Image img(U2W(converter.getThumbFileName()));
            ASSERT_TRUE(img.GetLastStatus() == Gdiplus::Ok);
            EXPECT_EQ(320, img.GetWidth());
            EXPECT_EQ(240, img.GetHeight());
        }

        /*{
            Gdiplus::Image img(U2W(outFileName));
            ASSERT_TRUE(img.GetLastStatus() == Gdiplus::Ok);
            EXPECT_EQ(320, img.GetWidth());
            EXPECT_EQ(240, img.GetHeight());
        }*/
        if (outFileName != fileName) {
            EXPECT_TRUE(IuCoreUtils::RemoveFile(converter.getImageFileName()));
        }
        EXPECT_TRUE(IuCoreUtils::RemoveFile(converter.getThumbFileName()));
    }
}