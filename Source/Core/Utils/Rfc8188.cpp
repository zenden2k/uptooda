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

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <memory>
#include <stdexcept>
#include <vector>

#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/rand.h>

namespace IuCoreUtils::CryptoUtils {

namespace {

constexpr size_t RFC8188_KEY_LENGTH = 16;
constexpr size_t RFC8188_NONCE_LENGTH = 12;
constexpr size_t RFC8188_TAG_LENGTH = 16;
constexpr size_t RFC8188_RECORD_SIZE = 64 * 1024;
constexpr size_t RFC8188_PLAINTEXT_RECORD_SIZE = RFC8188_RECORD_SIZE - RFC8188_TAG_LENGTH - 1;

std::vector<unsigned char> DeriveHkdf(const std::string& key, const std::string& salt, const char* info,
                                      size_t outputLength) {
    using PkeyContext = std::unique_ptr<EVP_PKEY_CTX, decltype(&EVP_PKEY_CTX_free)>;
    PkeyContext context(EVP_PKEY_CTX_new_id(EVP_PKEY_HKDF, nullptr), EVP_PKEY_CTX_free);
    if (!context || EVP_PKEY_derive_init(context.get()) <= 0
        || EVP_PKEY_CTX_hkdf_mode(context.get(), EVP_PKEY_HKDEF_MODE_EXTRACT_AND_EXPAND) <= 0
        || EVP_PKEY_CTX_set_hkdf_md(context.get(), EVP_sha256()) <= 0
        || EVP_PKEY_CTX_set1_hkdf_salt(context.get(), reinterpret_cast<const unsigned char*>(salt.data()),
                                       static_cast<int>(salt.size()))
            <= 0
        || EVP_PKEY_CTX_set1_hkdf_key(context.get(), reinterpret_cast<const unsigned char*>(key.data()),
                                      static_cast<int>(key.size()))
            <= 0
        || EVP_PKEY_CTX_add1_hkdf_info(context.get(), reinterpret_cast<const unsigned char*>(info),
                                       static_cast<int>(strlen(info) + 1))
            <= 0) {
        throw std::runtime_error("RFC 8188 HKDF initialization failed");
    }

    std::vector<unsigned char> result(outputLength);
    size_t derivedLength = outputLength;
    if (EVP_PKEY_derive(context.get(), result.data(), &derivedLength) <= 0 || derivedLength != outputLength) {
        throw std::runtime_error("RFC 8188 HKDF derivation failed");
    }
    return result;
}

std::string EncryptRecord(const char* data, size_t dataLength, bool isLast, uint32_t sequenceNumber,
                          const std::vector<unsigned char>& contentKey, const std::vector<unsigned char>& nonceBase) {
    std::vector<unsigned char> plaintext(isLast ? dataLength + 1 : RFC8188_RECORD_SIZE - RFC8188_TAG_LENGTH);
    if (dataLength != 0) {
        memcpy(plaintext.data(), data, dataLength);
    }
    plaintext[dataLength] = isLast ? 2 : 1;

    std::array<unsigned char, RFC8188_NONCE_LENGTH> nonce { };
    std::copy(nonceBase.begin(), nonceBase.end(), nonce.begin());
    nonce[8] ^= static_cast<unsigned char>((sequenceNumber >> 24) & 0xff);
    nonce[9] ^= static_cast<unsigned char>((sequenceNumber >> 16) & 0xff);
    nonce[10] ^= static_cast<unsigned char>((sequenceNumber >> 8) & 0xff);
    nonce[11] ^= static_cast<unsigned char>(sequenceNumber & 0xff);

    using CipherContext = std::unique_ptr<EVP_CIPHER_CTX, decltype(&EVP_CIPHER_CTX_free)>;
    CipherContext context(EVP_CIPHER_CTX_new(), EVP_CIPHER_CTX_free);
    if (!context || EVP_EncryptInit_ex(context.get(), EVP_aes_128_gcm(), nullptr, nullptr, nullptr) != 1
        || EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_GCM_SET_IVLEN, static_cast<int>(nonce.size()), nullptr) != 1
        || EVP_EncryptInit_ex(context.get(), nullptr, nullptr, contentKey.data(), nonce.data()) != 1) {
        throw std::runtime_error("RFC 8188 AES-GCM initialization failed");
    }

    std::string result(plaintext.size() + RFC8188_TAG_LENGTH, '\0');
    int encryptedLength = 0;
    if (EVP_EncryptUpdate(context.get(), reinterpret_cast<unsigned char*>(result.data()), &encryptedLength,
                          plaintext.data(), static_cast<int>(plaintext.size()))
        != 1) {
        throw std::runtime_error("RFC 8188 AES-GCM encryption failed");
    }
    int finalLength = 0;
    if (EVP_EncryptFinal_ex(context.get(), reinterpret_cast<unsigned char*>(result.data()) + encryptedLength,
                            &finalLength)
            != 1
        || EVP_CIPHER_CTX_ctrl(context.get(), EVP_CTRL_GCM_GET_TAG, RFC8188_TAG_LENGTH,
                               result.data() + encryptedLength + finalLength)
            != 1) {
        throw std::runtime_error("RFC 8188 AES-GCM finalization failed");
    }
    result.resize(encryptedLength + finalLength + RFC8188_TAG_LENGTH);
    return result;
}

} // namespace

std::string CryptographicRandomBytes(int length) {
    constexpr int MAX_RANDOM_BYTES = 1024 * 1024;
    if (length < 0 || length > MAX_RANDOM_BYTES) {
        throw std::invalid_argument("Random byte count must be between 0 and 1048576");
    }

    std::string result(length, '\0');
    if (length != 0 && RAND_bytes(reinterpret_cast<unsigned char*>(result.data()), length) != 1) {
        throw std::runtime_error("RAND_bytes failed");
    }
    return result;
}

std::string EncryptRfc8188(const std::string& data, const std::string& key) {
    if (key.size() != RFC8188_KEY_LENGTH) {
        throw std::invalid_argument("RFC 8188 key must be exactly 16 bytes");
    }

    const std::string salt = CryptographicRandomBytes(RFC8188_KEY_LENGTH);
    const auto contentKey = DeriveHkdf(key, salt, "Content-Encoding: aes128gcm", RFC8188_KEY_LENGTH);
    const auto nonceBase = DeriveHkdf(key, salt, "Content-Encoding: nonce", RFC8188_NONCE_LENGTH);

    const size_t recordCount
        = data.empty() ? 0 : (data.size() + RFC8188_PLAINTEXT_RECORD_SIZE - 1) / RFC8188_PLAINTEXT_RECORD_SIZE;
    std::string result;
    result.reserve(21 + data.size() + recordCount * (RFC8188_TAG_LENGTH + 1));
    result.append(salt);
    result.push_back(static_cast<char>((RFC8188_RECORD_SIZE >> 24) & 0xff));
    result.push_back(static_cast<char>((RFC8188_RECORD_SIZE >> 16) & 0xff));
    result.push_back(static_cast<char>((RFC8188_RECORD_SIZE >> 8) & 0xff));
    result.push_back(static_cast<char>(RFC8188_RECORD_SIZE & 0xff));
    result.push_back('\0');

    for (size_t recordIndex = 0; recordIndex < recordCount; ++recordIndex) {
        if (recordIndex > UINT32_MAX) {
            throw std::length_error("RFC 8188 record sequence number exceeds the supported limit");
        }
        const size_t offset = recordIndex * RFC8188_PLAINTEXT_RECORD_SIZE;
        const size_t length = std::min(RFC8188_PLAINTEXT_RECORD_SIZE, data.size() - offset);
        result += EncryptRecord(data.data() + offset, length, recordIndex + 1 == recordCount,
                                static_cast<uint32_t>(recordIndex), contentKey, nonceBase);
    }
    return result;
}

} // namespace IuCoreUtils::CryptoUtils
