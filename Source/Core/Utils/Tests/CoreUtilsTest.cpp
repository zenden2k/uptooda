#define __STDC_LIMIT_MACROS
#ifndef NOMINMAX
#define NOMINMAX
#endif

#include <cstdint>
#include <string>
//#include <limits.h>

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "Core/Utils/CoreUtils.h"
#include "Core/Utils/IOException.h"
#ifdef _WIN32
#include <WinSock.h>
#endif
#include "Core/Utils/CryptoUtils.h"
#include "Tests/TestHelpers.h"
#include "Core/3rdpart/pcreplusplus.h"

class CoreUtilsTest : public ::testing::Test {
public:
    CoreUtilsTest() : constSizeFileName(TestHelpers::resolvePath("file_with_const_size.png")) {
 
    }
protected:    
    const std::string constSizeFileName;
    const int64_t constSizeFileSize = 14830;
};

using namespace IuCoreUtils;

TEST_F(CoreUtilsTest, ExtractFilePath)
{
    std::string fileName = "c:\\Program Files (x86)\\Image Uploader\\Image Uploader.exe";
    std::string result = ExtractFilePath(fileName);
    EXPECT_EQ("c:\\Program Files (x86)\\Image Uploader\\", result);

    result = ExtractFilePath("c:/Program Files (x86)\\Image Uploader/ExplorerIntegration64.dll");
    EXPECT_EQ("c:/Program Files (x86)\\Image Uploader/", result);

    result = ExtractFilePath("avcodec-56.dll");
    EXPECT_EQ("", result);
}

TEST_F(CoreUtilsTest, ExtractFileName)
{
    std::string fileName = R"(c:\Program Files (x86)\Image Uploader\Image Uploader.exe)";
    std::string result = ExtractFileName(fileName);
    EXPECT_EQ("Image Uploader.exe", result);

    result = ExtractFileName("c:/Program Files (x86)\\Image Uploader/ExplorerIntegration64.dll");
    EXPECT_EQ("ExplorerIntegration64.dll", result);

    result = ExtractFileName("avcodec-56.dll");
    EXPECT_EQ("avcodec-56.dll", result);

    result = ExtractFileName("/usr/bin/imgupload");
    EXPECT_EQ("imgupload", result);

    result = ExtractFileName(R"(\\?\e:\Video\test.mp4)");
    EXPECT_EQ("test.mp4", result);

    result = ExtractFileName("");
    EXPECT_EQ("", result);
}

TEST_F(CoreUtilsTest, ExtractFileExt)
{
    std::string fileName = R"(c:\Program Files (x86)\Image Uploader\Image Uploader.exe)";
    std::string result = ExtractFileExt(fileName);
    EXPECT_EQ("exe", result);

    result = ExtractFileExt("c:/Program Files (x86)\\Image Uploader/ExplorerIntegration64.dll");
    EXPECT_EQ("dll", result);

    result = ExtractFileExt(R"(c:\Program Files .(x86)\Image Uploader.exe\Image Uploader)");
    EXPECT_EQ("", result);

    result = ExtractFileExt("archive.tar.gz");
    EXPECT_EQ("gz", result);

    result = ExtractFileExt("/usr/share/imgupload.png");
    EXPECT_EQ("png", result);

    result = ExtractFileExt(R"(\\?\e:\Video\test.mp4)");
    EXPECT_EQ("mp4", result);

    result = ExtractFileExt("");
    EXPECT_EQ("", result);
}

TEST_F(CoreUtilsTest, ExtractFileNameNoExt)
{
    std::string fileName = R"(c:\Program Files (x86)\Image Uploader\Image Uploader.exe)";
    std::string result = ExtractFileNameNoExt(fileName);
    EXPECT_EQ("Image Uploader", result);

    result = ExtractFileNameNoExt("c:/Program Files (x86)\\Image Uploader/ExplorerIntegration64.dll");
    EXPECT_EQ("ExplorerIntegration64", result);

    result = ExtractFileNameNoExt(R"(c:\Program Files .(x86)\Image Uploader.exe\Image Uploader)");
    EXPECT_EQ("Image Uploader", result);

    result = ExtractFileNameNoExt("archive.tar.gz");
    EXPECT_EQ("archive.tar", result);

    result = ExtractFileNameNoExt(R"(\\?\e:\Video\test.mp4)");
    EXPECT_EQ("test", result);
}

TEST_F(CoreUtilsTest, GenerateRandomFilename)
{
    using testing::MatchesRegex;

    {
        std::string fileName = R"(c:\Program Files (x86)\Image Uploader\Image Uploader.exe)";
        std::string result = GenerateRandomFilename(fileName, 8);
        pcrepp::Pcre regexp(R"(^c:\\Program Files \(x86\)\\Image Uploader\\Image Uploader_\w{8}\.exe$)");
        EXPECT_TRUE(regexp.search(result));
    }

    {
        std::string fileName = "ExplorerIntegration64.dll";
        std::string result = GenerateRandomFilename(fileName, 5);
        pcrepp::Pcre regexp(R"(^ExplorerIntegration64_\w{5}\.dll$)");
        EXPECT_TRUE(regexp.search(result));
    }

    {
        std::string fileName = "ExplorerIntegration64.dll";
        std::string result = GenerateRandomFilename(fileName, 0);
        pcrepp::Pcre regexp(R"(^ExplorerIntegration64_\w+\.dll$)");
        EXPECT_TRUE(regexp.search(result));
    }

    {
        std::string fileName = "/usr/bin/imgupload";
        std::string result = GenerateRandomFilename(fileName, 7);
        pcrepp::Pcre regexp(R"(^/usr/bin/imgupload_\w{7}$)");
        EXPECT_TRUE(regexp.search(result));
    }

    {
        std::string fileName = "imgupload";
        std::string result = GenerateRandomFilename(fileName, 1);
        pcrepp::Pcre regexp(R"(^imgupload_\w$)");
        EXPECT_TRUE(regexp.search(result));
    }

    {
        std::string fileName = "";
        std::string result = GenerateRandomFilename(fileName, 10);
        pcrepp::Pcre regexp(R"(^\w{10}$)");
        EXPECT_TRUE(regexp.search(result));
    }
    //EXPECT_THAT(result, MatchesRegex(R"(c:\\Program Files \(x86\)\\Image Uploader\\Image Uploader_\w+\.exe)"));
}

TEST_F(CoreUtilsTest, ExtractFileNameFromUrl)
{
    std::string result = ExtractFileNameFromUrl("http://www.directupload.net/file/d/3970/vmtbrege_jpg.htm");
    EXPECT_EQ("vmtbrege_jpg.htm", result);
    result = ExtractFileNameFromUrl("http://www.directupload.net/file/d/3970/vmtbrege_jpg.html?test=124&grgrg=54+5435");
    EXPECT_EQ("vmtbrege_jpg.html", result);

}

TEST_F(CoreUtilsTest, stringToint64_t) 
{
    EXPECT_TRUE(StringToInt64("9223372036854775807")== 9223372036854775807);
    EXPECT_TRUE(StringToInt64("-9223372036854775807") == INT64_C(-9223372036854775807));
    EXPECT_TRUE(StringToInt64("0")== 0);
}

TEST_F(CoreUtilsTest, GetDefaultExtensionForMimeType) 
{
    EXPECT_EQ("gif", GetDefaultExtensionForMimeType("image/gif"));
    EXPECT_EQ("png", GetDefaultExtensionForMimeType("image/png"));
    EXPECT_EQ("jpg", GetDefaultExtensionForMimeType("image/jpeg"));
    EXPECT_EQ("webp", GetDefaultExtensionForMimeType("image/webp"));
    EXPECT_EQ("", GetDefaultExtensionForMimeType("application/unknown"));
}

TEST_F(CoreUtilsTest, fileSizeToString) 
{
    EXPECT_EQ(FileSizeToString(140*1024*1024), "140 MB");
    EXPECT_EQ(FileSizeToString(static_cast<int64_t>(25.5*1024)), "25.5 KB");
    EXPECT_EQ(FileSizeToString(0), "0 B");
    EXPECT_EQ("10 GB", FileSizeToString(10ULL * 1024 * 1024 * 1024));
    EXPECT_EQ("", FileSizeToString(-1));
}

TEST_F(CoreUtilsTest, Utf8ToWstring)
{
    std::wstring result = Utf8ToWstring("\x49\xC3\xB1\x74\xC3\xAB\x72\x6E\xC3\xA2\x74\x69\xC3\xB4\x6E\xC3\xA0\x6C\x69\x7A\xC3\xA6\x74\x69\xC3\xB8\x6E");
    EXPECT_EQ(result, L"I\u00F1t\u00EBrn\u00E2ti\u00F4n\u00E0liz\u00E6ti\u00F8n");
    result = Utf8ToWstring("");
    EXPECT_EQ(result, L"");
    result = Utf8ToWstring("test test");
    EXPECT_EQ(result, L"test test");
}

TEST_F(CoreUtilsTest, getFileSize)
{
    int64_t size = IuCoreUtils::GetFileSize("not_existing_file22342343");
    EXPECT_EQ(size, -1);
    ASSERT_TRUE(IuCoreUtils::FileExists(constSizeFileName));
    size = IuCoreUtils::GetFileSize(constSizeFileName);
    EXPECT_EQ(size, constSizeFileSize);
}

TEST_F(CoreUtilsTest, copyFile)
{
    std::string destFile = "out_new_file.png";
    ASSERT_TRUE(IuCoreUtils::FileExists(constSizeFileName));
    bool res = IuCoreUtils::CopyFileToDest(constSizeFileName, destFile, true);
    EXPECT_EQ(res, true);
    EXPECT_EQ(IuCoreUtils::FileExists(destFile), true);
    int64_t size = IuCoreUtils::GetFileSize(destFile);
    EXPECT_EQ(size, constSizeFileSize);
}

#ifdef _WIN32
TEST_F(CoreUtilsTest, gettimeofday)
{
    struct timeval tp;
    gettimeofday(&tp, NULL);
    int64_t curTime = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    gettimeofday(&tp, NULL);
    int64_t curTime2 = tp.tv_sec * 1000 + tp.tv_usec / 1000;
    EXPECT_GE(curTime2, curTime2);
}
#endif

TEST_F(CoreUtilsTest, ReadUtf8TextFile)
{
    std::string file = TestHelpers::resolvePath("utf8_text_file_with_bom.txt");
    ASSERT_TRUE(IuCoreUtils::FileExists(file));
    std::string data;
    EXPECT_TRUE(ReadUtf8TextFile(file, data));
    EXPECT_EQ("Test file with bom. \xD0\xA2\xD0\xB5\xD1\x81\xD1\x82\xD0\xBE\xD0\xB2\xD1\x8B\xD0\xB9 \xD1\x84\xD0\xB0\xD0\xB9\xD0\xBB \xD1\x81 BOM.", data);

    std::string file2 = TestHelpers::resolvePath("utf8_text_file.txt");
    std::string data2;
    EXPECT_TRUE(ReadUtf8TextFile(file2, data2));
    EXPECT_EQ("Test file without bom. \xD0\xA2\xD0\xB5\xD1\x81\xD1\x82\xD0\xBE\xD0\xB2\xD1\x8B\xD0\xB9 \xD1\x84\xD0\xB0\xD0\xB9\xD0\xBB \xD0\xB1\xD0\xB5\xD0\xB7 BOM.", data2);
    
    std::string file3 = TestHelpers::resolvePath("file_with_zero_size.dat");
    std::string data3;
    EXPECT_TRUE(ReadUtf8TextFile(file3, data3));
    EXPECT_EQ("", data3);

    std::string file4 = TestHelpers::resolvePath("utf16_text_file.txt");
    std::string data4;
    EXPECT_TRUE(ReadUtf8TextFile(file4, data4));
    EXPECT_EQ("Test file in UTF16-LE. \xD0\xA2\xD0\xB5\xD1\x81\xD1\x82\xD0\xBE\xD0\xB2\xD1\x8B\xD0\xB9 \xD1\x84\xD0\xB0\xD0\xB9\xD0\xBB.", data4);
 
    std::string file5 = TestHelpers::resolvePath("not_existing_file226546546343");
    std::string data5;
    EXPECT_FALSE(ReadUtf8TextFile(file5, data5));
}

TEST_F(CoreUtilsTest, GetFileContents)
{
    ASSERT_TRUE(IuCoreUtils::FileExists(constSizeFileName));
    std::string data = GetFileContents(constSizeFileName);
    EXPECT_EQ(constSizeFileSize, data.size());
    std::string hash = IuCoreUtils::CryptoUtils::CalcMD5Hash(&data[0], data.size());
    EXPECT_EQ("ebbd98fc18bce0e9dd774f836b5c3bf8", hash);

    std::string filePath = TestHelpers::resolvePath("not_existing_file2265466576546343");
    EXPECT_THROW(GetFileContents(filePath), IOException);
}

TEST_F(CoreUtilsTest, GetFileContentsEx_Basic)
{
    ASSERT_TRUE(IuCoreUtils::FileExists(constSizeFileName));
    {
        std::string data = GetFileContentsEx(constSizeFileName, 100, 1000);
        EXPECT_EQ(1000, data.size());
        std::string hash = IuCoreUtils::CryptoUtils::CalcMD5Hash(data.data(), data.size());
        EXPECT_EQ("920a5642b048c0b04e05591f1e314218", hash);
    }
    {
        std::string data = GetFileContentsEx(constSizeFileName, 0, constSizeFileSize);
        EXPECT_EQ(constSizeFileSize, data.size());
        std::string hash = IuCoreUtils::CryptoUtils::CalcMD5Hash(data.data(), data.size());
        EXPECT_EQ("ebbd98fc18bce0e9dd774f836b5c3bf8", hash);
    }
    {
        std::string data = GetFileContentsEx(constSizeFileName, 5000, constSizeFileSize, true);
        EXPECT_EQ(constSizeFileSize - 5000, data.size());
        std::string hash = IuCoreUtils::CryptoUtils::CalcMD5Hash(data.data(), data.size());
        EXPECT_EQ("6855f0ede7f43f04b46869655823c513", hash);
    }
    {
        std::string data = GetFileContentsEx(constSizeFileName, 5000, 0);
        EXPECT_EQ(0, data.size());
    }
    {
        std::string data = GetFileContentsEx(constSizeFileName, 0, 0);
        EXPECT_EQ(0, data.size());
    }
}

TEST_F(CoreUtilsTest, GetFileContentsEx_Errors)
{
    const std::string zeroSizeFile = TestHelpers::resolvePath("file_with_zero_size.dat");
    EXPECT_THROW(GetFileContentsEx(TestHelpers::resolvePath("not_existing_file226546546343"), 100, 1000), std::system_error);

    // with allowPartialRead = false
    EXPECT_THROW(GetFileContentsEx(constSizeFileName, 0, constSizeFileSize + 1), std::out_of_range);
    EXPECT_THROW(GetFileContentsEx(constSizeFileName, constSizeFileSize - 10, 100), std::out_of_range);
    EXPECT_THROW(GetFileContentsEx(constSizeFileName, -100, 100), std::out_of_range);
    EXPECT_THROW(GetFileContentsEx(constSizeFileName, -1, 0), std::out_of_range);
    EXPECT_THROW(GetFileContentsEx(zeroSizeFile, 0, 100), std::out_of_range);
    EXPECT_NO_THROW(GetFileContentsEx(zeroSizeFile, 0, 0));
    EXPECT_THROW(GetFileContentsEx(zeroSizeFile, 100, 100), std::out_of_range);

    // with allowPartialRead = true
    EXPECT_NO_THROW(GetFileContentsEx(constSizeFileName, 0, constSizeFileSize + 1, true));
    EXPECT_NO_THROW(GetFileContentsEx(constSizeFileName, constSizeFileSize - 10, 100, true));
    EXPECT_THROW(GetFileContentsEx(constSizeFileName, -100, 100, true), std::out_of_range);
    EXPECT_THROW(GetFileContentsEx(constSizeFileName, -1, 0, true), std::out_of_range);
    EXPECT_NO_THROW(GetFileContentsEx(zeroSizeFile, 0, 0, true));
    EXPECT_NO_THROW(GetFileContentsEx(zeroSizeFile, 0, 100, true));
    EXPECT_THROW(GetFileContentsEx(zeroSizeFile, 100, 100, true), std::out_of_range);
}

TEST_F(CoreUtilsTest, GetFileMimeType) {
    ASSERT_TRUE(IuCoreUtils::FileExists(constSizeFileName));
    std::string type = GetFileMimeType(constSizeFileName);
    EXPECT_EQ("image/png", type);

    std::string webpFilePath = TestHelpers::resolvePath("Images/poroshok.webp");
    ASSERT_TRUE(IuCoreUtils::FileExists(webpFilePath));
    std::string type2 = GetFileMimeType(webpFilePath);
    EXPECT_EQ("image/webp", type2);
    
    {
        std::string jpegFilePath = TestHelpers::resolvePath("\x67\x72\x61\x62\x5f\xd1\x8e\xd0\xbd\xd0\xb8\xd0\xba\xd0\xbe\xd0\xb4\x5f\xe4\xbd\xa0\xe5\xa5\xbd\x2e\x6a\x70\x67");
        ASSERT_TRUE(IuCoreUtils::FileExists(jpegFilePath));
        std::string type3 = GetFileMimeType(jpegFilePath);
        EXPECT_EQ("image/jpeg", type3);
    }
}