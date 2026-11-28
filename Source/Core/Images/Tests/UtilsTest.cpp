#include <gtest/gtest.h>



#include "Core/TempFileDeleter.h"
#include "Core/Images/ImageLoader.h"
#include "Core/Images/Utils.h"
#include "Core/Utils/CoreUtils.h"
#include "Func/GdiPlusInitializer.h"
#include "Core/Utils/CryptoUtils.h"
#include "Core/Utils/IOException.h"
#include "Tests/TestHelpers.h"

using namespace ImageUtils;

class UtilsTest : public ::testing::Test {
private:
    GdiPlusInitializer init;
public:

    void waitFormatAvailable() {
        bool isClipboardOpened = OpenClipboard(0) != FALSE;
        ASSERT_TRUE(isClipboardOpened);
        DWORD startTime = ::GetTickCount();
        while (IsClipboardFormatAvailable(CF_UNICODETEXT) == FALSE) {
            if (::GetTickCount() - startTime > 1000) {
                ASSERT_TRUE(false);
                break;
            }
        }
        CloseClipboard();
    }
};

TEST_F(UtilsTest, LoadAnimatedGifAndWebp) {
    struct TestImage {
        const char* Path;
        size_t FrameCount;
        uint64_t FirstFrameDurationMs;
    };
    const TestImage images[] = {
        { "Images/animation.gif", 10, 150 },
        { "Images/animated-webp-supported.webp", 12, 70 },
    };
    for (const auto& [path, frameCount, firstFrameDurationMs] : images) {
        SCOPED_TRACE(path);
        const auto fileName = IuCoreUtils::Utf8ToWstring(TestHelpers::resolvePath(path));
        ImageLoader loader;
        auto first = loader.loadFromFile(fileName.c_str(), ImageFrameMode::FIRST_FRAME);
        ASSERT_TRUE(first);
        EXPECT_TRUE(first->isSrcMultiFrame());
        EXPECT_EQ(1u, first->frameCount());

        auto [data, size] = ExUtilReadFile(fileName.c_str());
        ASSERT_TRUE(data);
        auto all = loader.loadFromMemory(data.get(), size, ImageFrameMode::ALL_FRAMES);
        ASSERT_TRUE(all);
        ASSERT_EQ(frameCount, all->frameCount());
        EXPECT_EQ(first->getBitmap()->GetWidth(), all->frameBitmap(1)->GetWidth());
        EXPECT_EQ(first->getBitmap()->GetHeight(), all->frameBitmap(1)->GetHeight());
        EXPECT_NE(all->frameBitmap(0), all->frameBitmap(1));
        EXPECT_EQ(first->frameDurationMs(0), all->frameDurationMs(0));
        EXPECT_EQ(firstFrameDurationMs, all->frameDurationMs(0));
    }
}

TEST_F(UtilsTest, BitmapFromMemory)
{
    std::string fileName = TestHelpers::resolvePath("file_with_const_size.png");
    int fileSize = static_cast<int>(IuCoreUtils::GetFileSize(fileName));
    ASSERT_GT(fileSize, 0);
    auto data = std::make_unique<uint8_t[]>(fileSize);
    FILE *f = IuCoreUtils::FopenUtf8(fileName.c_str(), "rb");
    size_t bytesRead = fread(data.get(), 1, fileSize, f);
    ASSERT_EQ(fileSize, bytesRead);
    fclose(f);
    auto bm = BitmapFromMemory(data.get(), fileSize);
    ASSERT_TRUE(bm != nullptr);
    EXPECT_EQ(366, bm->GetHeight());
    EXPECT_EQ(251, bm->GetWidth());
    Gdiplus::Color color;
    Gdiplus::Status status = bm->GetPixel(10, 10, &color);
    EXPECT_EQ(Gdiplus::Ok, status);
    EXPECT_EQ(16777215, color.GetValue());
}

TEST_F(UtilsTest, GetThumbnail) {
    std::string fileName = TestHelpers::resolvePath("file_with_const_size.png");
    ASSERT_TRUE(IuCoreUtils::FileExists(fileName));
    int fileSize = static_cast<int>(IuCoreUtils::GetFileSize(fileName));
    ASSERT_GT(fileSize, 0);
    auto data = std::make_unique<uint8_t[]>(fileSize);
    FILE *f = IuCoreUtils::FopenUtf8(fileName.c_str(), "rb");
    size_t bytesRead = fread(data.get(), 1, fileSize, f);
    ASSERT_EQ(fileSize, bytesRead);
    fclose(f);
    auto bm = BitmapFromMemory(data.get(), fileSize);
    ASSERT_TRUE(bm != nullptr);
    Gdiplus::Size size;
    auto thumb = GetThumbnail(bm.get(), 120, 100, &size);
    ASSERT_TRUE(thumb != nullptr);
    EXPECT_LE(thumb->GetWidth(), 120u);
    EXPECT_EQ(100, thumb->GetHeight());
    EXPECT_EQ(251, size.Width);
    EXPECT_EQ(366, size.Height);
    thumb = nullptr;

    auto thumb2 = GetThumbnail(bm.get(), 251, 366, &size);
    ASSERT_TRUE(thumb2 != nullptr);

    EXPECT_EQ(251, thumb2->GetWidth());
    EXPECT_EQ(366, thumb2->GetHeight());

    {
        auto thumb3 = GetThumbnail(U2W(TestHelpers::resolvePath("Images/exif-rgb-thumbnail.jpg")), 150, 120, &size);
        ASSERT_TRUE(thumb3 != nullptr);
        EXPECT_EQ(293, size.Width);
        EXPECT_EQ(233, size.Height);
        EXPECT_LE(thumb3->GetWidth(), 150u);
        EXPECT_LE(thumb3->GetHeight(), 120u);
        Gdiplus::Color color;
        thumb3->GetPixel(0, 0, &color);
        EXPECT_EQ(3637300478, color.GetValue());
    }

    {
        auto thumb4 = GetThumbnail(U2W(TestHelpers::resolvePath("Images/poroshok.webp")), 150, 120, &size);
        ASSERT_TRUE(thumb4 != nullptr);
        EXPECT_EQ(800, size.Width);
        EXPECT_EQ(322, size.Height);
        EXPECT_LE(thumb4->GetWidth(), 150u);
        EXPECT_LE(thumb4->GetHeight(), 120u);
        /*Gdiplus::Color color;
        thumb4->GetPixel(0, 0, &color);
        EXPECT_EQ(1485865579, color.GetValue());*/
    }

     {
         auto thumb4 = GetThumbnail(U2W(TestHelpers::resolvePath("Images/animated-webp-supported.webp")), 150, 120, &size);
         ASSERT_TRUE(thumb4 != nullptr);
         EXPECT_EQ(400, size.Width);
         EXPECT_EQ(400, size.Height);
         EXPECT_LE(thumb4->GetWidth(), 150u);
         EXPECT_LE(thumb4->GetHeight(), 120u);
         Gdiplus::Color color;
         thumb4->GetPixel(0, 0, &color);
         EXPECT_EQ(1811951973, color.GetValue());
     }

     for (int i = 1; i <= 8; i++ ) {
         std::string fname = "Images/Landscape_" + std::to_string(i) + ".jpg";
         auto thumb5 = GetThumbnail(U2W(TestHelpers::resolvePath(fname)), 150, 120, &size);
         ASSERT_TRUE(thumb5 != nullptr);
         EXPECT_EQ(1800, size.Width);
         EXPECT_EQ(1200, size.Height);
         EXPECT_LE(thumb5->GetWidth(), 150u);
         EXPECT_LE(thumb5->GetHeight(), 120u);
         Gdiplus::Color color;
         thumb5->GetPixel(100, 100, &color);
         EXPECT_EQ(4278190080, color.GetValue());
     }

     for (int i = 1; i <= 8; i++) {
         std::string fname = "Images/Portrait_" + std::to_string(i) + ".jpg";
         auto thumb6 = GetThumbnail(U2W(TestHelpers::resolvePath(fname)), 150, 120, &size);
         ASSERT_TRUE(thumb6 != nullptr);
         EXPECT_EQ(1200, size.Width);
         EXPECT_EQ(1800, size.Height);
         EXPECT_LE(thumb6->GetWidth(), 150u);
         EXPECT_LE(thumb6->GetHeight(), 120u);
         Gdiplus::Color color;
         thumb6->GetPixel(100, 100, &color);
         EXPECT_EQ(4278190080, color.GetValue());
     }


}
/*
TEST_F(UtilsTest, CopyBitmapToClipboardInDataUriFormat) {
    const char* fileName = "file_with_const_size.png";
    std::unique_ptr<Bitmap> bm(Bitmap::FromFile(U2W(fileName)));
    ASSERT_TRUE(bm != nullptr);
    bool res = CopyBitmapToClipboardInDataUriFormat(bm.get(), 1, 85); 
    EXPECT_TRUE(res);

    waitFormatAvailable();
    
    CString clipboardText;
    res = WinUtils::GetClipboardText(clipboardText, 0, true);
    ASSERT_TRUE(res);
    std::string clipboardTextA = W2U(clipboardText);
    std::string md5 = IuCoreUtils::CryptoUtils::CalcMD5HashFromString(clipboardTextA);
    EXPECT_EQ("7b0e5e0e312bfed196c117fe821a4e30", md5);

    
    // Test HTML format
    res = CopyBitmapToClipboardInDataUriFormat(bm.get(), 1, 85, true);
    EXPECT_TRUE(res);
    waitFormatAvailable();
    CString clipboardText2;
    res = WinUtils::GetClipboardText(clipboardText2, 0, true);
    ASSERT_TRUE(res);
    std::string clipboardTextA2 = W2U(clipboardText2);
    std::string md52 = IuCoreUtils::CryptoUtils::CalcMD5HashFromString(clipboardTextA2);
    EXPECT_EQ("88d4f8b907c9b4d596a920419ccdd6f4", md52);
}
*/

TEST_F(UtilsTest, GetImageInfo) {
    std::string fname = TestHelpers::resolvePath("file_with_const_size.png");
    ASSERT_TRUE(IuCoreUtils::FileExists(fname));
    ImageInfo ii = GetImageInfo(U2W(fname));
    EXPECT_EQ(251, ii.width);
    EXPECT_EQ(366, ii.height);
    ImageInfo ii2 = GetImageInfo(U2W(TestHelpers::resolvePath("notexistingfile57345345.png")));
    EXPECT_EQ(0, ii2.width);
    EXPECT_EQ(0, ii2.height);
}

TEST_F(UtilsTest, ExUtilReadFile) {
    std::string fileName = TestHelpers::resolvePath("file_with_const_size.png");
    ASSERT_TRUE(IuCoreUtils::FileExists(fileName));

    {
        auto [data, size] = ExUtilReadFile(U2W(fileName));
        EXPECT_TRUE(!!data);
        EXPECT_EQ(14830, size);
        std::string hash = IuCoreUtils::CryptoUtils::CalcMD5Hash(data.get(), size);
        EXPECT_EQ("ebbd98fc18bce0e9dd774f836b5c3bf8", hash);
    }
    {
        auto [data, size] = ExUtilReadFile(U2W(TestHelpers::resolvePath("file_with_zero_size.dat")));
        EXPECT_EQ(0, size);
        EXPECT_TRUE(!!data);
    }
}

TEST_F(UtilsTest, ExUtilReadFileExceptions) {
    EXPECT_THROW(ExUtilReadFile(U2W(TestHelpers::resolvePath("notexistingfile5734533345.png"))), IOException);
}

TEST_F(UtilsTest, StringToColor) {
    {
        Gdiplus::Color clr = StringToColor("#ffffff");
        EXPECT_EQ(255, clr.GetR());
        EXPECT_EQ(255, clr.GetG());
        EXPECT_EQ(255, clr.GetB());
        EXPECT_EQ(255, clr.GetA());
    }
    {
        Gdiplus::Color clr = StringToColor("#000000");
        EXPECT_EQ(0, clr.GetR());
        EXPECT_EQ(0, clr.GetG());
        EXPECT_EQ(0, clr.GetB());
        EXPECT_EQ(255, clr.GetA());
    }
    {
        Gdiplus::Color clr = StringToColor("rgb(0,0,1)");
        EXPECT_EQ(0, clr.GetR());
        EXPECT_EQ(0, clr.GetG());
        EXPECT_EQ(1, clr.GetB());
        EXPECT_EQ(255, clr.GetA());
    }
    {
        Gdiplus::Color clr = StringToColor("rgb(255,13,184)");
        EXPECT_EQ(255, clr.GetR());
        EXPECT_EQ(13, clr.GetG());
        EXPECT_EQ(184, clr.GetB());
        EXPECT_EQ(255, clr.GetA());
    }
    {
        Gdiplus::Color clr = StringToColor("rgb(255, 13, 184)");
        EXPECT_EQ(255, clr.GetR());
        EXPECT_EQ(13, clr.GetG());
        EXPECT_EQ(184, clr.GetB());
        EXPECT_EQ(255, clr.GetA());
    }
    {
        Gdiplus::Color clr = StringToColor("rgba(255,13,184,1.0)");
        EXPECT_EQ(255, clr.GetR());
        EXPECT_EQ(13, clr.GetG());
        EXPECT_EQ(184, clr.GetB());
        EXPECT_EQ(255, clr.GetA());
    }
    {
        Gdiplus::Color clr = StringToColor("rgba(122,40,11,0.2)");
        EXPECT_EQ(122, clr.GetR());
        EXPECT_EQ(40, clr.GetG());
        EXPECT_EQ(11, clr.GetB());
        EXPECT_EQ(51, clr.GetA());
    }
    {
        Gdiplus::Color clr = StringToColor("rgba(122, 40, 11, 0.2)");
        EXPECT_EQ(122, clr.GetR());
        EXPECT_EQ(40, clr.GetG());
        EXPECT_EQ(11, clr.GetB());
        EXPECT_EQ(51, clr.GetA());
    }
    {
        Gdiplus::Color clr = StringToColor("#$$2$$$");
        EXPECT_EQ(0, clr.GetR());
        EXPECT_EQ(0, clr.GetG());
        EXPECT_EQ(0, clr.GetB());
        EXPECT_EQ(255, clr.GetA());
    }
    {
        Gdiplus::Color clr = StringToColor("");
        EXPECT_EQ(0, clr.GetR());
        EXPECT_EQ(0, clr.GetG());
        EXPECT_EQ(0, clr.GetB());
        EXPECT_EQ(255, clr.GetA());
    }
}

TEST_F(UtilsTest, SaveImageToFile) {
    TempFileDeleter deleter;
    {
        std::string destFile = "out_test_file1.png";
        deleter.addFile(destFile);
        CString wideDestFile = U2W(destFile);
        Gdiplus::Bitmap bm(640, 480);
        CString mimeType;
        EXPECT_TRUE(SaveImageToFile(&bm, wideDestFile, nullptr, sifPNG, 95, &mimeType));
        ImageLoader loader;
        auto img = loader.loadFromFile(wideDestFile);
        EXPECT_EQ(640, img->getWidth());
        EXPECT_EQ(480, img->getHeight());
        EXPECT_TRUE(!!img->getBitmap());
        EXPECT_EQ("image/png", mimeType);
        EXPECT_EQ("image/png", IuCoreUtils::GetFileMimeType(destFile));
    }
    {
        std::string destFile = "out_test_file2.jpg";
        deleter.addFile(destFile);
        CString wideDestFile = U2W(destFile);
        Gdiplus::Bitmap bm(200, 150);
        CString mimeType;
        EXPECT_TRUE(SaveImageToFile(&bm, wideDestFile, nullptr, sifJPEG, 95, &mimeType));
        ImageLoader loader;
        auto img = loader.loadFromFile(wideDestFile);
        EXPECT_EQ(200, img->getWidth());
        EXPECT_EQ(150, img->getHeight());
        EXPECT_TRUE(!!img->getBitmap());
        EXPECT_EQ("image/jpeg", mimeType);
        EXPECT_EQ("image/jpeg", IuCoreUtils::GetFileMimeType(destFile));
    }
    {
        std::string destFile = "out_test_file3.webp";
        deleter.addFile(destFile);
        CString wideDestFile = U2W(destFile);
        Gdiplus::Bitmap bm(200, 150);
        CString mimeType;
        EXPECT_TRUE(SaveImageToFile(&bm, wideDestFile, nullptr, sifWebp, 50, &mimeType));
        ImageLoader loader;
        auto img = loader.loadFromFile(wideDestFile);
        EXPECT_EQ(200, img->getWidth());
        EXPECT_EQ(150, img->getHeight());
        EXPECT_TRUE(!!img->getBitmap());
        EXPECT_EQ("image/webp", mimeType);
        EXPECT_EQ("image/webp", IuCoreUtils::GetFileMimeType(destFile));
    }
    {
        std::string destFile = "out_test_file4.webp";
        deleter.addFile(destFile);
        CString wideDestFile = U2W(destFile);
        Gdiplus::Bitmap bm(200, 150);
        CString mimeType;
        EXPECT_TRUE(SaveImageToFile(&bm, wideDestFile, nullptr, sifWebpLossless, 20, &mimeType));
        ImageLoader loader;
        auto img = loader.loadFromFile(wideDestFile);
        EXPECT_EQ(200, img->getWidth());
        EXPECT_EQ(150, img->getHeight());
        EXPECT_TRUE(!!img->getBitmap());
        EXPECT_EQ("image/webp", mimeType);
        EXPECT_EQ("image/webp", IuCoreUtils::GetFileMimeType(destFile));
    }

}

TEST_F(UtilsTest, SaveImageToStream) {
    {
        IStream* stream = SHCreateMemStream(nullptr, 0);
        ASSERT_TRUE(stream != nullptr);
        Gdiplus::Bitmap bm(200, 150);
        CString mimeType;
        EXPECT_TRUE(SaveImageToFile(&bm, CString(), stream, sifPNG, 95, &mimeType));
        ULARGE_INTEGER size = {};
        ASSERT_HRESULT_SUCCEEDED(IStream_Size(stream, &size));
        EXPECT_LT(10, size.QuadPart);
        stream->Release();
    }
    {
        IStream* stream = SHCreateMemStream(nullptr, 0);
        ASSERT_TRUE(stream != nullptr);
        Gdiplus::Bitmap bm(200, 150);
        CString mimeType;
        EXPECT_TRUE(SaveImageToFile(&bm, CString(), stream, sifWebp, 95, &mimeType));
        ULARGE_INTEGER size = {};
        ASSERT_HRESULT_SUCCEEDED(IStream_Size(stream, &size));
        EXPECT_LT(10, size.QuadPart);
        stream->Release();
    }
    {
        IStream* stream = SHCreateMemStream(nullptr, 0);
        ASSERT_TRUE(stream != nullptr);
        Gdiplus::Bitmap bm(200, 150);
        CString mimeType;
        EXPECT_TRUE(SaveImageToFile(&bm, CString(), stream, sifWebpLossless, 95, &mimeType));
        ULARGE_INTEGER size = {};
        ASSERT_HRESULT_SUCCEEDED(IStream_Size(stream, &size));
        EXPECT_LT(10, size.QuadPart);
        stream->Release();
    }
}

TEST_F(UtilsTest, SaveImageToFileStream) {
    TempFileDeleter deleter;
    {
        std::string destFile = "out_test_file1.png";
        
        deleter.addFile(destFile);
        CString wideDestFile = U2W(destFile);
        IStream* stream{};
        ASSERT_HRESULT_SUCCEEDED(SHCreateStreamOnFileW(wideDestFile, STGM_CREATE|STGM_WRITE, &stream));
        ASSERT_TRUE(stream != nullptr);
        Gdiplus::Bitmap bm(200, 150);
        CString mimeType;
        EXPECT_TRUE(SaveImageToFile(&bm, CString(), stream, sifPNG, 95, &mimeType));
        stream->Release();

        EXPECT_EQ("image/png", IuCoreUtils::GetFileMimeType(destFile));
        ImageLoader loader;
        auto img = loader.loadFromFile(wideDestFile);
        EXPECT_TRUE(!!img->getBitmap());
        EXPECT_EQ(200, img->getWidth());
        EXPECT_EQ(150, img->getHeight());
    }
    {
        std::string destFile = "out_test_file2.webp";

        deleter.addFile(destFile);
        CString wideDestFile = U2W(destFile);
        IStream* stream{};
        ASSERT_HRESULT_SUCCEEDED(SHCreateStreamOnFileW(wideDestFile, STGM_CREATE | STGM_WRITE, &stream));
        ASSERT_TRUE(stream != nullptr);
        Gdiplus::Bitmap bm(200, 150);
        CString mimeType;
        EXPECT_TRUE(SaveImageToFile(&bm, CString(), stream, sifWebp, 95, &mimeType));
        stream->Release();

        EXPECT_EQ("image/webp", IuCoreUtils::GetFileMimeType(destFile));
        ImageLoader loader;
        auto img = loader.loadFromFile(wideDestFile);
        EXPECT_TRUE(!!img->getBitmap());
        EXPECT_EQ(200, img->getWidth());
        EXPECT_EQ(150, img->getHeight());
    }
    {
        std::string destFile = "out_test_file3.webp";

        deleter.addFile(destFile);
        CString wideDestFile = U2W(destFile);
        IStream* stream{};
        ASSERT_HRESULT_SUCCEEDED(SHCreateStreamOnFileW(wideDestFile, STGM_CREATE | STGM_WRITE, &stream));
        ASSERT_TRUE(stream != nullptr);
        Gdiplus::Bitmap bm(200, 150);
        CString mimeType;
        EXPECT_TRUE(SaveImageToFile(&bm, CString(), stream, sifWebpLossless, 95, &mimeType));
        stream->Release();

        EXPECT_EQ("image/webp", IuCoreUtils::GetFileMimeType(destFile));
        ImageLoader loader;
        auto img = loader.loadFromFile(wideDestFile);
        EXPECT_TRUE(!!img->getBitmap());
        EXPECT_EQ(200, img->getWidth());
        EXPECT_EQ(150, img->getHeight());
    }
}

TEST_F(UtilsTest, SaveImageToFileExceptions) {
    TempFileDeleter deleter;
    {
        std::string destFile = "not_existing_directory/out_test_file1.png";
        deleter.addFile(destFile);
        CString wideDestFile = U2W(destFile);
        Gdiplus::Bitmap bm(200, 100);
        EXPECT_THROW(SaveImageToFile(&bm, wideDestFile, nullptr, sifPNG, 95, nullptr), IOException);
    }
    {
        std::string destFile = "not_existing_directory/out_test_file2.webp";
        deleter.addFile(destFile);
        CString wideDestFile = U2W(destFile);
        Gdiplus::Bitmap bm(200, 100);
        EXPECT_THROW(SaveImageToFile(&bm, wideDestFile, nullptr, sifWebp, 95, nullptr), IOException);
    }
}

TEST_F(UtilsTest, IsImageAnimated) {
    {
        Gdiplus::Image img(U2W(TestHelpers::resolvePath("Images/animation.gif")));
        ASSERT_EQ(Gdiplus::Ok, img.GetLastStatus());
        EXPECT_TRUE(IsImageMultiFrame(&img));
    }
    {

        Gdiplus::Image img(U2W(TestHelpers::resolvePath("file_with_const_size.png")));
        ASSERT_EQ(Gdiplus::Ok, img.GetLastStatus());
        EXPECT_FALSE(IsImageMultiFrame(&img));
    }
}
