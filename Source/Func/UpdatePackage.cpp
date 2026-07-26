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

#include "UpdatePackage.h"

#include <boost/format.hpp>
#include <ctime>

#include <json/value.h>
#include <json/writer.h>
#include "atlheaders.h"
#include "3rdpart/Unzipper.h"
#include "Core/Utils/StringUtils.h"
#include "Core/Utils/CryptoUtils.h"
#include "WinUtils.h"
#include "IuCommonFunctions.h"
#include "Core/Utils/SystemUtils.h"
#include "Core/CommonDefs.h"
#include "Core/ServiceLocator.h"
#include "Core/AppRuntimeInfo.h"
#include "Core/i18n/Translator.h"


#ifdef IU_CLI
    #undef TR
    #define TR(str) _T(str)
#endif

CUpdateInfo::CUpdateInfo():
    m_CoreUpdate(false),
    m_ManualUpdate(false),
    m_TimeStamp(0)
{

}

bool CUpdateInfo::LoadUpdateFromFile(const CString& filename)
{
    SimpleXml xml;
    if(!xml.LoadFromFile(W2U(filename)))
    {
        ServiceLocator::instance()->logger()->write(ILogger::logError, _T("Update Engine"), CString(_T("Failed to load update file ")) + filename + _T("\r\n"));
        return false;
    }
    m_FileName = filename;
    Parse(xml);
    return true;
}

bool CUpdateInfo::SaveToFile(const CString& filename) const
{
    FILE *f = _wfopen(filename, _T("wb"));
    if(!f) return false;

    fwrite(m_Buffer.c_str(), 1, m_Buffer.size(), f);
    fclose(f);
    return true;
}

bool CUpdateInfo::LoadUpdateFromBuffer(const std::string& buffer)
{
    SimpleXml m_xml;
    if(!m_xml.LoadFromString(buffer))
    {
        ServiceLocator::instance()->logger()->write(ILogger::logError, _T("Update Engine"), CString(_T("Failed to load update file \r\n")) + _T("\r\nServer answer:\r\n") + U2W(buffer));
        return false;
    }
    m_Buffer = buffer;

    Parse(m_xml);

    return true;
}

bool CUpdateInfo::DoUpdate(const CUpdateInfo &newPackage)
{
    //Comparing this package with newPackage and performing updates
    return true;
}

bool CUpdateInfo::Parse( SimpleXml &xml)
{
    SimpleXmlNode root = xml.getRoot("UpdateInfo", false);
    if(root.IsNull()) return false;

    m_PackageName = IuCoreUtils::Utf8ToWstring(root.Attribute("Name")).c_str();
    m_UpdateUrl = IuCoreUtils::Utf8ToWstring(root.Attribute("UpdateUrl")).c_str();
    m_DownloadUrl = IuCoreUtils::Utf8ToWstring(root.Attribute("DownloadUrl")).c_str();
    m_DownloadPage = IuCoreUtils::Utf8ToWstring(root.Attribute("DownloadPage")).c_str();
    m_ManualUpdate = root.AttributeInt("ManualUpdate")!=0;
    m_Hash = IuCoreUtils::Utf8ToWstring(root.Attribute("Hash")).c_str();
    m_TimeStamp = root.AttributeInt("TimeStamp");
    m_DisplayName = IuCoreUtils::Utf8ToWstring(root.Attribute("DisplayName")).c_str();

    if (m_PackageName.IsEmpty() || m_UpdateUrl.IsEmpty() || !m_TimeStamp)
        return false;

    if (!m_ManualUpdate) {
        if (m_DownloadUrl.IsEmpty() || m_Hash.IsEmpty()) {
            return false;
        }
    } else {
        if (m_DownloadPage.IsEmpty()) {
            return false;
        }
    }
    int core = 0;
    core = root.AttributeInt("CoreUpdate");
    m_CoreUpdate = core!=0;

        //if(m_xml.FindElem(_T("Info")))
    //    {
    m_ReadableText = IuCoreUtils::Utf8ToWstring(root["Info"].Text()).c_str();
            //m_xml.GetData(m_ReadableText);
            m_ReadableText.Replace(_T("\n"),_T("\r\n"));
        //}

    return true;
}

bool CUpdateInfo::CanUpdate(const CUpdateInfo& newInfo) const
{
    if(m_TimeStamp >= newInfo.m_TimeStamp) return false;
    if(newInfo.m_PackageName != m_PackageName) return false;
    return true;
}

bool CUpdateInfo::CheckUpdates()
{
    return true;
}

CString CUpdateInfo::getHash() const
{
    return m_Hash;
}

bool CUpdateInfo::operator< (const CUpdateInfo& p) const {
    return m_TimeStamp < p.m_TimeStamp;
}

bool CUpdateInfo::isManualUpdate() const {
    return m_ManualUpdate;
}

bool CUpdateInfo::isCoreUpdate() const
{
    return m_CoreUpdate;
}

CString CUpdateInfo::displayName() const
{
    return m_DisplayName;
}

CString CUpdateInfo::downloadUrl() const
{
    return m_DownloadUrl;
}

CString CUpdateInfo::downloadPage() const {
    return m_DownloadPage;
}

CString CUpdateInfo::fileName() const
{
    return m_FileName;
}

CString CUpdateInfo::packageName() const
{
    return m_PackageName;
}

CString CUpdateInfo::readableText() const
{
    return m_ReadableText;
}

CString CUpdateInfo::updateUrl() const
{
    return m_UpdateUrl;
}

int CUpdateInfo::timeStamp() const
{
    return m_TimeStamp;
}

void CUpdateInfo::setFileName(const CString & name)
{
    m_FileName = name;
}

bool CUpdateManager::CheckUpdates()
{
    m_ErrorStr.Empty();
    Clear();
    std::vector<CString> fileList;
    WinUtils::GetFolderFileList(fileList, IuCommonFunctions::GetDataFolder() + _T("Update"), _T("*.xml"));

    bool Result = true;

    m_localUpdateInfo.clear();
    if (fileList.empty()) {
        m_ErrorStr = "No update files found in folder '" + IuCommonFunctions::GetDataFolder() + _T("Update\\'");
        return false;
    }

    size_t failed = 0;
    for (size_t i = 0; i < fileList.size(); i++) {
        CString fileName = IuCommonFunctions::GetDataFolder() + _T("Update\\") + fileList[i];
        if (!internal_load_update(fileName))
            failed++;
    }

    if (failed == fileList.size()) {
        return false;
    }
    return Result;
}

CString CUpdateManager::ErrorString() const
{
    return m_ErrorStr;
}

bool CUpdateManager::DoUpdates()
{
    nCurrentIndex = 0;
    for (size_t i = 0; i < m_updateList.size(); i++) {
        if (m_stop) {
            return false;
        }
        nCurrentIndex = i;
        internal_do_update(m_updateList[i]);
    }
    return true;
}

bool CUpdateManager::internal_load_update(CString name)
{
    CUpdateInfo localPackage;

    if (!localPackage.LoadUpdateFromFile(name)) {
        ServiceLocator::instance()->logger()->write(ILogger::logError, _T("Update Engine"), CString(_T("Could not download the update file '")) + name);
        return false;
    }

    m_localUpdateInfo.push_back(localPackage);
    CUpdateInfo remotePackage;
    auto nm = networkClientFactory_->create();
    nm->setTreatErrorsAsWarnings(true);
    nm->enableResponseCodeChecking(false);
    using namespace std::placeholders;
    nm->setProgressCallback(std::bind(&CUpdateManager::progressCallback, this, _1, _2, _3, _4, _5));

    CString url = localPackage.updateUrl();
    Json::Value request;

    const AppRuntimeInfo::AppVersionInfo* ver = AppRuntimeInfo::instance()->GetAppVersion();
    Json::Value version;
    version["FullString"] = ver->FullVersion;
    version["Minor"] = ver->Minor;
    version["Major"] = ver->Major;
    version["Build"] = ver->Build;
    version["Release"] = ver->Release;
    version["BuildDate"] = ver->BuildDate;
    version["CurlWithOpenSSL"] = ver->CurlWithOpenSSL;
    version["Is64Bit"] = sizeof(void*) == 8;
#if defined(_M_ARM64)
    version["Architecture"] = "ARM64";
#elif defined(_M_X64)
    version["Architecture"] = "x64";
#elif defined(_M_IX86)
    version["Architecture"] = "x86";
#endif

    ITranslator* translator = ServiceLocator::instance()->translator();
    if (translator != nullptr) {
        version["Language"] = translator->getCurrentLanguage();
        version["Locale"] = translator->getCurrentLocale();
    }
    request["AppVersion"] = version;

    Json::Value environment;
    Json::Value OS;
    OS["Name"] = IuCoreUtils::GetOsName();
    OS["Version"] = IuCoreUtils::GetOsVersion();
    OS["Is64Bit"] = IuCoreUtils::IsOs64Bit();
    environment["OS"] = OS;
    environment["CPUFeatures"] = IuCoreUtils::GetCpuFeatures();
    request["Environment"] = environment;

    Json::Value package;
    package["Name"] = W2U(localPackage.packageName());
    package["LocalTimestamp"] = localPackage.timeStamp();

    request["Package"] = package;
    nm->addQueryHeader("Content-Type", "application/json");

    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "None";
    builder["indentation"] = "   ";

    std::string jsonString = Json::writeString(builder, request);
    nm->setUserAgent(APP_NAME_A + std::string(" ") + ver->FullVersion);
    try {
        nm->setUrl(W2U(url));
        nm->doPost(jsonString);
    } catch ( NetworkClient::AbortedException&) {
        return false;
    }

    std::string contentType = nm->responseHeaderByName("Content-Type");
    std::vector<std::string_view> tokens = IuStringUtils::SplitSV(contentType, ";");

    if (nm->responseCode() != 200 || tokens.empty() || tokens[0] != "text/xml")
    {
        ServiceLocator::instance()->logger()->write(ILogger::logWarning, _T("Update Engine"), _T("Error while loading package ") + localPackage.packageName() + CString(_T("\r\nHTTP response code: ")) + WinUtils::IntToStr(nm->responseCode()) + _T("\r\n") + IuCoreUtils::Utf8ToWstring(nm->errorString()).c_str(), CString("URL=") + url);
        return false;
    }

    if (!remotePackage.LoadUpdateFromBuffer(nm->responseBody()))
    {
        return false;
    }

    if (!localPackage.CanUpdate(remotePackage)) {
        return true;
    }
    remotePackage.setFileName(localPackage.fileName());

    if (remotePackage.isManualUpdate()) {
        m_manualUpdatesList.push_back(remotePackage);
    }
    else {
        if (remotePackage.isCoreUpdate()) {
            m_nCoreUpdates++;
        }

        m_updateList.push_back(remotePackage);
    }


    return true;
}

bool CUpdateManager::AreCoreUpdates() const
{
    return m_nCoreUpdates != 0;
}

bool CUpdateManager::AreManualUpdates() const {
    return !m_manualUpdatesList.empty();
}

bool CUpdateManager::AreAutoUpdates() const {
    return !m_updateList.empty();
}

bool CUpdateManager::internal_do_update(CUpdateInfo& ui)
{
    CString filename = m_TempDirectory + ui.packageName() +_T(".zip");
    std::string filenamea = W2U(filename);
    nm_->setOutputFile( filenamea);
    m_statusCallback->updateStatus(nCurrentIndex, TR("Downloading file ")+ ui.downloadUrl());

    try {
        nm_->doGet(W2U(ui.downloadUrl()));
    } catch (NetworkClient::AbortedException&) {
        return false;
    }

    if(nm_->responseCode() != 200)
    {
        ServiceLocator::instance()->logger()->write(ILogger::logError, _T("Update Engine"), TR("Error while updating component ") + ui.packageName() + CString(_T("\r\nHTTP response code: ")) + WinUtils::IntToStr(nm_->responseCode())+ _T("\r\n") + IuCoreUtils::Utf8ToWstring(nm_->errorString()).c_str(), CString("URL=") + ui.downloadUrl());
        return false;
    }

    CString hash = ui.getHash();
    hash.MakeLower();
    if( hash != IuCoreUtils::Utf8ToWstring(IuCoreUtils::CryptoUtils::CalcMD5HashFromFile(IuCoreUtils::WstringToUtf8((LPCTSTR)filename))).c_str() || ui.getHash().IsEmpty())
    {
        updateStatus(nCurrentIndex, CString(TR("MD5 check of the update package failed ")) + IuCoreUtils::ExtractFileName(IuCoreUtils::WstringToUtf8((LPCTSTR)filename)).c_str());
        return false;
    }

    CUnzipper unzipper(filename);
    CString unzipFolder = m_TempDirectory + ui.packageName();
    if(!unzipper.UnzipTo(unzipFolder))
    {
        std::wstring errorMessage = str(boost::wformat(TR("Unable to unpack archive %s")) % filename.GetString());
        updateStatus(nCurrentIndex, errorMessage.c_str());
        return false;
    }

    CUpdatePackage updatePackage;
    updatePackage.setUpdateStatusCallback(this);
    if(!updatePackage.LoadUpdateFromFile(unzipFolder + _T("\\")+_T("package.xml")))
    {
        std::wstring errorMessage = str(boost::wformat(TR("Could not read package %s")) % ui.packageName().GetString());
        updateStatus(nCurrentIndex, errorMessage.c_str());
        return false;
    }

    if(!updatePackage.doUpdate())
        return false;
    CString finishText;
    finishText.Format(TR("Update finished. Updated %d of %d files "), updatePackage.updatedFileCount(), updatePackage.totalFileCount());
    m_statusCallback->updateStatus(nCurrentIndex, finishText );

    ui.SaveToFile(ui.fileName());
    m_nSuccessPackageUpdates++;
    return true;
}


CUpdatePackage::CUpdatePackage()
{
    m_statusCallback = nullptr;
    m_nUpdatedFiles = 0;
    m_nTotalFiles = 0;
    m_CoreUpdate = false;
    m_TimeStamp = 0;
}

bool CUpdatePackage::LoadUpdateFromFile(const CString& filename) {
    if (!IuCoreUtils::FileExists(W2U(filename))) {
        return false;
    }

    if (!m_xml.LoadFromFile(W2U(filename))) {
        ServiceLocator::instance()->logger()->write(ILogger::logError, _T("Update Engine"),
                                                    CString(_T("Failed to load update file \'")) +
                                                    WinUtils::DoExtractFileName(filename));
        return false;
    }

    m_PackageFolder = IuCoreUtils::Utf8ToWstring(IuCoreUtils::ExtractFilePath(W2U(filename))).c_str();
    m_PackageFolder += "\\";
    SimpleXmlNode root = m_xml.getRoot("UpdatePackage", false);
    if (root.IsNull()) return false;

    m_TimeStamp = root.AttributeInt("TimeStamp");

    int core = root.AttributeInt("CoreUpdate");

    m_CoreUpdate = (core != 0);

    SimpleXmlNode entry = root["Entries"];
    std::vector<SimpleXmlNode> entries;
    entry.GetChilds("Entry", entries);

    for (size_t i = 0; i < entries.size(); i++) {
        CUpdateItem ui;
        ui.name = IuCoreUtils::Utf8ToWstring(entries[i].Attribute("Name")).c_str();
        ui.hash = IuCoreUtils::Utf8ToWstring(entries[i].Attribute("Hash")).c_str();
        ui.saveTo = IuCoreUtils::Utf8ToWstring(entries[i].Attribute("SaveTo")).c_str();
        ui.action = IuCoreUtils::Utf8ToWstring(entries[i].Attribute("Action")).c_str();
        ui.flags = entries[i].Attribute("Flags");
        if (ui.name.IsEmpty() || (ui.hash.IsEmpty() && ui.action != _T("delete")) || ui.saveTo.IsEmpty())
            continue;
        m_entries.push_back(ui);
    }
    return true;
}

int CUpdatePackage::updatedFileCount() const
{
    return m_nUpdatedFiles;
}

int CUpdatePackage::totalFileCount() const
{
    return m_nTotalFiles;
}

bool CUpdatePackage::doUpdate()
{
    for(size_t i=0; i< m_entries.size(); i++)
    {
        CUpdateItem &ue = m_entries[i];
        CString copyFrom, copyTo;
        copyFrom = m_PackageFolder + ue.name;
        copyTo = ue.saveTo;
        if (ue.action != _T("delete") && (ue.hash != IuCoreUtils::Utf8ToWstring(IuCoreUtils::CryptoUtils::CalcMD5HashFromFile(IuCoreUtils::WstringToUtf8((LPCTSTR)copyFrom))).c_str() || ue.hash.IsEmpty()))
        {
            setStatusText( CString(TR("MD5 check failed for file "))+IuCoreUtils::Utf8ToWstring(IuCoreUtils::ExtractFileName(IuCoreUtils::WstringToUtf8((LPCTSTR)copyTo))).c_str());
            return false;
        }
    }

    for(size_t i=0; i< m_entries.size(); i++)
    {
        CUpdateItem &ue = m_entries[i];

        CString copyFrom, copyTo;
        copyFrom = m_PackageFolder + ue.name;
        copyTo = ue.saveTo;
        CString appFolder = WinUtils::GetAppFolder();
        if(appFolder.Right(1) == _T("\\"))
            appFolder.Delete(appFolder.GetLength()-1);

        copyTo.Replace(_T("%datapath%"), IuCommonFunctions::GetDataFolder());
        copyTo.Replace(_T("%apppath%"), appFolder);
        std::string dir = IuCoreUtils::ExtractFilePath(IuCoreUtils::WstringToUtf8((LPCTSTR)copyTo));
        if ( !IuCoreUtils::DirectoryExists(dir)) {
            if ( !IuCoreUtils::CreateDir(dir) ) {
                CString logMessage;
                logMessage.Format(_T("Could not create folder '%s'."), (LPCTSTR)IuCoreUtils::Utf8ToWstring(dir).c_str());
                ServiceLocator::instance()->logger()->write(ILogger::logError, _T("Update Engine"), logMessage);

            }
        }
        CString renameTo = copyTo + _T(".") + WinUtils::IntToStr(rand() % 10000) + _T(".old");

        CString buffer = U2W(IuCoreUtils::ExtractFilePath(W2U(copyTo)));
        std::vector<std::string> tokens;
        IuStringUtils::Split(ue.flags, ",", tokens, -1);
        bool skipFile = false;
        bool isWin64Os = WinUtils::IsWindows64Bit();
        for(const auto& token: tokens) {
            if (token == "os_win64bit") {
                skipFile = !isWin64Os;
            } else if (token == "os_win32bit") {
                skipFile = isWin64Os;
            }
        }
        if(skipFile) {
            continue;
        }
        m_nTotalFiles ++;

        if(ue.action == _T("delete"))
        {
            if(MoveFile(copyTo,renameTo))
                m_nUpdatedFiles++;
                DeleteFile(renameTo);
        }
        else
        {
            WinUtils::CreateFolder(buffer);
            if(m_CoreUpdate)
            {

                MoveFile(copyTo,renameTo);
            }
            setStatusText( _T("Copying file '") + copyFrom + _T("' to location '") + copyTo);

            if(!CopyFile(copyFrom,copyTo,FALSE))
            {
                ServiceLocator::instance()->logger()->write(ILogger::logWarning, _T("Update Engine"), CString(_T("Could not write file ")) + IuCoreUtils::Utf8ToWstring(IuCoreUtils::ExtractFileName(IuCoreUtils::WstringToUtf8((LPCTSTR)copyTo))).c_str());

            }
            else
            {
                m_nUpdatedFiles++;
                if(m_CoreUpdate) DeleteFile(renameTo);
            }
        }
    }
    WinUtils::DeleteDir2(m_PackageFolder);
    return true;
}

void CUpdatePackage::setStatusText(const CString& text)
{
    if(m_statusCallback)
        m_statusCallback->updateStatus(0, text);
}

CUpdateManager::CUpdateManager(std::shared_ptr<INetworkClientFactory> networkClientFactory, const CString& tempDirectory) :
    networkClientFactory_(std::move(networkClientFactory))
{
    m_statusCallback = nullptr;
    m_nCoreUpdates = 0;
    nCurrentIndex = 0;

    m_nSuccessPackageUpdates = 0;
    m_stop = false;
    nm_ = networkClientFactory_->create();
    using namespace std::placeholders;
    nm_->setProgressCallback(std::bind(&CUpdateManager::progressCallback, this, _1, _2, _3, _4, _5));
    m_TempDirectory = tempDirectory;
}

CString CUpdateManager::generateReport(bool manualUpdates)
{
    CString text;
    const std::vector<CUpdateInfo>& list = manualUpdates ? m_manualUpdatesList : m_updateList;

    for(const auto& item: list) {
        time_t t = item.timeStamp();
        tm * timeinfo = localtime ( &t );
        CString date;
        date.Format(_T("[%02d.%02d.%04d]"), timeinfo->tm_mday, timeinfo->tm_mon+1, 1900+timeinfo->tm_year);
        text += _T(" * ") + item.displayName() + _T("  ") + date + _T("\r\n\r\n");
        text += item.readableText();
        text += _T("\r\n");
    }
    return text;
}

CString CUpdateManager::generateUpdateMessage() {
    CString text;

    for (size_t i = 0; i<m_updateList.size(); i++) {
        time_t t = m_updateList[i].timeStamp();
        tm * timeinfo = localtime(&t);
        CString date;
        date.Format(_T("[%02d.%02d.%04d]"), timeinfo->tm_mday, timeinfo->tm_mon + 1, 1900 + timeinfo->tm_year);
        text += _T(" * ") + m_updateList[i].displayName() + _T("  ") + date + _T("\r\n\r\n");

    }
    return text;
}

void CUpdatePackage::setUpdateStatusCallback(CUpdateStatusCallback * callback)
{
    m_statusCallback = callback;
}

void CUpdateManager::setUpdateStatusCallback(CUpdateStatusCallback * callback)
{
    m_statusCallback = callback;
}
void CUpdateManager::updateStatus(int packageIndex, const CString& status)
{
    if(m_statusCallback)
        m_statusCallback->updateStatus(packageIndex, status);
}

bool CUpdateManager::AreUpdatesAvailable() const
{
    return !m_updateList.empty();
}

int CUpdateManager::progressCallback(INetworkClient* clientp, int64_t dltotal, int64_t dlnow, int64_t ultotal, int64_t ulnow) {
    CUpdateManager * um = this;
    CString text;
    CString buf1, buf2;
    buf1 = U2W(IuCoreUtils::FileSizeToString(dlnow));
    buf2 = U2W(IuCoreUtils::FileSizeToString(dltotal));
    int percent = 0;
    if(dltotal != 0 )
    percent = int(((double)dlnow/ dltotal) * 100);
    if(percent > 100) percent = 0;
    text.Format(TR("Downloaded %s of %s (%d %%)"), static_cast<LPCTSTR>(buf1), static_cast<LPCTSTR>(buf2), percent);
    um->updateStatus(nCurrentIndex, text);
    if(um->m_stop) return -1;
    return 0;
}

void CUpdateManager::Clear()
{
    m_updateList.clear();
    m_manualUpdatesList.clear();
    m_nCoreUpdates = 0;
    m_nSuccessPackageUpdates = 0;
    m_stop = false;
}

int CUpdateManager::successPackageUpdatesCount() const
{
    return m_nSuccessPackageUpdates;
}

void CUpdateManager::stop()
{
    m_stop = true;
}
