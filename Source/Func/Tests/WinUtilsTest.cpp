#include <gtest/gtest.h>

#include "Core/Utils/CoreTypes.h"
#include "Func/WinUtils.h"

using namespace WinUtils;

class WinUtilsTest : public ::testing::Test {

};

TEST_F(WinUtilsTest, myExtractFileName) {
    CString fileName = L"c:\\Program Files (x86)\\Image Uploader\\Image Uploader.exe";
    CString result = DoExtractFileName(fileName);
    EXPECT_STREQ(L"Image Uploader.exe", result);
    result = DoExtractFileName(L"c:/Program Files (x86)\\Image Uploader/ExplorerIntegration64.dll");
    EXPECT_STREQ(L"ExplorerIntegration64.dll", result);
    result = DoExtractFileName(L"avcodec-56.dll");
    EXPECT_STREQ(L"avcodec-56.dll", result);
    result = DoExtractFileName(L"\\\\?\\d:\\Develop\\imageuploader-1.3.2-vs2013\\image-uploader\\Source\\Tests\\CMakeLists.txt");
    EXPECT_STREQ(L"CMakeLists.txt", result);
    result = DoExtractFileName(L"\\\\?\\d:\\Develop\\imageuploader-1.3.2-vs2013\\..\\Source\\Tests\\CMakeLists2.txt");
    EXPECT_STREQ(L"CMakeLists2.txt", result);
}

TEST_F(WinUtilsTest, GetFileExt) {
    CString fileName = L"c:\\Program Files (x86)\\Image Uploader\\Image Uploader.exe";
    CString result = GetFileExt(fileName);
    EXPECT_STREQ(L"exe", result);
    result = GetFileExt(L"c:/Program Files (x86)\\Image Uploader/ExplorerIntegration64.dll");
    EXPECT_STREQ(L"dll", result);
    result = GetFileExt(L"c:\\Program Files .(x86)\\Image Uploader.exe\\Image Uploader");
    EXPECT_STREQ(L"", result);
    result = GetFileExt(L"archive.tar.gz");
    EXPECT_STREQ(L"gz", result);
    result = GetFileExt(L"\\\\?\\d:\\Develop\\imageuploader-1.3.2-vs2013\\..\\Source\\Tests\\CMakeLists2.TXT");
    EXPECT_STREQ(L"TXT", result);
}

TEST_F(WinUtilsTest, GetOnlyFileName) {
    CString fileName = L"c:\\Program Files (x86)\\Image Uploader\\Image Uploader.exe";
    CString result = GetOnlyFileName(fileName);
    EXPECT_STREQ(L"Image Uploader", result);
    result = GetOnlyFileName("c:/Program Files (x86)\\Image Uploader/ExplorerIntegration64.dll");
    EXPECT_STREQ(L"ExplorerIntegration64", result);
    result = GetOnlyFileName("c:\\Program Files .(x86)\\Image Uploader.exe\\Image Uploader");
    EXPECT_STREQ(L"Image Uploader", result);
    result = GetOnlyFileName(L"archive.tar.gz");
    EXPECT_STREQ(L"archive.tar", result);
    result = GetOnlyFileName(L"\\\\?\\d:\\Develop\\imageuploader-1.3.2-vs2013\\..\\Source\\Tests\\CMakeLists2.TXT");
    EXPECT_STREQ(L"CMakeLists2", result);
}

TEST_F(WinUtilsTest, ExtractFilePath) {
    CString fileName = L"c:\\Program Files (x86)\\Image Uploader\\Image Uploader.exe";
    TCHAR buffer[MAX_PATH] = {0};
    LPTSTR res = ExtractFilePath(fileName, buffer, MAX_PATH);
    EXPECT_STREQ(L"c:\\Program Files (x86)\\Image Uploader\\", buffer);
    EXPECT_STREQ(L"c:\\Program Files (x86)\\Image Uploader\\", res);
    
    ExtractFilePath(L"c:/Program Files (x86)\\Image Uploader/ExplorerIntegration64.dll", buffer, MAX_PATH);
    EXPECT_STREQ(L"c:/Program Files (x86)\\Image Uploader/", buffer);
    ExtractFilePath(L"avcodec-56.dll", buffer, MAX_PATH);
    EXPECT_STREQ(L"", buffer);
}

TEST_F(WinUtilsTest, TrimString) {
    CString fileName = L"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.";
    EXPECT_STREQ(L"Lorem ipsum d...magna aliqua.", TrimString(fileName, 30));
    CString fileName2 = L"c:\\Program Files (x86)\\Image Uploader\\Image Uploader.exe";
    EXPECT_STREQ(L"c:\\Program Files (x86)\\Image Uploader\\Image Uploader.exe", TrimString(fileName2, 57));
    EXPECT_STREQ(L"", TrimString(L"", 35));
}

TEST_F(WinUtilsTest, TrimStringEnd) {
    CString fileName = L"Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua.";
    EXPECT_STREQ(L"Lorem ipsum dolor sit amet,...", TrimStringEnd(fileName, 30));
    CString fileName2 = L"c:\\Program Files (x86)\\Image Uploader\\Image Uploader.exe";
    EXPECT_STREQ(L"c:\\Program Files (x86)\\Image Uploader\\Image Uploader.exe", TrimStringEnd(fileName2, 57));
    EXPECT_STREQ(L"", TrimStringEnd(L"", 35));
}

TEST_F(WinUtilsTest, IsStrInList) {
    WCHAR list[] = L"jpg\0jpeg\0png\0bmp\0gif\0tif\0tiff\0webp\0\0";
    EXPECT_TRUE(WinUtils::IsStrInList(L"gif", list));
    EXPECT_TRUE(WinUtils::IsStrInList(L"webp", list));
    EXPECT_TRUE(WinUtils::IsStrInList(L"WEBP", list));
    EXPECT_FALSE(WinUtils::IsStrInList(L"blabla", list));
    WCHAR list2[] = L"\0\0";
    EXPECT_FALSE(WinUtils::IsStrInList(L"blabla2", list2));
}

TEST_F(WinUtilsTest, ExtractStrFromList) {
    WCHAR list[] = L"Alpha,Beta,Gamma,Delta,Epsilon,Zeta,Eta,Theta,Iota,Kappa,Lambda";
    WCHAR buffer[20] = { 0 };

    bool res = ExtractStrFromList(list, 2, buffer, ARRAY_SIZE(buffer));
    EXPECT_TRUE(res);
    EXPECT_STREQ(L"Gamma", buffer);

    res = ExtractStrFromList(list, 10, buffer, ARRAY_SIZE(buffer));
    EXPECT_TRUE(res);
    EXPECT_STREQ(L"Lambda", buffer);

    res = ExtractStrFromList(list, 25, buffer, ARRAY_SIZE(buffer));
    EXPECT_FALSE(res);

    res = ExtractStrFromList(list, 22, buffer, ARRAY_SIZE(buffer), L"");
    EXPECT_FALSE(res);
    EXPECT_STREQ(L"", buffer);

    res = ExtractStrFromList(list, 24, buffer, ARRAY_SIZE(buffer), L"Default string");
    EXPECT_FALSE(res);
    EXPECT_STREQ(L"Default string", buffer);
}
