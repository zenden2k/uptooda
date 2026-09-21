/*

    Uptooda - free application for uploading images/files to the Internet

    Copyright 2007-2025 Sergey Svistunov (zenden2k@gmail.com)

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.

*/


#include "CryptoUtils.h"

#include <array>
#include <cstdio>
#include <stdexcept>
#include <string>

#include <openssl/md5.h>
#include <openssl/sha.h>
#include <openssl/hmac.h>

#include "CoreUtils.h"

namespace IuCoreUtils {

std::string CryptoUtils::CalcMD5Hash(const void* data, size_t size)
{
    std::string result;
    MD5_CTX context;

    MD5_Init(&context);
    MD5_Update(&context, data, size);

    unsigned char buff[16] = "";

    MD5_Final(buff, &context);

    for(int i=0;i<16; i++)
    {
        char temp[5];
        sprintf(temp, "%02x",buff[i]);
        result += temp;
    }
    return result;
}

std::string CryptoUtils::CalcMD5HashFromString(const std::string& data)
{
    std::string result;
    MD5_CTX context;

    MD5_Init(&context);
    MD5_Update(&context, (unsigned char*)data.c_str(), data.length());

    unsigned char buff[16] = "";

    MD5_Final(buff, &context);

    for(int i=0;i<16; i++)
    {
        char temp[5];
        sprintf(temp, "%02x",buff[i]);
        result += temp;
    }
    return result;
}

std::string CryptoUtils::CalcMD5HashFromFile(const std::string& filename) {
    std::string result;
    MD5_CTX context;

    MD5_Init(&context);
    FILE* f = IuCoreUtils::FopenUtf8( filename.c_str(), "rb" );
    if (f) {
        unsigned char buf[4096];
        while ( !feof(f) ) {
            size_t bytesRead = fread( buf, 1, sizeof(buf), f );
            if (  bytesRead == 0 ) {
                break;
            }
            MD5_Update(&context, (unsigned char*)buf, bytesRead);
        }
        unsigned char buff[16] = "";

        MD5_Final(buff, &context);

        fclose(f);

        for (int i = 0; i < 16; i++) {
            char temp[5];
            sprintf( temp, "%02x", buff[i] );
            result += temp;
        }
    }

    return result;
}

std::string CryptoUtils::CalcSHA1Hash(const void* data, size_t size) {
    const int HashSize = 20;
    std::string result;
    SHA_CTX context;

    SHA1_Init( &context );
    SHA1_Update( &context, data, size );

    unsigned char buff[HashSize] = "";

    SHA1_Final( buff, &context);

    for ( int i = 0; i < HashSize; i++) {
        char temp[5];
        sprintf(temp, "%02x",buff[i]);
        result += temp;
    }
    return result;
}

std::string CryptoUtils::CalcHMACSHA1Hash(const std::string& key, const void* data, size_t size, bool base64) {
    unsigned char* digest;

    // Using sha1 hash engine here.
    // You may use other hash engines. e.g EVP_md5(), EVP_sha224, EVP_sha512, etc
    digest = HMAC(EVP_sha1(), key.c_str(), key.length(), (unsigned char*)data, size, NULL, NULL);

    char mdString[50]="";
    for(int i = 0; i < 20; i++) {
        sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);
    }
    if ( base64 ) {
        return CryptoUtils::Base64EncodeRaw(reinterpret_cast<const char*>(digest), 20);
    }

    return mdString;
}

std::string CryptoUtils::CalcHMACSHA1HashFromString(const std::string& key, const std::string& data, bool base64) {
    return CalcHMACSHA1Hash( key, data.data(), data.size(), base64 );
}

std::string CryptoUtils::CalcSHA1HashFromString(const std::string& data) {
    return CalcSHA1Hash( data.data(), data.size() );
}

std::string CryptoUtils::CalcSHA1HashFromFileWithPrefix(const std::string& filename, const std::string& prefix, const std::string& postfix) {
    const int HashSize = 20;
    std::string result;
    SHA_CTX context;

    SHA1_Init(&context);
    FILE* f = IuCoreUtils::FopenUtf8( filename.c_str(), "rb" );

    if (f) {
        if (!prefix.empty()) {
            SHA1_Update(&context, prefix.data(), prefix.length());
        }
        unsigned char buf[4096];
        while ( !feof(f) ) {
            size_t bytesRead = fread(buf, 1, sizeof(buf), f);

            SHA1_Update(&context, buf, bytesRead);
        }
        unsigned char buff[HashSize] = "";
        if (!postfix.empty()) {
            SHA1_Update(&context, postfix.data(), postfix.length());
        }
        SHA1_Final(buff, &context);

        fclose(f);

        for (int i = 0; i < HashSize; i++) {
            char temp[5];
            sprintf(temp, "%02x", buff[i]);
            result += temp;
        }
    }
    return result;
}

std::string CryptoUtils::CalcSHA1HashFromFile(const std::string& filename) {
    return CalcSHA1HashFromFileWithPrefix(filename, "", "");
}

std::string CryptoUtils::CalcSHA256Hash(const void* data, size_t size) {
    std::array<unsigned char, SHA256_DIGEST_LENGTH> hash{};
    SHA256_CTX context;

    if (SHA256_Init(&context) != 1 ||
        SHA256_Update(&context, data, size) != 1 ||
        SHA256_Final(hash.data(), &context) != 1) {
        throw std::runtime_error("SHA256 hashing failed");
    }

    static constexpr char HEX_DIGITS[] = "0123456789abcdef";

    std::string result;
    result.resize(hash.size() * 2);

    for (size_t i = 0; i < hash.size(); ++i) {
        result[i * 2] = HEX_DIGITS[hash[i] >> 4];
        result[i * 2 + 1] = HEX_DIGITS[hash[i] & 0x0F];
    }

    return result;
}

std::string CryptoUtils::CalcSHA256HashFromFile(const std::string& filename, int64_t offset, size_t chunkSize) {
    const int HashSize = 32;
    std::string result;
    SHA256_CTX context;

    SHA256_Init(&context);
    FILE* f = IuCoreUtils::FopenUtf8(filename.c_str(), "rb");

    if (f) {
        if (offset) {
            IuCoreUtils::Fseek64(f, offset, SEEK_SET);
        }

        unsigned char buf[4096];
        int64_t totalRead = 0;
        while (!feof(f)) {
            size_t bytesRead = fread(buf, 1, chunkSize ? std::min(chunkSize - totalRead, sizeof(buf)): sizeof(buf), f);

            SHA256_Update(&context, (unsigned char*)buf, bytesRead);
            totalRead += bytesRead;
            if (chunkSize && totalRead >= chunkSize) {
                break;
            }
        }
        unsigned char buff[HashSize] = "";
        SHA256_Final(buff, &context);

        fclose(f);

        for (int i = 0; i < HashSize; i++) {
            char temp[5];
            sprintf(temp, "%02x", buff[i]);
            result += temp;
        }
    }
    return result;
}

std::string CryptoUtils::CalcSHA256HashFromString(const std::string& data) {
    return CalcSHA256Hash(data.data(), data.size());
}


std::string CryptoUtils::CalcSHA512Hash(const void* data, size_t size) {
    constexpr auto HashSize = 64;
    std::string result;
    SHA512_CTX context;

    SHA512_Init(&context);
    SHA512_Update(&context, data, size);

    unsigned char buff[HashSize] = "";

    SHA512_Final(buff, &context);

    for (int i = 0; i < HashSize; i++) {
        char temp[5];
        sprintf(temp, "%02x", buff[i]);
        result += temp;
    }
    return result;
}


std::string CryptoUtils::CalcSHA512HashFromFile(const std::string& filename, int64_t offset, size_t chunkSize) {
    constexpr auto HashSize = 64;
    std::string result;
    SHA512_CTX context;

    SHA512_Init(&context);
    FILE* f = IuCoreUtils::FopenUtf8(filename.c_str(), "rb");

    if (f) {
        if (offset) {
            IuCoreUtils::Fseek64(f, offset, SEEK_SET);
        }

        unsigned char buf[4096];
        int64_t totalRead = 0;
        while (!feof(f)) {
            size_t bytesRead = fread(buf, 1, chunkSize ? std::min(chunkSize - totalRead, sizeof(buf)) : sizeof(buf), f);

            SHA512_Update(&context, (unsigned char*)buf, bytesRead);
            totalRead += bytesRead;
            if (chunkSize && totalRead >= chunkSize) {
                break;
            }
        }
        unsigned char buff[HashSize] = "";
        SHA512_Final(buff, &context);

        fclose(f);

        for (int i = 0; i < HashSize; i++) {
            char temp[5];
            sprintf(temp, "%02x", buff[i]);
            result += temp;
        }
    }
    return result;
}

std::string CryptoUtils::CalcSHA512HashFromString(const std::string& data) {
    return CalcSHA512Hash(data.data(), data.size());
}

}; // end of namespace IuCoreUtils
