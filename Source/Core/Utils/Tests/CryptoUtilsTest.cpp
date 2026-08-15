#include <gtest/gtest.h>
#include "Core/Utils/CoreUtils.h"
#include "Core/Utils/CryptoUtils.h"
#include "Tests/TestHelpers.h"

class CryptoUtilsTest : public ::testing::Test {
public:

    CryptoUtilsTest() : constSizeFileName(TestHelpers::resolvePath("file_with_const_size.png")),
        zeroSizeFileName(TestHelpers::resolvePath("file_with_zero_size.dat"))
    {

    }
protected:
    const std::string constSizeFileName;
    const std::string zeroSizeFileName;
};

using namespace IuCoreUtils::CryptoUtils;

TEST_F(CryptoUtilsTest, CalcMD5HashFromString)
{
    std::string result = CalcMD5HashFromString("zenden");
    EXPECT_EQ("9bdfa43c0351de396a363848380a99b1", result);
}

TEST_F(CryptoUtilsTest, CalcSHA1HashFromString)
{
    std::string result = CalcSHA1HashFromString("zenden");
    EXPECT_EQ("dae26f5db3ecb29fd0282b712d619bbb518a4201", result);
}

TEST_F(CryptoUtilsTest, CalcHMACSHA1HashFromString)
{
    std::string result = CalcHMACSHA1HashFromString("someKey", "zenden", true);
    EXPECT_EQ("vf8nxw3pAU1L+yqK1T8exEi2kJA=", result);
    result = CalcHMACSHA1HashFromString("f#0{s^44a\"$>tB3zeIM6", "zenden2k", false);
    EXPECT_EQ("6e9b7ea9b590ffd406d6d8b2dca4c0e85cde9556", result);
}

TEST_F(CryptoUtilsTest, CalcMD5HashFromFile)
{
    std::string result = CalcMD5HashFromFile(constSizeFileName);
    EXPECT_EQ("ebbd98fc18bce0e9dd774f836b5c3bf8", result);
}


TEST_F(CryptoUtilsTest, CalcSHA1HashFromFile)
{
    std::string result = CalcSHA1HashFromFile(constSizeFileName);
    EXPECT_EQ("9eaa90959561639b4f1abc4914ce7b8943e0351c", result);
}

TEST_F(CryptoUtilsTest, CalcSHA1HashFromFileWithPrefix)
{
    std::string fileName = constSizeFileName;
    int64_t fileSize = IuCoreUtils::GetFileSize(fileName);
    std::string result = CalcSHA1HashFromFileWithPrefix(fileName, "mrCloud", std::to_string(fileSize));
    EXPECT_EQ("be933e5fa7236218736be5a538e559da15f5ab00", result);
}

TEST_F(CryptoUtilsTest, CalcSHA256HashFromString)
{
    std::string result = CalcSHA256HashFromString("zenden");
    EXPECT_EQ("f8b1f55e96be17bd326d178a472a00918d30f68604ce5ce2d5faf6cc3790eee6", result);
    result = CalcSHA256HashFromString("");
    EXPECT_EQ("e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855", result);
}

TEST_F(CryptoUtilsTest, CalcSHA256HashFromFile)
{
    std::string result = CalcSHA256HashFromFile(constSizeFileName);
    EXPECT_EQ("4a4c3b5200abfdcc89d67f0b8dbbe2da1ee91b49895e855256061222b41c8458", result);

    std::string result2 = CalcSHA256HashFromFile(constSizeFileName, 1000, 2000);
    EXPECT_EQ("e21595586706b01f90a3f78d1b49af9c7d2a58ed5613120c8a60fe109c14ff72", result2);
}

TEST_F(CryptoUtilsTest, CalcSHA512HashFromString) {
    std::string result = CalcSHA512HashFromString("zenden");
    EXPECT_EQ("852c808ee8f25c5e4ee3bae757a9ec2fbad5317457d12d70d6a445725524864ff4febf338946da09ee363a5ec5820b06cfb22666de3b40de5d1e4a4963fe1352", result);
}

TEST_F(CryptoUtilsTest, CalcSHA512HashFromFile) {
    std::string result = CalcSHA512HashFromFile(constSizeFileName);
    EXPECT_EQ("62b03b2a492a9376860dd7331d0dcc5c61383aa8eb77a6fac6c982a55d50a330b20fd2d2da91f0282437a7392f8a37ed0b92cb4c01d1d3da30ca776fdbf5f660", result);

    std::string result2 = CalcSHA512HashFromFile(constSizeFileName, 1000, 2000);
    EXPECT_EQ("11e7997a5f8a90153bdd2109cdb72f029394097db934a44d427db9843d6dd86ce66783e0c4a44501e87f9c07e828240604de16117adbb94687d00ef533ebd005", result2);
}

TEST_F(CryptoUtilsTest, Base64Encode)
{
    EXPECT_EQ("QmFzZTY0IGlzIGEgZ2VuZXJpYyB0ZXJtIGZvciBhIG51bWJlciBvZiBzaW1pbGFyIGVuY29kaW5nIHNjaGVtZXMgdGhhdCBlbmNvZGU=", Base64Encode("Base64 is a generic term for a number of similar encoding schemes that encode"));
    EXPECT_EQ("", Base64Encode(""));
}

TEST_F(CryptoUtilsTest, Base64Decode)
{
    EXPECT_EQ("Base64 is a generic term for a number of similar encoding schemes that encode", Base64Decode("QmFzZTY0IGlzIGEgZ2VuZXJpYyB0ZXJtIGZvciBhIG51bWJlciBvZiBzaW1pbGFyIGVuY29kaW5nIHNjaGVtZXMgdGhhdCBlbmNvZGU="));
    EXPECT_EQ("", Base64Decode(""));
}

TEST_F(CryptoUtilsTest, Base64EncodeFile)
{
    {
        std::string output;
        bool res = Base64EncodeFile(constSizeFileName, output);
        EXPECT_TRUE(res);
        EXPECT_EQ("e0f0a6e5fcd8712fe470af8bc875d525", CalcMD5HashFromString(output));
    }
    {
        std::string output;
        bool res = Base64EncodeFile(zeroSizeFileName, output);
        EXPECT_TRUE(res);
        EXPECT_EQ("", output);
    }
    {
        std::string output;
        bool res = Base64EncodeFile(TestHelpers::resolvePath("utf8_text_file.txt"), output);
        EXPECT_TRUE(res);
        EXPECT_EQ("VGVzdCBmaWxlIHdpdGhvdXQgYm9tLiDQotC10YHRgtC+0LLRi9C5INGE0LDQudC7INCx0LXQtyBCT00u", output);
    }
    {
        std::string output;
        bool res = Base64EncodeFile("notexistingfile437859347650342643438095734", output);
        EXPECT_FALSE(res);
        EXPECT_TRUE(output.empty());
    }
}

TEST_F(CryptoUtilsTest, Md5Crypt) {
    EXPECT_EQ("$1$v0iBkPDv$H9RJWM2bdbz35rEaM78zV.", Md5Crypt("1", "v0iBkPDv"));
    EXPECT_EQ("$1$saltsalt$mKNFfCWMzLKBEz7IDUA.F/", Md5Crypt("6WzaQ9jnl5fr39NAoEud", "saltsaltsalt"));
    EXPECT_FALSE(Md5Crypt("6WzaQ9jnl5fr39NAoEud", "").empty());
    EXPECT_THROW(Md5Crypt(nullptr, nullptr), std::invalid_argument);
}


