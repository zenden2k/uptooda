/*

    Image Uploader -  free application for uploading images/files to the Internet

    Copyright 2007-2018 Sergey Svistunov (zenden2k@yandex.ru)

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

#ifndef NOMINMAX
#define NOMINMAX
#endif 

#include "NetworkClient.h"

#include <sys/stat.h>
#include <fcntl.h>
#include <memory.h>
#include <cstdio>
#include <algorithm>

#include "Core/Utils/CoreUtils.h"
#include "Core/Utils/StringUtils.h"
#include "Core/Logging.h"
#include "CurlShare.h"

#ifdef USE_OPENSSL
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/conf.h>
#include <openssl/engine.h>
#endif

/*#ifdef _MSC_VER
    #if defined(USE_OPENSSL) 
        #pragma comment(lib, "libcurl_openssl.lib")
    #else
        #pragma comment(lib, "libcurl.lib")
    #endif
#endif*/

#if defined(_WIN32) && defined(CURL_WIN32_UTF8_FILENAMES)
    #define UTF8_FILENAME(name) name
#else
    #define UTF8_FILENAME(name) IuCoreUtils::Utf8ToSystemLocale(name)
#endif

namespace NetworkClientInternal {

size_t simple_read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
    return  fread(ptr, size, nmemb, reinterpret_cast<FILE*>(stream));
}

// We might need this function for avoiding "cannot rewind" error,
// but it is not being called when CURLFORM_STREAM is used.
// KNOWN bug in curl: https://github.com/curl/curl/issues/768
/*int simple_seek_callback(void *userp, curl_off_t offset, int origin)
{
    if (fseek(reinterpret_cast<FILE*>(userp), offset, origin)==0) {
        return CURL_SEEKFUNC_OK;
    } 
    return CURL_SEEKFUNC_CANTSEEK;
}*/

#if defined(USE_OPENSSL) 
char CertFileName[1024] = "";

/* we have this global to let the callback get easy access to it */
static std::vector<std::mutex*> lockarray;

void lock_callback(int mode, int type, char const *file, int line)
{
    (void)file;
    (void)line;
    if (mode & CRYPTO_LOCK) {
        lockarray[type]->lock();
    } else {
        lockarray[type]->unlock();
    }
}

unsigned long thread_id()
{
#ifdef _WIN32
    return ::GetCurrentThreadId();
#else
    return std::hash<std::thread::id>()(std::this_thread::get_id());
#endif
}

// TODO: remove this function
void init_locks()
{
    int i;

    lockarray.resize(CRYPTO_num_locks());
    for (i = 0; i < CRYPTO_num_locks(); i++) {
        lockarray[i] = new std::mutex();
    }

    CRYPTO_set_id_callback(thread_id);
    CRYPTO_set_locking_callback(lock_callback);
}

void kill_locks()
{
    int i;
    if (lockarray.empty()) {
        return;
    }
    CRYPTO_set_locking_callback(NULL);
    CRYPTO_set_id_callback(NULL);
    for (i = 0; i < CRYPTO_num_locks(); i++)
        delete lockarray[i];
    lockarray.clear();
}
#endif

}

int NetworkClient::set_sockopts(void * clientp, curl_socket_t sockfd, curlsocktype purpose) 
{
    #ifdef _WIN32
        // See http://support.microsoft.com/kb/823764
        NetworkClient* nm = reinterpret_cast<NetworkClient*>(clientp);
        int val = nm->m_UploadBufferSize + 32;
        setsockopt(sockfd, SOL_SOCKET, SO_SNDBUF, reinterpret_cast<const char *>(&val), sizeof(val));
    #endif
    return 0;
}

int NetworkClient::private_static_writer(char *data, size_t size, size_t nmemb, void *buffer_in)
{
    auto cbd = reinterpret_cast<CallBackData*>(buffer_in);
    NetworkClient* nm = cbd->nmanager;
    if(nm)
    {
        if(cbd->funcType == funcTypeBody)
        {
            return nm->private_writer(data, size, nmemb);
        }
        else
            return nm->private_header_writer(data, size, nmemb);
    }
    return 0;
}

void NetworkClient::setProxy(const std::string &host, int port, int type)
{
    curl_easy_setopt(curl_handle, CURLOPT_PROXY, host.c_str());
    if (port) {
        curl_easy_setopt(curl_handle, CURLOPT_PROXYPORT, static_cast<long>(port));
    }
    curl_easy_setopt(curl_handle, CURLOPT_PROXYTYPE, static_cast<long>(type));
#ifdef NDEBUG
    curl_easy_setopt(curl_handle, CURLOPT_NOPROXY, "localhost,127.0.0.1"); 
#endif
} 

void NetworkClient::setProxyUserPassword(const std::string &username, const std::string& password) {
    if (username.empty() && password.empty()) {
        curl_easy_setopt(curl_handle, CURLOPT_PROXYUSERPWD, NULL);
        curl_easy_setopt(curl_handle, CURLOPT_PROXYAUTH, NULL);
    } else {
        std::string authStr = urlEncode(username) + ":" + urlEncode(password);
        curl_easy_setopt(curl_handle, CURLOPT_PROXYUSERPWD, authStr.c_str());
        curl_easy_setopt(curl_handle, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
    }
}

void NetworkClient::clearProxy() {
    curl_easy_setopt(curl_handle, CURLOPT_PROXY, "");
    curl_easy_setopt(curl_handle, CURLOPT_PROXYUSERPWD, NULL);
    curl_easy_setopt(curl_handle, CURLOPT_PROXYAUTH, NULL);
}

int NetworkClient::private_writer(char *data, size_t size, size_t nmemb)
{
    if(!m_OutFileName.empty())
    {
        if(!m_hOutFile)
            if ((m_hOutFile = IuCoreUtils::fopen_utf8(m_OutFileName.c_str(), "wb")) == nullptr) {
                LOG(ERROR) << "Unable to create output file:" << std::endl << m_OutFileName;
                throw NetworkClient::AbortedException("Unable to create output file");
            }
               
        fwrite(data, size,nmemb, m_hOutFile);
    }
    else
        internalBuffer.append(data, size * nmemb);
    return size * nmemb;
}

int NetworkClient::private_header_writer(char *data, size_t size, size_t nmemb)
{
    m_headerBuffer.append(data, size * nmemb);
    return size * nmemb;
}

int NetworkClient::ProgressFunc(void *clientp, double dltotal, double dlnow, double ultotal, double ulnow)
{
    auto nm = reinterpret_cast<NetworkClient*>(clientp);
    if(nm && nm->m_progressCallbackFunc)
    {
        if  (nm->chunkOffset_>=0 && nm->chunkSize_>0 && nm->m_currentActionType == atUpload) {
            ultotal = static_cast<double>(nm->m_CurrentFileSize);
            ulnow = nm->chunkOffset_ + ulnow;
        } else if( ((ultotal<=0 && nm->m_CurrentFileSize>0)) && nm->m_currentActionType == atUpload)
            ultotal = double(nm->m_CurrentFileSize);
        return nm->m_progressCallbackFunc(nm, dltotal, dlnow, ultotal, ulnow);
    }
    return 0;
}

void NetworkClient::setMethod(const std::string &str)
{
    m_method = str;
}

bool  NetworkClient::_curl_init = false;

std::mutex NetworkClient::_mutex;

NetworkClient::NetworkClient()
{
    curl_init();
    enableResponseCodeChecking_ = true;
    m_hOutFile = nullptr;
    chunkOffset_ = -1;
    chunkSize_ = -1;
    m_uploadingFileReadBytes = 0;
    chunk_ = nullptr;
    curlShare_ = nullptr;
    m_CurrentFileSize = -1;
    m_uploadingFile = nullptr;
    *m_errorBuffer = 0;
    m_progressCallbackFunc = nullptr;
    curl_handle = curl_easy_init(); // Initializing libcurl
    m_bodyFuncData.funcType = funcTypeBody;
    m_bodyFuncData.nmanager = this;
    m_UploadBufferSize = 1048576;
    m_headerFuncData.funcType = funcTypeHeader;
    m_headerFuncData.nmanager = this;
    m_nUploadDataOffset = 0;
    treatErrorsAsWarnings_ = false;
    logger_ = nullptr;
    proxyProvider_ = nullptr;
    curl_easy_setopt(curl_handle, CURLOPT_COOKIELIST, "");
    m_userAgent = "Mozilla/5.0 (Windows NT 6.3; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/74.0.3729.169 Safari/537.36";

    curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, private_static_writer);
    curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, &m_bodyFuncData);    
    curl_easy_setopt(curl_handle, CURLOPT_WRITEHEADER, &m_headerFuncData);
    curl_easy_setopt(curl_handle, CURLOPT_ERRORBUFFER, m_errorBuffer);
    
    curl_easy_setopt(curl_handle, CURLOPT_PROGRESSFUNCTION, &ProgressFunc);
    curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_PROGRESSDATA, this);
    curl_easy_setopt(curl_handle, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl_handle, CURLOPT_ENCODING, "");
    curl_easy_setopt(curl_handle, CURLOPT_SOCKOPTFUNCTION, &set_sockopts);
    curl_easy_setopt(curl_handle, CURLOPT_SOCKOPTDATA, this);
    /*
    TODO: use new progress callbacks
#if LIBCURL_VERSION_NUM >= 0x072000

    curl_easy_setopt(curl_handle, CURLOPT_XFERINFOFUNCTION, xferinfo);
    curl_easy_setopt(curl_handle, CURLOPT_XFERINFODATA, file);
#endif
     */
#if defined(_WIN32) && defined(USE_OPENSSL)
    curl_easy_setopt(curl_handle, CURLOPT_CAINFO, NetworkClientInternal::CertFileName);
#endif
    //if (_is_openssl || !WinUtils::IsWine())
    {
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 1L);
        curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 2L);
    }
/*#ifdef _DEBUG   
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYPEER, 0L); 
    curl_easy_setopt(curl_handle, CURLOPT_SSL_VERIFYHOST, 0L);
#endif*/
    //We want the referrer field set automatically when following locations
    curl_easy_setopt(curl_handle, CURLOPT_AUTOREFERER, 1L); 
    curl_easy_setopt(curl_handle, CURLOPT_MAXREDIRS, 8L);
    curl_easy_setopt(curl_handle, CURLOPT_BUFFERSIZE, 32768L);
    curl_easy_setopt(curl_handle, CURLOPT_VERBOSE, 0L);
    
    /*
    // enable TCP keep-alive for this transfer
    curl_easy_setopt(curl_handle, CURLOPT_TCP_KEEPALIVE, 1L);
    // keep-alive idle time to 120 seconds
    curl_easy_setopt(curl_handle, CURLOPT_TCP_KEEPIDLE, 120L);
    // interval time between keep-alive probes: 60 seconds 
    curl_easy_setopt(curl_handle, CURLOPT_TCP_KEEPIDLE, 60L);
    */
}

NetworkClient::~NetworkClient()
{
    curl_easy_setopt(curl_handle, CURLOPT_PROGRESSFUNCTION, nullptr);
    curl_easy_cleanup(curl_handle);
#ifdef USE_OPENSSL
    ERR_remove_thread_state(nullptr);
#endif
    proxyProvider_ = nullptr;
}

void NetworkClient::addQueryParam(const std::string& name, const std::string& value)
{
    QueryParam newParam;
    newParam.name = name;
    newParam.value = value;
    newParam.isFile = false;
    m_QueryParams.push_back(newParam);
}

void NetworkClient::addQueryParamFile(const std::string& name, const std::string& fileName, const std::string& displayName, const std::string& contentType)
{
    QueryParam newParam;
    newParam.name = name;
    newParam.value = fileName;
    newParam.isFile = true;
    newParam.contentType = contentType;
    newParam.displayName= displayName;
    m_QueryParams.push_back(newParam);
}

void NetworkClient::setUrl(const std::string& url)
{
    m_url = url;
    curl_easy_setopt(curl_handle, CURLOPT_URL, url.c_str());
}

void NetworkClient::closeFileList(std::vector<FILE *>& files)
{
    for(auto f : files ) {
        fclose(f);
    }
    files.clear();
}

bool NetworkClient::doUploadMultipartData()
{
    private_initTransfer();
    std::vector<FILE *> openedFiles;

    struct curl_httppost *formpost=nullptr;
    struct curl_httppost *lastptr=nullptr;

    {
        std::vector<QueryParam>::iterator it, end = m_QueryParams.end();

        for(it=m_QueryParams.begin(); it!=end; ++it)
        {
            if(it->isFile)
            {
                    /*curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, fread);
                    curl_easy_setopt(curl_handle, CURLOPT_SEEKFUNCTION, fseek);*/
                                        std::string fileName = it->value;
                    // FIXME: > 2gb  file size & unicode filenames support on Windows
                    uint64_t  curFileSize = IuCoreUtils::getFileSize(fileName);

                    // Known bug in curl: https://github.com/curl/curl/issues/768
                    if (/*curFileSize > LONG_MAX*/  true ) { 
                        std::string ansiFileName = UTF8_FILENAME(fileName);
                        ;
#if defined(_WIN32) && !defined(CURL_WIN32_UTF8_FILENAMES)
                        //LOG(WARNING) << "Uploading files bigger than 2 GB via multipart/form-data with file names non representable in system locale is not supported";
#endif
                        if (it->contentType.empty())
                            curl_formadd(&formpost,
                            &lastptr,
                            CURLFORM_COPYNAME, it->name.c_str(),
                            CURLFORM_FILENAME, it->displayName.c_str(),
                            CURLFORM_FILE, ansiFileName.c_str(),
                            CURLFORM_END);
                        else
                            curl_formadd(&formpost,
                            &lastptr,
                            CURLFORM_COPYNAME, it->name.c_str(),
                            CURLFORM_FILENAME, it->displayName.c_str(),
                            CURLFORM_FILE, ansiFileName.c_str(),
                            CURLFORM_CONTENTTYPE, it->contentType.c_str(),
                            CURLFORM_END);
                    } /*else {
                        if (it->contentType.empty())
                            curl_formadd(&formpost,
                            &lastptr,
                            CURLFORM_COPYNAME, it->name.c_str(),
                            CURLFORM_FILENAME, it->displayName.c_str(),
                            CURLFORM_STREAM, curFile,
                            CURLFORM_CONTENTSLENGTH, static_cast<long>(curFileSize),
                            CURLFORM_END);
                        else
                            curl_formadd(&formpost,
                            &lastptr,
                            CURLFORM_COPYNAME, it->name.c_str(),
                            CURLFORM_FILENAME, it->displayName.c_str(),
                            CURLFORM_STREAM, curFile,
                            CURLFORM_CONTENTSLENGTH, static_cast<long>(curFileSize),
                            CURLFORM_CONTENTTYPE, it->contentType.c_str(),
                            CURLFORM_END);
                    } */ 
            }
            else
            {
                 curl_formadd(&formpost,
                        &lastptr,
                        CURLFORM_COPYNAME, it->name.c_str(),
                        CURLFORM_COPYCONTENTS, it->value.c_str(),
                        CURLFORM_END);
            }
        }
    }

    curl_easy_setopt(curl_handle, CURLOPT_HTTPPOST, formpost);
    m_currentActionType = atUpload;
    curl_result = curl_easy_perform(curl_handle);
    closeFileList(openedFiles);
    curl_formfree(formpost);
    return private_on_finish_request();
}

bool NetworkClient::private_on_finish_request()
{
    private_checkResponse();
    private_cleanup_after();
    private_parse_headers();
    if (curl_result != CURLE_OK)
    {
        if (curl_result == CURLE_ABORTED_BY_CALLBACK) {
            throw AbortedException("Aborted by callback");
        }
        return false; //fail
    }

    return true;
}

std::string NetworkClient::responseBody()
{
    return internalBuffer;
}

int NetworkClient::responseCode()
{
    long result=-1;
    curl_easy_getinfo(curl_handle, CURLINFO_RESPONSE_CODE, &result);
    return result;
}

void NetworkClient::addQueryHeader(const std::string& name, const std::string& value)
{
    m_QueryHeaders.emplace_back(name, value);
}

bool NetworkClient::doGet(const std::string & url)
{
    if(!url.empty())
        setUrl(url);

    private_initTransfer();
    if(!private_apply_method())
        curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 1);
    m_currentActionType = atGet;
    curl_result = curl_easy_perform(curl_handle);
    return private_on_finish_request();

}

bool NetworkClient::doPost(const std::string& data)
{
    private_initTransfer();
    if(!private_apply_method())
   curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
    std::string postData;
    std::vector<QueryParam>::iterator it, end = m_QueryParams.end();

        for(it=m_QueryParams.begin(); it!=end; ++it)
        {
            if(!it->isFile)
            {
                postData+= urlEncode(it->name)+"="+urlEncode(it->value)+"&";
            }
        }

    if(data.empty()) {
        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, postData.c_str());
    }
    else {
        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, (const char*)data.data());
        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE, (long)data.length());
    }

    m_currentActionType = atPost;    
    curl_result = curl_easy_perform(curl_handle);
    return private_on_finish_request();
}

std::string NetworkClient::urlEncode(const std::string& str)
{
    char * encoded = curl_easy_escape(curl_handle, str.c_str() , str.length() );
    std::string res = encoded;
    res+="";
    curl_free(encoded);
    return res;
}

std::string NetworkClient::urlDecode(const std::string& str) {
    char * decoded = curl_easy_unescape(curl_handle, str.c_str(), str.length(), nullptr);
    std::string res = decoded;
    res += "";
    curl_free(decoded);
    return res;
}

std::string NetworkClient::errorString()
{
    return m_errorBuffer;
}

void NetworkClient::setUserAgent(const std::string& userAgentStr)
{
    m_userAgent = userAgentStr;
}
 
void NetworkClient::private_initTransfer()
{
    private_cleanup_before();
    curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, m_userAgent.c_str());

#if defined(_WIN32) && !defined(USE_OPENSSL)
    // See https://github.com/curl/curl/issues/264
    curl_easy_setopt(curl_handle, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NO_REVOKE);
#endif
    std::vector<CustomHeaderItem>::iterator it, end = m_QueryHeaders.end();
    chunk_ = nullptr;

    for(it = m_QueryHeaders.begin(); it!=end; ++it)
    {
        if ( it->value == "\n" ) {
            chunk_ = curl_slist_append(chunk_, (it->name + ";" + it->value).c_str());
        } else {
            chunk_ = curl_slist_append(chunk_, (it->name + ": " + it->value).c_str());
        } 
        
    }

    curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, chunk_);
    if (proxyProvider_) {
        proxyProvider_->provideProxyForUrl(this, m_url);
    }
}

void NetworkClient::private_checkResponse()
{
    if ( !enableResponseCodeChecking_ && curl_result == CURLE_OK )  {
        return;
    }
    int code = responseCode();
    if ( ( !code || (code>= 400 && code<=499)) && errorString() != "Callback aborted" ) {
        std::string errorDescr;
        if (!errorLogIdString_.empty()) {
            errorDescr = errorLogIdString_ + "\r\n";
        }
        errorDescr += "Request failed, URL: '" + m_url + "'. \r\n";
        
        if (code) {
            errorDescr += "Response code: " + IuCoreUtils::toString(code) + "\r\n";
        }
        std::string errDescr = errorString();
        if (!errDescr.empty()) {
            errorDescr += errDescr  + "\r\n";
        }
        errorDescr += internalBuffer;
        if (logger_) {
            logger_->logNetworkError(!treatErrorsAsWarnings_, errorDescr);
        } else {
            (treatErrorsAsWarnings_ ? LOG(WARNING) : LOG(ERROR)) << errorDescr;
        }
    }
}

void NetworkClient::curl_init() {
    std::lock_guard<std::mutex> guard(_mutex);
    if (!_curl_init) {
#ifdef USE_OPENSSL
        NetworkClientInternal::init_locks(); // init locks should be called BEFORE curl_global_init
#endif

        curl_global_init(CURL_GLOBAL_ALL);
        //curl_version_info_data * infoData = curl_version_info(CURLVERSION_NOW);
        //_is_openssl = strstr(infoData->ssl_version, "WinSSL") != infoData->ssl_version;

#if defined(WIN32) && defined(USE_OPENSSL)
            using namespace NetworkClientInternal;
            GetModuleFileNameA(0, CertFileName, 1023);
            int i, len = lstrlenA(CertFileName);
            for (i = len; i >= 0; i--) {
                if (CertFileName[i] == '\\') {
                    CertFileName[i + 1] = 0;
                    break;
                }
            }
            strcat(CertFileName, "curl-ca-bundle.crt");
#endif
        atexit(&curl_cleanup);
        _curl_init = true;
    }
}

void NetworkClient::curl_cleanup()
{
    curl_global_cleanup(); 
#ifdef USE_OPENSSL
    #if defined(_WIN32) && !defined(OPENSSL_NO_COMP)
        SSL_COMP_free_compression_methods(); // to avoid memory leaks
    #endif
    NetworkClientInternal::kill_locks();
#endif
}

std::string NetworkClient::responseHeaderText()
{
    return m_headerBuffer;
}

void NetworkClient::setProgressCallback(const ProgressCallback& func)
{
    m_progressCallbackFunc = func;
}

void NetworkClient::private_parse_headers()
{
    auto headers = IuStringUtils::SplitSV(m_headerBuffer, "\n");

    for (const auto &it: headers) {
        auto parts = IuStringUtils::SplitSV(it, ":", 2);

        if (parts.size() == 2) {
            std::string name(IuStringUtils::TrimSV(parts[0]));
            std::string value(IuStringUtils::TrimSV(parts[1]));
            m_ResponseHeaders.emplace_back(name, value);
        }
    }
}

std::string NetworkClient::responseHeaderByName(const std::string& name)
{
    std::vector<CustomHeaderItem>::iterator it, end = m_ResponseHeaders.end();
    
    for(it = m_ResponseHeaders.begin(); it!=end; ++it)
    {
        if(it->name == name)
            return it->value;
    }
    return std::string();

}

int NetworkClient::responseHeaderCount()
{
    return m_ResponseHeaders.size();
}

std::string NetworkClient::responseHeaderByIndex(int index, std::string& name)
{
    if (index >= 0 && static_cast<size_t>(index) < m_ResponseHeaders.size()) {
        name = m_ResponseHeaders[index].name;
        return m_ResponseHeaders[index].value;
    }
    return std::string();
}

void NetworkClient::private_cleanup_before()
{
    m_nUploadDataOffset = 0;
    std::vector<CustomHeaderItem>::iterator it, end = m_QueryHeaders.end();

    bool add = true;
    for(it = m_QueryHeaders.begin(); it!=end; ++it)
    {
        if(it->name == "Expect" ) { add = false; break; }
    }
    addQueryHeader("Expect", "");
    m_ResponseHeaders.clear();
    internalBuffer.clear();
    m_headerBuffer.clear();
}

void NetworkClient::private_cleanup_after()
{
    m_currentActionType = atNone;
    m_QueryHeaders.clear();
    m_QueryParams.clear();
    if(m_hOutFile)
    {
        fclose(m_hOutFile);
        m_hOutFile = nullptr;
    }
    m_OutFileName.clear();
    m_method.clear();
	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, nullptr);
	curl_easy_setopt(curl_handle, CURLOPT_MIMEPOST, nullptr);
	curl_easy_setopt(curl_handle, CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(-1));
	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(-1));
	curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, nullptr);
	curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST, nullptr);
	curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, 0L);
	curl_easy_setopt(curl_handle, CURLOPT_NOBODY, 0L);
	curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 1L);
	curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, nullptr);
	curl_easy_setopt(curl_handle, CURLOPT_SEEKFUNCTION, nullptr);
	curl_easy_setopt(curl_handle, CURLOPT_SEEKDATA, nullptr);
	curl_easy_setopt(curl_handle, CURLOPT_READDATA, stdin);

    m_uploadData.clear();
    m_uploadingFile = nullptr;
    chunkOffset_ = -1;
    chunkSize_ = -1;
    enableResponseCodeChecking_ = true;
    m_nUploadDataOffset = 0;
    /*curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, 0L);
    curl_easy_setopt(curl_handle, CURLOPT_READDATA, 0L);*/
    if(chunk_)
    {
        curl_slist_free_all(chunk_);
        chunk_ = nullptr;
    }
}

size_t NetworkClient::read_callback(void *ptr, size_t size, size_t nmemb, void *stream)
{
    auto nm = reinterpret_cast<NetworkClient*>(stream);;
    if(!nm) {
        return 0;
    }
    return nm->private_read_callback(ptr, size, nmemb, stream);
} 

size_t NetworkClient::private_read_callback(void *ptr, size_t size, size_t nmemb, void *)
{
    size_t retcode;
   
    if (m_uploadingFile) {
        int64_t pos = IuCoreUtils::ftell_64(m_uploadingFile);
        if (chunkOffset_ != -1 && pos >= chunkOffset_ + m_currentUploadDataSize) {
            return 0;
        }
        retcode = fread(ptr, size, nmemb, m_uploadingFile);
        if (chunkOffset_ != -1) {
            retcode = std::min<>((int64_t)retcode, chunkOffset_ + m_currentUploadDataSize - pos);
        }
        m_uploadingFileReadBytes += retcode;
    } else
    {
        int wantsToRead = size * nmemb;
        // dont even try to remove "<>" brackets!!
        int canRead = std::min<>((int)m_uploadData.size()-m_nUploadDataOffset, (int)wantsToRead);
        memcpy(ptr, m_uploadData.data() + m_nUploadDataOffset, canRead);
        m_nUploadDataOffset += canRead;
        retcode = canRead;
    }
    return retcode;
}

int NetworkClient::private_seek_callback(void *userp, curl_off_t offset, int origin) {
    auto nc = reinterpret_cast<NetworkClient*>(userp);

    if (nc->m_uploadingFile) {
        int64_t newOffset = offset;
        int newOrigin = origin;
        if (origin == SEEK_SET && nc->chunkOffset_ != -1) {
            newOffset = nc->chunkOffset_ + offset;
        } else if (origin == SEEK_END) {
            // not implemented
            return CURL_SEEKFUNC_CANTSEEK;
        }
        return IuCoreUtils::fseek_64(nc->m_uploadingFile, newOffset, newOrigin);
    } else {
        if (origin == SEEK_SET) {
            if (offset < 0 || offset>= nc->m_uploadData.size()) {
                return CURL_SEEKFUNC_CANTSEEK;
            }
            nc->m_nUploadDataOffset = offset;
        } else if (origin == SEEK_CUR) {
            nc->m_nUploadDataOffset += offset;
        } else {
            // not implemented
            return CURL_SEEKFUNC_CANTSEEK;
        }
        return CURL_SEEKFUNC_OK;
    }
}


bool NetworkClient::doUpload(const std::string& fileName, const std::string &data)
{
    if(!fileName.empty())
    {
        m_uploadingFile = IuCoreUtils::fopen_utf8(fileName.c_str(), "rb"); /* open file to upload */
        if(!m_uploadingFile) 
        {
            LOG(ERROR)<< "Failed to open file '" << fileName << "'";
            return false; /* can't continue */
        }
        m_CurrentFileSize = IuCoreUtils::getFileSize(fileName);
        m_currentUploadDataSize = m_CurrentFileSize;
        if(m_CurrentFileSize < 0) {
            fclose(m_uploadingFile);
            return false;
        }
            
        if ( chunkSize_  >0 && chunkOffset_ >= 0 ) {
            m_currentUploadDataSize = chunkSize_;
            if ( IuCoreUtils::fseek_64(m_uploadingFile, chunkOffset_, SEEK_SET)) {
            }
        }
        m_uploadingFileReadBytes = 0;
        m_currentActionType = atUpload;
    }
    else
    {
        m_nUploadDataOffset =0;
        m_uploadData = data;
        m_CurrentFileSize = data.length();
        m_currentUploadDataSize = m_CurrentFileSize;
        m_currentActionType = atPost;
    }
    curl_easy_setopt(curl_handle, CURLOPT_READFUNCTION, read_callback);
    curl_easy_setopt(curl_handle, CURLOPT_SEEKFUNCTION, private_seek_callback);
    curl_easy_setopt(curl_handle, CURLOPT_SEEKDATA, this);
    if (!private_apply_method()) {
        curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
    }
    curl_easy_setopt(curl_handle,CURLOPT_POSTFIELDS, NULL);
    curl_easy_setopt(curl_handle, CURLOPT_READDATA, this);
    
    /*if ( m_method != "PUT" ) {
        addQueryHeader("Content-Length", IuCoreUtils::int64_tToString(m_currentUploadDataSize));
    }*/
    private_initTransfer();

	curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDSIZE_LARGE, static_cast<curl_off_t>(m_currentUploadDataSize));
    curl_easy_setopt(curl_handle, CURLOPT_INFILESIZE_LARGE, static_cast<curl_off_t>(m_currentUploadDataSize));
    
    curl_result = curl_easy_perform(curl_handle);
    if(m_uploadingFile)
         fclose(m_uploadingFile);
    bool res = private_on_finish_request();
    return res;
}

bool NetworkClient::private_apply_method()
{
    curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST,NULL);
    curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, 0L);
    if(m_method == "POST")
        curl_easy_setopt(curl_handle, CURLOPT_POST, 1L);
    else     if(m_method == "GET")
        curl_easy_setopt(curl_handle, CURLOPT_HTTPGET, 1L);
    else if (m_method == "PUT")
            curl_easy_setopt(curl_handle, CURLOPT_UPLOAD, 1L);
    else if (!m_method.empty())
    {
        curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST,m_method.c_str());
    }
    else 
    {
        curl_easy_setopt(curl_handle, CURLOPT_CUSTOMREQUEST,NULL);
        return false;
    }
    return true;

}

void NetworkClient::setReferer(const std::string &str)
{
    curl_easy_setopt(curl_handle, CURLOPT_REFERER, str.c_str());
}

int NetworkClient::getCurlResult()
{
    return curl_result;
}
CURL* NetworkClient::getCurlHandle()
{
    return curl_handle;
}

void NetworkClient::setOutputFile(const std::string &str)
{
    m_OutFileName = str;
}

void NetworkClient::setUploadBufferSize(int size)
{
    m_UploadBufferSize = size;
}

void NetworkClient::setChunkOffset(double offset)
{
    chunkOffset_ = static_cast<uint64_t>(offset);
}

void NetworkClient::setChunkSize(double size)
{
    chunkSize_ = static_cast<uint64_t>(size);
}

void NetworkClient::setTreatErrorsAsWarnings(bool treat)
{
    treatErrorsAsWarnings_ = treat;
}

std::string NetworkClient::getCurlResultString()
{
    const char * str = curl_easy_strerror(curl_result);
    std::string res(str);
    res+="";
    //curl_free(str);
    return res;
}


void NetworkClient::setCurlShare(CurlShare* share)
{
    curlShare_ = share;
    curl_easy_setopt(curl_handle, CURLOPT_SHARE, share->getHandle());
}

void NetworkClient::enableResponseCodeChecking(bool enable)
{
    enableResponseCodeChecking_ = enable;
}

void NetworkClient::setErrorLogId(const std::string& str) {
    errorLogIdString_ = str;
}

void NetworkClient::setLogger(Logger* logger) {
    logger_ = logger;
}

void NetworkClient::setProxyProvider(std::shared_ptr<ProxyProvider> provider) {
    proxyProvider_ = provider;
}

void NetworkClient::setCurlOption(int option, const std::string &value) {
    curl_easy_setopt(curl_handle, static_cast<CURLoption>(option), value.c_str());
}

void NetworkClient::setCurlOptionInt(int option, long value) {
    curl_easy_setopt(curl_handle, static_cast<CURLoption>(option), value);
}

std::string NetworkClient::getCurlInfoString(int option)
{
    char* buf = nullptr;
    curl_easy_getinfo(curl_handle, static_cast<CURLINFO>(option), &buf);
    std::string res = buf ? buf : std::string();
    return res;
}

int NetworkClient::getCurlInfoInt(int option)
{
    long res = 0;
    curl_easy_getinfo(curl_handle, static_cast<CURLINFO>(option), &res);
    return res;
}

double NetworkClient::getCurlInfoDouble(int option)
{
    double res = 0;
    curl_easy_getinfo(curl_handle, static_cast<CURLINFO>(option), &res);
    return res;
}

void NetworkClient::setTimeout(uint32_t timeout)
{
    curl_easy_setopt(curl_handle, CURLOPT_TIMEOUT, static_cast<long>(timeout));
}

void NetworkClient::setConnectionTimeout(uint32_t connection_timeout)
{
    curl_easy_setopt(curl_handle, CURLOPT_CONNECTTIMEOUT, static_cast<long>(connection_timeout));
}
