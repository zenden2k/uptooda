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
#include "ImageReuploaderDlg.h"

#include <algorithm>
#include <set>

#include <WinInet.h>
#include "atlheaders.h"
#include "Core/3rdpart/pcreplusplus.h"
#include "LogWindow.h"
#include "Gui/GuiTools.h"
#include "Core/Utils/StringUtils.h"
#include "Core/Upload/FileQueueUploader.h"
#include "Func/WinUtils.h"
#include "Gui/Controls/CustomEditControl.h"
#include "Core/LocalFileCache.h"
#include "Core/ServiceLocator.h"
#include "Func/IuCommonFunctions.h"
#include "Gui/Controls/ServerSelectorControl.h"
#include "Core/Upload/FileUploadTask.h"
#include "Core/3rdpart/UriParser.h"
#include "Core/AppRuntimeInfo.h"
#include "Core/Network/NetworkClientFactory.h"
#include "Core/Settings/WtlGuiSettings.h"

const TCHAR CImageReuploaderDlg::LogTitle[] = _T("Image Reuploader");

// CImageReuploaderDlg
CImageReuploaderDlg::CImageReuploaderDlg(CWizardDlg *wizardDlg, CMyEngineList * engineList, UploadManager *  uploadManager,
    UploadEngineManager *uploadEngineManager, const CString &initialBuffer) :
        m_FileDownloader(std::make_shared<NetworkClientFactory>(), AppRuntimeInfo::instance()->tempDirectory()),
        m_InitialBuffer(initialBuffer)
{
    m_WizardDlg = wizardDlg;
    m_EngineList = engineList;
    uploadManager_ = uploadManager;
    htmlClipboardFormatId = RegisterClipboardFormat(_T("HTML Format"));
    uploadEngineManager_ = uploadEngineManager;
    m_nFilesCount = 0;
    m_nFilesUploaded = 0;
    m_nFilesDownloaded = 0;
}

LRESULT CImageReuploaderDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    CenterWindow(GetParent());
    AddClipboardFormatListener(m_hWnd);

    DlgResize_Init(false, true, 0); // resizable dialog without "griper"
    sourceTextEditControl.AttachToDlgItem(m_hWnd, IDC_INPUTTEXT);

    using namespace std::placeholders;
    sourceTextEditControl.setOnPasteCallback(std::bind(&CImageReuploaderDlg::OnEditControlPaste, this, _1));
    outputEditControl.AttachToDlgItem(m_hWnd, IDC_OUTPUTTEXT);
    ::SetFocus(GetDlgItem(IDOK));
    SetWindowText(TR("Re-upload images"));
    TRC(IDOK, "Reupload");
    TRC(IDCANCEL, "Close");
    TRC(IDC_COPYTOCLIPBOARD, "Copy to clipboard");
    TRC(IDC_SOURCECODERADIO, "Source code");
    TRC(IDC_LINKSLISTRADIO, "List of URLs");
    TRC(IDC_IMAGEDOWNLOADERTIP, "Enter the text containing links to images (HTML, BBcode, or just plain text):");
    TRC(IDC_PASTEHTML, "Paste HTML from clipboard");
    TRC(IDC_SOURCEURLLABEL, "Source URL (optional):");
    TRC(IDC_PASTEHTMLONCTRLVCHECKBOX, "Paste HTML on Ctrl+V");
    TRC(IDC_DESCRIPTION, "Reuploading images in the text with preserving it's original markup");
    TRC(IDC_SHOWLOG, "Show Error Log");

    if (ServiceLocator::instance()->translator()->isRTL()) {
        // Removing WS_EX_RTLREADING style from some controls to look properly when RTL interface language is choosen
        GuiTools::RemoveWindowStyleEx(GetDlgItem(IDC_INPUTTEXT), WS_EX_RTLREADING);
        GuiTools::RemoveWindowStyleEx(GetDlgItem(IDC_SOURCEURLEDIT), WS_EX_RTLREADING);
        GuiTools::RemoveWindowStyleEx(GetDlgItem(IDC_OUTPUTTEXT), WS_EX_RTLREADING);
    }

    RECT serverSelectorRect = GuiTools::GetDialogItemRect( m_hWnd, IDC_IMAGESERVERPLACEHOLDER);
    imageServerSelector_  = std::make_unique<CServerSelectorControl>(uploadEngineManager_, true);
    imageServerSelector_->Create(m_hWnd, serverSelectorRect);
    imageServerSelector_->setTitle(TR("Server for uploading images"));
    imageServerSelector_->ShowWindow( SW_SHOW );
    imageServerSelector_->SetWindowPos( GetDlgItem(IDC_IMAGESERVERPLACEHOLDER), serverSelectorRect.left, serverSelectorRect.top, serverSelectorRect.right-serverSelectorRect.left, serverSelectorRect.bottom - serverSelectorRect.top, SWP_NOZORDER);
    imageServerSelector_->setServerProfile(settings->imageServer.getByIndex(0));

    GuiTools::SetCheck(m_hWnd, IDC_SOURCECODERADIO, true );
    GuiTools::SetCheck(m_hWnd, IDC_PASTEHTMLONCTRLVCHECKBOX, settings->ImageReuploaderSettings.PasteHtmlOnCtrlV);
    descriptionLabelFont_ = GuiTools::MakeLabelBold(GetDlgItem(IDC_DESCRIPTION));
    HWND hWnd = GetDlgItem(IDC_ANIMATIONSTATIC);
    if (hWnd) {
        m_wndAnimation.SubclassWindow(hWnd);
        m_wndAnimation.ShowWindow(SW_HIDE);
    }

    if ( IsClipboardFormatAvailable(htmlClipboardFormatId) || IsClipboardFormatAvailable(CF_TEXT ) ) {
        sourceTextEditControl.SendMessage(WM_PASTE);
    }

    SetDlgItemText(IDC_SERVENAMELABEL, CString(TR("Server")) + _T(": ") + Utf8ToWCstring(serverProfile_.serverName()));

    SetDlgItemText(IDC_RESULTSLABEL, _T(""));
    ::SetFocus(GetDlgItem(IDC_FILEINFOEDIT));
    SendDlgItemMessage(IDC_INPUTTEXT, EM_SETLIMITTEXT, 20 * 1024 * 1024, 0);
    SendDlgItemMessage(IDC_OUTPUTTEXT, EM_SETLIMITTEXT, 20 * 1024 * 1024, 0);
    clipboardUpdated();
    return 1;
}

LRESULT CImageReuploaderDlg::OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    serverProfile_ = imageServerSelector_->serverProfile();
    BeginDownloading();
    return 0;
}

LRESULT CImageReuploaderDlg::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    bool closeWindow = true;
    if (m_FileDownloader.isRunning() ) {
        m_FileDownloader.stop();
        closeWindow = false;
    }
    if ( uploadManager_->IsRunning() ) {
        uploadSession_->stop();
        closeWindow = false;
    }

    if ( closeWindow ) {
        WtlGuiSettings& Settings = *ServiceLocator::instance()->settings<WtlGuiSettings>();
        OnClose();
        EndDialog(wID);
    }
    return 0;
}

LRESULT CImageReuploaderDlg::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    RemoveClipboardFormatListener(m_hWnd);
    return 0;
}

void CImageReuploaderDlg::OnFileDownloadFinished(bool ok, int statusCode, const CFileDownloader::DownloadFileListItem& it)
{
    bool success = false;

    //DownloadItemData* dit = reinterpret_cast<DownloadItemData*>(it.id);

    if ( ok ) {
        m_nFilesDownloaded++;
        if ( addUploadTask(it, it.fileName) ) {
            updateStats();
            success = true;
        }
    } else {


    }

    if ( !success ) {
        ILogger::LogMsgType logMessageType = ILogger::logError;
        CString cacheLogMessage;
        if ( tryGetFileFromCache(it, cacheLogMessage ) ) {
            logMessageType = ILogger::logWarning;
        }

        ServiceLocator::instance()->logger()->write(logMessageType, LogTitle, _T("Cannot download the image '") + Utf8ToWCstring(it.url) + _T("'.")
            + _T("\r\nStatus code:") + WinUtils::IntToStr(statusCode) + (cacheLogMessage.IsEmpty() ? _T("") : (_T("\r\n\r\n") + cacheLogMessage)) + _T("\r\n") );

        /*if ( !cacheLogMessage.IsEmpty() ) {
            WriteLog(logWarning, LogTitle, cacheLogMessage);
        }*/
    }
}

bool CImageReuploaderDlg::tryGetFileFromCache(const CFileDownloader::DownloadFileListItem& it, CString& logMessage) {
    auto* dit = static_cast<DownloadItemData*>(it.id);
    LocalFileCache* localFileCache = LocalFileCache::instance();
    bool success = false;
    localFileCache->ensureHistoryParsed();
    std::string localFile = localFileCache->get(dit->originalUrl);
    CString message;
    if ( !localFile.empty() ) {
        message.Format(_T("File '%s' has been found on local computer ('%s')"), U2W(it.url).GetString(), U2W(localFile).GetString() );
        if ( addUploadTask(it, localFile) ) {
            updateStats();
            success = true;
        }
    } else {
        localFile = localFileCache->getThumb(dit->originalUrl);
        if ( !localFile.empty() ) {
            ImageConverter imageConverter;
            Thumbnail thumb;

            if (!thumb.loadFromFile(WCstringToUtf8(IuCommonFunctions::GetDataFolder() + _T("\\Thumbnails\\"))
                + serverProfile_.getImageUploadParams().getThumb().TemplateName +
                ".xml")) {
                ServiceLocator::instance()->logger()->write(ILogger::logError, LogTitle, TR("Couldn't load thumbnail preset!"));
            } else {
                message.Format(_T("Generating the thumbnail from local file ('%s')"), U2W(localFile).GetString() );
                imageConverter.setEnableProcessing(false);
                auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
                imageConverter.setImageConvertingParams(settings->ConvertProfiles[U2W(serverProfile_.getImageUploadParams().ImageProfileName)]);
                imageConverter.setThumbCreatingParams(serverProfile_.getImageUploadParams().getThumb());
                /*bool GenThumbs = serverProfile_.getImageUploadParams().CreateThumbs &&
                    ((!serverProfile_.getImageUploadParams().UseServerThumbs) || (!ue->SupportThumbnails));*/
                imageConverter.setThumbnail(&thumb);
                imageConverter.setGenerateThumb(true);
                imageConverter.convert(localFile);

                CString thumbFileName = U2W(imageConverter.getThumbFileName());
                if (!thumbFileName.IsEmpty()) {
                    if ( addUploadTask(it, WCstringToUtf8(thumbFileName)) ) {
                        updateStats();
                        success = true;
                    }
                }
            }
        }
        //message.Format(_T("File '%s' not found in local cache"), (LPCTSTR)Utf8ToWCstring(it.url) );
    }
    logMessage = message;
    return success;
}

bool CImageReuploaderDlg::addUploadTask(const CFileDownloader::DownloadFileListItem& it, const std::string& localFileName ) {
    std::string mimeType = IuCoreUtils::GetFileMimeType(localFileName);
    if (mimeType.find("image/") == std::string::npos) {
        ServiceLocator::instance()->logger()->write(ILogger::logError, LogTitle, _T("File '") + U2W(it.url) +
            _T("'\r\n doesn't seems to be an image.\r\nIt has mime type '") + U2W(mimeType) + "'.");
        return false;
    }

    auto* dit = static_cast<DownloadItemData*>(it.id);

    auto uploadItemData = std::make_unique<UploadItemData>();
    uploadItemData->sourceUrl = it.url;
    uploadItemData->sourceIndex = dit->sourceIndex;
    uploadItemData->originalUrl = dit->originalUrl;

    std::string defaultExtension = IuCoreUtils::GetDefaultExtensionForMimeType(mimeType);
    std::string displayName = it.displayName;
    if ( !defaultExtension.empty() ) {
        std::string fileNameWithoutExt = IuCoreUtils::ExtractFileNameNoExt(it.fileName);
        displayName = fileNameWithoutExt + "." + defaultExtension;
    }
    std::shared_ptr<FileUploadTask> fileUploadTask = std::make_shared<FileUploadTask>(localFileName, displayName);
    fileUploadTask->setServerProfile(serverProfile_);
    fileUploadTask->setIsImage(true);
    fileUploadTask->setUserData(uploadItemData.get());
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    fileUploadTask->setUrlShorteningServer(settings->urlShorteningServer);

    {
        std::lock_guard<std::mutex> lk(uploadItemsMutex_);
        uploadItems_.push_back(std::move(uploadItemData));
    }

    using namespace std::placeholders;
    fileUploadTask->addTaskFinishedCallback(std::bind(&CImageReuploaderDlg::OnFileFinished, this, _1, _2));
    {
        std::lock_guard<std::mutex> lk(uploadSessionMutex_);
        uploadSession_->addTask(fileUploadTask);
        uploadManager_->addTaskToQueue(fileUploadTask);
    }
    return true;
}

void CImageReuploaderDlg::OnDownloaderQueueFinished()
{
    if(!m_InitialBuffer.IsEmpty())
    {
        OnClose();
        EndDialog(0);
        return;
    }
    if ( !uploadManager_->IsRunning() ) {
        processFinished();
    }
}

bool CImageReuploaderDlg::ExtractLinks(const std::string& text, std::vector<std::string> &result) {
    pcrepp::Pcre regExpImages("<img [^>]*src=[\"|\']([^\"|\']+)", "imcu");

    std::string str = text;
    std::vector<Match> matches;
    std::set<int> uniqueOffsets;

    size_t pos = 0;
    while ( pos <= str.length() ) {
        if ( regExpImages.search(str, pos) ) {
            Match match;
            match.start = regExpImages.get_match_start(0);
            match.length = regExpImages.get_match_end(0) - match.start + 1;

            if ( uniqueOffsets.find(match.start) == uniqueOffsets.end() ) {
                uniqueOffsets.insert(match.start);
                matches.push_back(match);
            }

            pos = regExpImages.get_match_end() + 1;

        } else {
            break;
        }
    }

    pcrepp::Pcre regExpBbcode("\\[img\\](.+?)\\[/img\\]", "imcu");

    pos = 0;
    while ( pos <= str.length() ) {
        if ( regExpBbcode.search(str, pos) ) {
            Match match;
            match.start = regExpBbcode.get_match_start(0);
            match.length = regExpBbcode.get_match_end(0) - match.start + 1;

            if ( uniqueOffsets.find(match.start) == uniqueOffsets.end() ) {
                uniqueOffsets.insert(match.start);
                matches.push_back(match);
            }

            pos = regExpBbcode.get_match_end() + 1;

        } else {
            break;
        }
    }

    pcrepp::Pcre reg("((http|https|ftp)://[\\w\\d:#@%/;$()~_?\\+-=\\\\\\.&]*)", "imcu");
    pos = 0;

    while (pos <= str.length()) {
        if( reg.search(str, pos)) {
            size_t questionMarkPos =  reg[1].find_last_of('?');
            std::string url;
            if ( questionMarkPos != std::string::npos ) {
                url = reg[1].substr(0, questionMarkPos);
            } else {
                url = reg[1];
            }
            auto& supportedExtensions = IuCommonFunctions::GetSupportedImageExtensions();
            std::string fileExt = IuStringUtils::ToLower( IuCoreUtils::ExtractFileExt(url) );
            if (supportedExtensions.find(fileExt) != supportedExtensions.end()) {
                Match match;
                match.start = reg.get_match_start(0);
                match.length = reg.get_match_end(0) - match.start + 1;
                if ( uniqueOffsets.find(match.start) == uniqueOffsets.end() ) {
                    uniqueOffsets.insert(match.start);
                    matches.push_back(match);
                }
            }

            pos = reg.get_match_end() + 1;
        }
        else {
            break;
        }
    }
    std::sort(matches.begin(), matches.end());
    std::vector<Match>::iterator it;
    it = std::unique(matches.begin(), matches.end());
    matches.resize( std::distance(matches.begin(), it));

    std::set<std::string> uniqueLinks;

    for ( std::vector<Match>::iterator it = matches.begin(); it != matches.end(); ++it ) {
        std::string link = text.substr(it->start, it->length);
        if ( uniqueLinks.find(link) == uniqueLinks.end() ) {
            uniqueLinks.insert(link);
            result.push_back(link);
        }
    }

    return true;
}

bool CImageReuploaderDlg::BeginDownloading()
{
    m_nFilesCount = 0;
    m_nFilesDownloaded = 0;
    m_nFilesUploaded = 0;
    uploadedItems_.clear();
    std::string inputText = WCstringToUtf8( GuiTools::GetWindowText(GetDlgItem(IDC_INPUTTEXT)) );
    std::vector<std::string> links;
    ExtractLinks(inputText, links);

    CString sourceUrl = GuiTools::GetDlgItemText( m_hWnd, IDC_SOURCEURLEDIT );
    std::string sourceUrlUtf8 = W2U(sourceUrl);

    if ( !sourceUrl.IsEmpty() ) {
        uriparser::Uri uri(sourceUrlUtf8);

        if ( uri.scheme().empty() ) {
            CString message;
            message.Format(TR("Invalid source URL: '%s'. Absolute URL is needed, including schema (http,https,etc)"), static_cast<LPCTSTR>(sourceUrl) );
            LocalizedMessageBox(message, APP_NAME, MB_OK | MB_ICONEXCLAMATION);
            return false;
        }
    }

    if ( links.empty() ) {
        LocalizedMessageBox(TR("Could not find links in the given text!"), APP_NAME, MB_OK | MB_ICONEXCLAMATION);
        return false;
    } else {
        uploadSession_ = std::make_shared<UploadSession>();
        uploadSession_->addSessionFinishedCallback(std::bind(&CImageReuploaderDlg::OnQueueFinished, this, std::placeholders::_1));
        uploadManager_->addSession(uploadSession_);
        std::string result;
        for (size_t i = 0; i < links.size(); i++) {
            std::string url = links[i];
            uriparser::Uri uri(url);
            std::string scheme = uri.scheme();
            std::string host = uri.host();

            CString urlWide = Utf8ToWCstring(url);
            if ( scheme.empty() && !host.empty() /*urlWide.Left(2) == _T("//")*/) { // links without protocol
                std::string newScheme = "http";
                if (!sourceUrlUtf8.empty()) {
                    uriparser::Uri pageUri(sourceUrlUtf8);
                    newScheme = pageUri.scheme();
                }
                if (!newScheme.empty()) {
                    urlWide = U2W(newScheme) + _T(":") + urlWide;
                    url = newScheme + ":" + url;
                    scheme = newScheme;
                }
            }

            std::string absoluteUrl = url;

            if ( scheme.empty() )  {
                if ( sourceUrl.IsEmpty() ) {
                    ServiceLocator::instance()->logger()->write(ILogger::logError, LogTitle, _T("Cannot download file by relative url: \"") + urlWide + _T("\".\r\nYou must provide base URL."));
                    continue;
                }
                TCHAR absoluteUrlBuffer[MAX_PATH+1];
                DWORD bufferLength = MAX_PATH;
                InternetCombineUrl(sourceUrl, Utf8ToWCstring(url), absoluteUrlBuffer, &bufferLength, 0);
                absoluteUrl = WCstringToUtf8(absoluteUrlBuffer);
            }

            result += absoluteUrl + "\r\n";
            auto dit = std::make_unique<DownloadItemData>();
            dit->originalUrl = url;
            dit->sourceIndex = i;
            void* id = dit.get();
            downloadItems_.push_back(std::move(dit));

            m_FileDownloader.addFile( absoluteUrl, id, WCstringToUtf8(sourceUrl) );
            m_nFilesCount ++;

        }
        if ( m_nFilesCount ) {
            m_wndAnimation.ShowWindow(SW_SHOW);
            GuiTools::EnableDialogItem(m_hWnd, IDOK, false);

            SetDlgItemText(IDCANCEL, TR("Stop"));
            auto mgr = ServiceLocator::instance()->historyManager();

            historySession_ = mgr->newSession();
            using namespace std::placeholders;

            m_FileDownloader.setOnFileFinishedCallback(std::bind(&CImageReuploaderDlg::OnFileDownloadFinished, this, _1, _2, _3));
            m_FileDownloader.setOnQueueFinishedCallback(std::bind(&CImageReuploaderDlg::OnDownloaderQueueFinished, this));
            m_FileDownloader.start();

            updateStats();
            //SetDlgItemText( IDC_OUTPUTTEXT,  Utf8ToWCstring(result) );
        }
    }
    return false;
}

bool CImageReuploaderDlg::LinksAvailableInText(const CString &text)
{
    return false;
}


void CImageReuploaderDlg::OnFileFinished(UploadTask* task, bool ok) {
    if (ok) {
        FileUploadTask* fileUploadTask = dynamic_cast<FileUploadTask*>(task);
        if (!fileUploadTask) {
            return;
        }
        auto* uploadItemData = static_cast<UploadItemData*>(fileUploadTask->userData());
        UploadedItem item;
        item.sourceUrl = uploadItemData->sourceUrl;
        item.originalUrl = uploadItemData->originalUrl;
        UploadResult* result = task->uploadResult();
        item.newUrl = result->directUrl;

        {
            std::lock_guard<std::mutex> lk(mutex_);
            uploadedItems_[uploadItemData->sourceIndex] = item;
            generateOutputText();
            m_nFilesUploaded++;
        }
        updateStats();
    }
}

void CImageReuploaderDlg::OnQueueFinished(UploadSession* uploadSession) {
    GuiTools::EnableDialogItem(m_hWnd, IDOK, true);
    updateStats();
    if ( !m_FileDownloader.isRunning() ) {
        processFinished();
    }
}

void CImageReuploaderDlg::generateOutputText() {
    bool showSourceCode = GuiTools::GetCheck(m_hWnd, IDC_SOURCECODERADIO);

    if ( showSourceCode ) {
        std::string inputText = WCstringToUtf8( GuiTools::GetWindowText(GetDlgItem(IDC_INPUTTEXT)) );
        for ( const auto& it: uploadedItems_) {
            const UploadedItem& uploadedItem = it.second;
            inputText = IuStringUtils::Replace(inputText, uploadedItem.originalUrl, uploadedItem.newUrl);
        }
        SetDlgItemText( IDC_OUTPUTTEXT,  Utf8ToWCstring(inputText) );
    } else {
        CString outputText;
        for ( const auto& it: uploadedItems_) {
            const UploadedItem& uploadedItem = it.second;
            outputText += Utf8ToWCstring(uploadedItem.newUrl) + _T("\r\n");
        }
        SetDlgItemText( IDC_OUTPUTTEXT,  outputText );
    }
}

LRESULT CImageReuploaderDlg::OnClickedOutputRadioButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    generateOutputText();
    return 0;
}

void CImageReuploaderDlg::updateStats() {
    CString text =  CString(TR("Uploaded images: ")) + WinUtils::IntToStr(uploadedItems_.size())
        + (m_nFilesDownloaded ? CString(_T("/")) + WinUtils::IntToStr(m_nFilesDownloaded) : CString(_T("")))
        + CString(_T(", ")) + TR("Downloaded: ")
        + WinUtils::IntToStr(m_nFilesDownloaded) + CString(_T("/")) + WinUtils::IntToStr(m_nFilesCount);
    SetDlgItemText( IDC_RESULTSLABEL,text);
}

void CImageReuploaderDlg::processFinished() {
    ::EnableWindow(GetDlgItem(IDOK),true);
    ::EnableWindow(GetDlgItem(IDC_FILEINFOEDIT),true);
    TRC(IDCANCEL, "Close");
    m_wndAnimation.ShowWindow(SW_HIDE);
    SetDlgItemText(IDCANCEL, TR("Close"));
}

LRESULT CImageReuploaderDlg::OnClickedCopyToClipboardButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    CString outputText = GuiTools::GetWindowText(GetDlgItem(IDC_OUTPUTTEXT));
    WinUtils::CopyTextToClipboard(outputText);
    return 0;
}

bool GetClipboardHtml(CString& text, CString& outSourceUrl)
{
    UINT clipboardFormat = RegisterClipboardFormat(_T("HTML Format"));
    if (OpenClipboard(NULL))
    {
        HGLOBAL hglb = GetClipboardData(clipboardFormat);
        LPCSTR lpstr = static_cast<LPCSTR>(GlobalLock(hglb));
        //std::string tempStr = lpstr;
        std::string ansiString = lpstr;

        std::istringstream f(ansiString);
        std::string line;
        int startFragment = -1;
        int endFragment = -1;
        std::string sourceUrl;

        while (std::getline(f, line)) {
            std::vector<std::string> tokens;
            IuStringUtils::Split(line, ":", tokens, 2);
            if ( tokens.size() == 2) {
                if ( tokens[0] == "StartFragment") {
                    startFragment = atoi(tokens[1].c_str());
                } else if ( tokens[0] == "EndFragment" ) {
                    endFragment = atoi(tokens[1].c_str());
                } else if ( tokens[0] == "SourceURL" ) {
                    sourceUrl = tokens[1];
                }
            }
            std::cout << line << std::endl;
        }
        if ( startFragment != -1 && endFragment != -1 ) {
            text = Utf8ToWCstring( ansiString.substr(startFragment, endFragment - startFragment) );
        }
        outSourceUrl = Utf8ToWCstring(sourceUrl);

        GlobalUnlock(hglb);
        CloseClipboard();

        return true;
    }

    return false;
}

LRESULT CImageReuploaderDlg::OnClickedPasteHtml(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    pasteHtml();
    return 0;
}

bool CImageReuploaderDlg::OnEditControlPaste(CCustomEditControl*) {
    if ( GuiTools::GetCheck(m_hWnd, IDC_PASTEHTMLONCTRLVCHECKBOX ) ) {
        return pasteHtml();
    }
    return false;
}

bool CImageReuploaderDlg::pasteHtml() {
    if ( IsClipboardFormatAvailable(htmlClipboardFormatId) ) {
        CString clipboardText;
        CString sourceUrl;
        if (  WinUtils::GetClipboardHtml(clipboardText, sourceUrl) ) {
            SendDlgItemMessage(IDC_INPUTTEXT, EM_REPLACESEL, TRUE, (LPARAM)(LPCTSTR)clipboardText);
            //SendDlgItemMessage(IDC_INPUTTEXT, EM_SETSEL, -1, 0);
            SetDlgItemText(IDC_SOURCEURLEDIT, sourceUrl);
            return true;
        }
        return false;
    }
    return false;
}

bool CImageReuploaderDlg::OnClose() {
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    settings->ImageReuploaderSettings.PasteHtmlOnCtrlV = GuiTools::GetCheck(m_hWnd, IDC_PASTEHTMLONCTRLVCHECKBOX);
    return true;
}

LRESULT CImageReuploaderDlg::OnShowLogClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    ServiceLocator::instance()->logWindow()->Show();
    return 0;
}

LRESULT CImageReuploaderDlg::OnClipboardUpdate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    clipboardUpdated();
    return 0;
}

void CImageReuploaderDlg::clipboardUpdated() {
    bool isHtmlAvailable = IsClipboardFormatAvailable(htmlClipboardFormatId) != 0;

    GuiTools::EnableDialogItem(m_hWnd, IDC_PASTEHTML, isHtmlAvailable);
}
