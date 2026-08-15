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

#include <cmath>
#include <array>

#include <libbase64.h>
#include <openssl/evp.h>
#include <openssl/conf.h>
#include <openssl/err.h>
#include <openssl/bio.h>
#include <openssl/buffer.h>
#include <openssl/md5.h>

#include "Core/Upload/CommonTypes.h"

namespace IuCoreUtils::CryptoUtils {

std::string Base64Encode(const std::string& data)
{
    std::string res;
    size_t outlen = ((4 * data.length() / 3) + 3) & ~3;
    res.resize(outlen);
    base64_encode(data.data(), data.length(), &res[0], &outlen, 0);
    res.resize(outlen);
    return res;
}

std::string Base64EncodeRaw(const char* bytes, unsigned int len) {
    std::string res;
    size_t outlen = ((4 * len / 3) + 3) & ~3;
    res.resize(outlen);
    base64_encode(bytes, len, &res[0], &outlen, 0);
    res.resize(outlen);
    return res;
}

std::string Base64Decode(const std::string& data)
{
    std::string res;
    auto outlen = static_cast<size_t>(std::ceil(data.length() * 3 / 4.0));
    res.resize(outlen);
    base64_decode(data.data(), data.length(), &res[0], &outlen, 0);
    res.resize(outlen);
    return res;
}

bool Base64EncodeFile(const std::string& fileName, std::string& result) {
    FILE* f = FopenUtf8(fileName.c_str(), "rb");
    if (!f) {
        return false;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        return false;
    }
    long fsize = ftell(f);
    rewind(f);
    result.clear();

    if (!fsize) {
        return true;
    }
    size_t outlen = 0, bytes_read;
    size_t buffer_size = /*((4 * fsize / 3) + 3) & ~3*/ 4 * fsize / 3 + 5;
    const int read_buffer_size = 1024 * 1024;
    auto read_buffer = std::make_unique<char[]>(read_buffer_size);
    result.resize(buffer_size, '\0');

    base64_state state;
    base64_stream_encode_init(&state, 0);
    char* encoded_cur = &result[0];
    size_t total_len = 0;
    while ((bytes_read = fread(read_buffer.get(), 1, read_buffer_size, f)) > 0) {
        outlen = 0;
        base64_stream_encode(&state, read_buffer.get(), bytes_read, encoded_cur, &outlen);
        encoded_cur += outlen;
        total_len += outlen;
    }

    base64_stream_encode_final(&state, encoded_cur, &outlen);
    total_len += outlen;
    result.resize(total_len);
    //encoded_cur += outlen;
    return true;
}

std::string DecryptAES(const std::string& base64cipher, const std::string& keyStr) {
    const std::vector<unsigned char> key(keyStr.begin(), keyStr.end());
    if (key.size() != 16) {
        throw std::runtime_error("Key must be 16 bytes for AES-128");
    }

    std::string base64Str = Base64Decode(base64cipher);
    std::vector<unsigned char> cipherData(base64Str.begin(), base64Str.end());

    if (cipherData.size() <= 16) {
        throw std::runtime_error("Ciphertext too short (no IV)");
    }

    std::vector<unsigned char> iv(cipherData.begin(), cipherData.begin() + 16);
    std::vector<unsigned char> encrypted(cipherData.begin() + 16, cipherData.end());

    EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        throw std::runtime_error("EVP_CIPHER_CTX_new failed");
    }

    int len;
    std::vector<unsigned char> plaintext(encrypted.size() + 16);

    if (1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr, key.data(), iv.data())) {
        throw std::runtime_error("EVP_DecryptInit_ex failed");
    }

    if (1 != EVP_DecryptUpdate(ctx, plaintext.data(), &len, encrypted.data(), encrypted.size())) {
        throw std::runtime_error("EVP_DecryptUpdate failed");
    }

    int plaintext_len = len;

    if (1 != EVP_DecryptFinal_ex(ctx, plaintext.data() + len, &len)) {
        throw std::runtime_error("EVP_DecryptFinal_ex failed");
    }

    plaintext_len += len;
    EVP_CIPHER_CTX_free(ctx);

    return std::string(plaintext.begin(), plaintext.begin() + plaintext_len);
}

static void _crypt_to64(char* s, unsigned long v, int n) {
    static const unsigned char itoa64[] = "./0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

    while (--n >= 0) {
        *s++ = itoa64[v & 0x3f];
        v >>= 6;
    }
}

// Based on freebsd /lib/libcrypt/crypt-md5.c
std::string Md5Crypt(const char* pw, const char* salt) {
    constexpr auto MD5_SIZE = 16;
    constexpr auto PASSWORD_MAX_LEN = 120;

    if (!pw || !salt) {
        throw std::invalid_argument("Null pointer passed as password or salt");
    }

    MD5_CTX ctx, ctx1;
    unsigned long l;
    int sl, pl;
    unsigned int i;
    std::array<unsigned char, MD5_SIZE> final {};
    const char *sp, *ep;
    std::array<char, PASSWORD_MAX_LEN> passwd {};
    const char* magic = "$1$";
    size_t passwd_len = 0;
    size_t pw_len = strlen(pw);

    // Refine the Salt first
    sp = salt;

    // If it starts with the magic string, then skip that
    if (!strncmp(sp, magic, strlen(magic))) {
        sp += strlen(magic);
    }

    // It stops at the first '$', max 8 chars
    for (ep = sp; *ep && *ep != '$' && ep < (sp + 8); ep++) {
        continue;
    }

    // get the length of the true salt
    sl = ep - sp;

    // Initialize MD5 context
    if (!MD5_Init(&ctx)) {
        throw std::runtime_error("MD5 initialization failed");
    }

    // The password first
    MD5_Update(&ctx, (const unsigned char*)pw, pw_len);

    // Then our magic string
    MD5_Update(&ctx, (const unsigned char*)magic, strlen(magic));

    // Then the raw salt
    MD5_Update(&ctx, (const unsigned char*)sp, (unsigned int)sl);

    // Then just as many characters of the MD5(pw,salt,pw)
    if (!MD5_Init(&ctx1)) {
        throw std::runtime_error("MD5 initialization failed");
    }
    MD5_Update(&ctx1, (const unsigned char*)pw, pw_len);
    MD5_Update(&ctx1, (const unsigned char*)sp, (unsigned int)sl);
    MD5_Update(&ctx1, (const unsigned char*)pw, pw_len);
    MD5_Final(final.data(), &ctx1);

    for (pl = (int)pw_len; pl > 0; pl -= MD5_SIZE) {
        MD5_Update(&ctx, (const unsigned char*) final.data(),
            (unsigned int)(pl > MD5_SIZE ? MD5_SIZE : pl));
    }

    // Clear sensitive data
    final.fill(0);

    // Then something really weird...
    for (i = (unsigned int)pw_len; i; i >>= 1) {
        if (i & 1) {
            MD5_Update(&ctx, final.data(), 1);
        } else {
            MD5_Update(&ctx, (const unsigned char*)pw, 1);
        }
    }

    // Now make the output string safely
    passwd_len = strlen(magic);
    if (passwd_len >= passwd.size()) {
        throw std::runtime_error("Buffer overflow risk");
    }
    memcpy(passwd.data(), magic, passwd_len + 1);

    size_t to_copy = std::min(static_cast<size_t>(sl), passwd.size() - passwd_len - 1);
    if (to_copy > 0) {
        memcpy(passwd.data() + passwd_len, sp, to_copy);
        passwd_len += to_copy;
    }

    if (passwd_len + 1 >= passwd.size()) {
        throw std::runtime_error("Buffer overflow risk");
    }
    passwd[passwd_len++] = '$';
    passwd[passwd_len] = '\0';

    MD5_Final(final.data(), &ctx);

    // 1000 iterations
    for (i = 0; i < 1000; i++) {
        if (!MD5_Init(&ctx1)) {
            throw std::runtime_error("MD5 initialization failed");
        }
        if (i & 1) {
            MD5_Update(&ctx1, (const unsigned char*)pw, pw_len);
        } else {
            MD5_Update(&ctx1, final.data(), MD5_SIZE);
        }

        if (i % 3) {
            MD5_Update(&ctx1, (const unsigned char*)sp, (unsigned int)sl);
        }

        if (i % 7) {
            MD5_Update(&ctx1, (const unsigned char*)pw, pw_len);
        }

        if (i & 1) {
            MD5_Update(&ctx1, final.data(), MD5_SIZE);
        } else {
            MD5_Update(&ctx1, (const unsigned char*)pw, pw_len);
        }

        MD5_Final(final.data(), &ctx1);
    }

    char* p = passwd.data() + passwd_len;
    size_t remaining = passwd.size() - passwd_len;

    if (remaining < 22 + 1) { // 22 chars for hash + null terminator
        throw std::runtime_error("Buffer too small for hash");
    }

    l = (final[0] << 16) | (final[6] << 8) | final[12];
    _crypt_to64(p, l, 4);
    p += 4;
    l = (final[1] << 16) | (final[7] << 8) | final[13];
    _crypt_to64(p, l, 4);
    p += 4;
    l = (final[2] << 16) | (final[8] << 8) | final[14];
    _crypt_to64(p, l, 4);
    p += 4;
    l = (final[3] << 16) | (final[9] << 8) | final[15];
    _crypt_to64(p, l, 4);
    p += 4;
    l = (final[4] << 16) | (final[10] << 8) | final[5];
    _crypt_to64(p, l, 4);
    p += 4;
    l = final[11];
    _crypt_to64(p, l, 2);
    p += 2;
    *p = '\0';

    // Clear sensitive data
    final.fill(0);

    return std::string(passwd.data());
}

}
