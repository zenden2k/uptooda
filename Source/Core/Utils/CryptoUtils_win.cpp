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

#include <cstdio>
#include <string>
#include <windows.h>
#include <Wincrypt.h>

#include "CoreUtils.h"

namespace IuCoreUtils {

typedef struct _my_blob {
    BLOBHEADER header;
    DWORD len;
    BYTE key[1];
}my_blob;


enum HashType {
    HashSha1,
    HashMd5,
    HashSha256,
    HashSha512
};

std::string GetHashText(const void * data, const size_t data_size, HashType hashType)
{
    HCRYPTPROV hProv = NULL;

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        return {};
    }

    BOOL hash_ok = FALSE;
    HCRYPTPROV hHash = NULL;
    switch (hashType) {
    case HashSha1: hash_ok = CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash); break;
    case HashMd5: hash_ok = CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash); break;
    case HashSha256: hash_ok = CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash); break;
    case HashSha512: hash_ok = CryptCreateHash(hProv, CALG_SHA_512, 0, 0, &hHash); break;
    }

    if (!hash_ok) {
        CryptReleaseContext(hProv, 0);
        return {};
    }

    if (!CryptHashData(hHash, static_cast<const BYTE *>(data), static_cast<DWORD>(data_size), 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return {};
    }

    DWORD cbHashSize = 0, dwCount = sizeof(DWORD);
    if (!CryptGetHashParam(hHash, HP_HASHSIZE, reinterpret_cast<BYTE*>(&cbHashSize), &dwCount, 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return {};
    }

    std::vector<BYTE> buffer(cbHashSize);
    if (!CryptGetHashParam(hHash, HP_HASHVAL, &buffer[0], &cbHashSize, 0)) {
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        return {};
    }

    std::ostringstream oss;

    for (std::vector<BYTE>::const_iterator iter = buffer.begin(); iter != buffer.end(); ++iter) {
        oss.fill('0');
        oss.width(2);
        oss << std::hex << static_cast<const int>(*iter);
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    return oss.str();
}

std::string GetHashTextFromFile(const std::string& filename, HashType hashType, const std::string& prefix = "", const std::string& postfix = "", int64_t offset = 0, size_t chunkSize = 0)
{
    HCRYPTPROV hProv = NULL;
    DWORD dwStatus = 0;
    BOOL bResult = FALSE;
    DWORD cbRead = 0;
    std::wstring fileNameW = IuCoreUtils::Utf8ToWstring(filename);
    HANDLE  hFile = CreateFile(fileNameW.c_str(),
        GENERIC_READ,
        FILE_SHARE_READ,
        NULL,
        OPEN_EXISTING,
        FILE_FLAG_SEQUENTIAL_SCAN,
        NULL);
    const int BUFSIZE = 32 * 1024;
    BYTE rgbFile[BUFSIZE];

    if (INVALID_HANDLE_VALUE == hFile)
    {
        dwStatus = GetLastError();
        LOG(ERROR) << "Error opening file " << filename << "\nError: " << dwStatus;
        return {};
    }

    if (!CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        dwStatus = GetLastError();
        LOG(ERROR) << "CryptAcquireContext failed: " << dwStatus;
        CloseHandle(hFile);
        return {};
    }

    BOOL hash_ok = FALSE;
    HCRYPTPROV hHash = NULL;
    switch (hashType) {
        case HashSha1: hash_ok = CryptCreateHash(hProv, CALG_SHA1, 0, 0, &hHash); break;
        case HashMd5: hash_ok = CryptCreateHash(hProv, CALG_MD5, 0, 0, &hHash); break;
        case HashSha256: hash_ok = CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash); break;
        case HashSha512: hash_ok = CryptCreateHash(hProv, CALG_SHA_512, 0, 0, &hHash); break;
    }

    if (!hash_ok) {
        dwStatus = GetLastError();
        LOG(ERROR) << "CryptCreateHash failed: " << dwStatus;
        CryptReleaseContext(hProv, 0);
        CloseHandle(hFile);
        return {};
    }

    if (!prefix.empty()) {
        if (!CryptHashData(hHash, (BYTE*)prefix.data(), prefix.size(), 0))
        {
            dwStatus = GetLastError();
            LOG(ERROR) << "CryptHashData failed: " << dwStatus;
            CryptReleaseContext(hProv, 0);
            CryptDestroyHash(hHash);
            CloseHandle(hFile);
            return {};
        }
    }

    size_t totalRead = 0;
    if (offset) {
        LARGE_INTEGER liSize;
        liSize.QuadPart = offset;
        SetFilePointerEx(hFile, liSize, nullptr, FILE_BEGIN);

        //SetFilePointer(hFile, offset, nullptr, FILE_BEGIN);
    }

    while ((bResult = ReadFile(hFile, rgbFile, chunkSize ? std::min<size_t>(chunkSize - totalRead, BUFSIZE): BUFSIZE,
        &cbRead, NULL)) != 0)
    {
        if (0 == cbRead)
        {
            break;
        }
        totalRead += cbRead;
        if (!CryptHashData(hHash, rgbFile, cbRead, 0))
        {
            dwStatus = GetLastError();
            LOG(ERROR) << "CryptHashData failed: " << dwStatus;
            CryptReleaseContext(hProv, 0);
            CryptDestroyHash(hHash);
            CloseHandle(hFile);
            return {};
        }
        if (chunkSize && totalRead >= chunkSize) {
            break;
        }
    }

    if (!postfix.empty()) {
        if (!CryptHashData(hHash, (BYTE*)postfix.data(), postfix.size(), 0))
        {
            dwStatus = GetLastError();
            LOG(ERROR) << "CryptHashData failed: " << dwStatus;
            CryptReleaseContext(hProv, 0);
            CryptDestroyHash(hHash);
            CloseHandle(hFile);
            return {};
        }
    }


    DWORD cbHashSize = 0, dwCount = sizeof(DWORD);
    if (!CryptGetHashParam(hHash, HP_HASHSIZE, (BYTE *)&cbHashSize, &dwCount, 0)) {
        dwStatus = GetLastError();
        LOG(ERROR) << "CryptGetHashParam failed: " << dwStatus;
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        CloseHandle(hFile);
        return {};
    }

    std::vector<BYTE> buffer(cbHashSize);
    if (!CryptGetHashParam(hHash, HP_HASHVAL, reinterpret_cast<BYTE*>(&buffer[0]), &cbHashSize, 0)) {
        dwStatus = GetLastError();
        LOG(ERROR) << "CryptGetHashParam failed: " << dwStatus;
        CryptDestroyHash(hHash);
        CryptReleaseContext(hProv, 0);
        CloseHandle(hFile);
        return {};
    }

    std::ostringstream oss;

    for (std::vector<BYTE>::const_iterator iter = buffer.begin(); iter != buffer.end(); ++iter) {
        oss.fill('0');
        oss.width(2);
        oss << std::hex << static_cast<const int>(*iter);
    }

    CryptDestroyHash(hHash);
    CryptReleaseContext(hProv, 0);
    CloseHandle(hFile);
    return oss.str();
}

std::string HMAC(const void* data, size_t size, const std::string& password, bool base64, DWORD AlgId = CALG_MD5 ){
    // Author : RosDevil
    // http://www.rohitab.com/discuss/topic/39777-hmac-md5sha1/
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    HCRYPTKEY hKey = 0;
    HCRYPTHASH hHmacHash = 0;
    BYTE * pbHash = 0;
    DWORD dwDataLen = 0;
    DWORD dwStatus = 0;
    HMAC_INFO HmacInfo;
    std::string res;
    int err = 0;

    ZeroMemory(&HmacInfo, sizeof(HmacInfo));

    if (AlgId == CALG_MD5){
        HmacInfo.HashAlgid = CALG_MD5;
        pbHash = new BYTE[16];
        dwDataLen = 16;
    }
    else if (AlgId == CALG_SHA1){
        HmacInfo.HashAlgid = CALG_SHA1;
        pbHash = new BYTE[20];
        dwDataLen = 20;
    }
    else{
        return 0;
    }

    ZeroMemory(pbHash, sizeof(dwDataLen));

    my_blob * kb = NULL;
    DWORD kbSize = static_cast<DWORD>(sizeof(my_blob) + password.length());

    kb = static_cast<my_blob*>(malloc(kbSize));
    if (!kb) {
        err = 1;
        goto Exit;
    }
    kb->header.bType = PLAINTEXTKEYBLOB;
    kb->header.bVersion = CUR_BLOB_VERSION;
    kb->header.reserved = 0;
    kb->header.aiKeyAlg = CALG_RC2;
    memcpy(&kb->key, password.c_str(), password.length());
    kb->len = static_cast<DWORD>(password.length());

    if (!CryptAcquireContext(&hProv, NULL, MS_ENHANCED_PROV, PROV_RSA_FULL, CRYPT_VERIFYCONTEXT | CRYPT_NEWKEYSET)){
        dwStatus = GetLastError();
        LOG(ERROR) << "CryptAcquireContext failed: " << dwStatus;
        err = 1;
        goto Exit;
    }

    if (!CryptImportKey(hProv, (BYTE*)kb, kbSize, 0, CRYPT_IPSEC_HMAC_KEY, &hKey)){
        dwStatus = GetLastError();
        LOG(ERROR) << "CryptImportKey failed: " << dwStatus;
        err = 1;
        goto Exit;
    }

    if (!CryptCreateHash(hProv, CALG_HMAC, hKey, 0, &hHmacHash)){
        dwStatus = GetLastError();
        LOG(ERROR) << "CryptCreateHash failed: " << dwStatus;
        err = 1;
        goto Exit;
    }


    if (!CryptSetHashParam(hHmacHash, HP_HMAC_INFO, (BYTE*)&HmacInfo, 0)){
        dwStatus = GetLastError();
        LOG(ERROR) << "CryptSetHashParam failed: " << dwStatus;
        err = 1;
        goto Exit;
    }

    if (!CryptHashData(hHmacHash, (BYTE*)data, static_cast<DWORD>(size), 0)){
        dwStatus = GetLastError();
        LOG(ERROR) << "CryptHashData failed: " << dwStatus;
        err = 1;
        goto Exit;
    }

    if (!CryptGetHashParam(hHmacHash, HP_HASHVAL, pbHash, &dwDataLen, 0)){
        dwStatus = GetLastError();
        LOG(ERROR) << "CryptGetHashParam failed: " << dwStatus;
        err = 1;
        goto Exit;
    }


    if (!base64)
    {
        char temp[3];
        ZeroMemory(temp, 3);
        for (unsigned int m = 0; m < dwDataLen; m++){
            sprintf(temp, "%2x", pbHash[m]);
            if (temp[1] == ' ') temp[1] = '0'; // note these two: they are two CORRECTIONS to the conversion in HEX, sometimes the Zeros are
            if (temp[0] == ' ') temp[0] = '0'; // printed with a space, so we replace spaces with zeros; (this error occurs mainly in HMAC-SHA1)
            res += temp;
        }
    } else
    {
        res = CryptoUtils::Base64EncodeRaw(reinterpret_cast<const char*>(pbHash), dwDataLen);
    }

Exit:
    free(kb);
    if (hHmacHash)
        CryptDestroyHash(hHmacHash);
    if (hKey)
        CryptDestroyKey(hKey);
    if (hHash)
        CryptDestroyHash(hHash);
    if (hProv)
        CryptReleaseContext(hProv, 0);
    delete[] pbHash;

    if (err == 1){
        return {};
    }

    return res;
}

std::string CryptoUtils::CalcMD5Hash(const void* data, size_t size)
{
    return GetHashText(data, size, HashMd5);
}

std::string CryptoUtils::CalcMD5HashFromString(const std::string& data)
{
    return GetHashText(data.data(), data.length(), HashMd5);
}

std::string CryptoUtils::CalcMD5HashFromFile(const std::string& filename) {
    return GetHashTextFromFile(filename, HashMd5);
}

std::string CryptoUtils::CalcSHA1Hash(const void* data, size_t size) {
    return GetHashText(data, size, HashSha1);
}

std::string CryptoUtils::CalcHMACSHA1Hash(const std::string& key, const void* data, size_t size, bool base64) {
    return HMAC(data, size, key, base64, CALG_SHA1);
}

std::string CryptoUtils::CalcHMACSHA1HashFromString(const std::string& key, const std::string& data, bool base64) {
    return CalcHMACSHA1Hash( key, data.data(), data.size(), base64 );
}

std::string CryptoUtils::CalcSHA1HashFromString(const std::string& data) {
    return CalcSHA1Hash( data.data(), data.size() );
}

std::string CryptoUtils::CalcSHA1HashFromFile(const std::string& filename) {
    return GetHashTextFromFile(filename, HashSha1);
}

std::string CryptoUtils::CalcSHA1HashFromFileWithPrefix(const std::string& filename, const std::string& prefix, const std::string& postfix) {
    return GetHashTextFromFile(filename, HashSha1, prefix, postfix);
}

std::string CryptoUtils::CalcSHA256Hash(const void* data, size_t size) {
    return GetHashText(data, size, HashSha256);
}

std::string CryptoUtils::CalcSHA256HashFromString(const std::string& data) {
    return CalcSHA256Hash(data.data(), data.size());
}

std::string CryptoUtils::CalcSHA256HashFromFile(const std::string& filename, int64_t offset, size_t chunkSize) {
    return GetHashTextFromFile(filename, HashSha256, {}, {}, offset, chunkSize);
}

std::string CryptoUtils::CalcSHA512Hash(const void* data, size_t size) {
    return GetHashText(data, size, HashSha512);
}

std::string CryptoUtils::CalcSHA512HashFromString(const std::string& data) {
    return CalcSHA512Hash(data.data(), data.size());
}

std::string CryptoUtils::CalcSHA512HashFromFile(const std::string& filename, int64_t offset, size_t chunkSize) {
    return GetHashTextFromFile(filename, HashSha512, {}, {}, offset, chunkSize);
}


} // end of namespace IuCoreUtils
