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

#include "AboutDlg.h"

#include <curl/curl.h>
#include <sqlite3.h>
#include "Gui/GuiTools.h"
#include "Func/WinUtils.h"
//#include <libavutil/ffversion.h>
#include <boost/config.hpp>
#include <boost/version.hpp>
#include <webp/decode.h>
#include <libheif/heif_version.h>
#ifdef IU_ENABLE_MEGANZ
#include <mega/version.h>
#endif
#include "Core/AppRuntimeInfo.h"
#ifdef IU_ENABLE_MEDIAINFO
#include "Func/MediaInfoHelper.h"
#endif
#include "Core/Settings/WtlGuiSettings.h"
#include "Func/IuCommonFunctions.h"
#include "Func/LangClass.h"

LRESULT CAboutDlg::OnInitDialog(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
{
    int iconWidth = ::GetSystemMetrics(SM_CXICON);
    int iconHeight = ::GetSystemMetrics(SM_CYICON);
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();

    SetDlgItemText(IDC_APPNAMELABEL, APP_NAME);

    thanksToLabelFont_ = GuiTools::MakeLabelBold(GetDlgItem(IDC_THANKSTOLABEL));
    LogoImage.SubclassWindow(GetDlgItem(IDC_STATICLOGO));
    LogoImage.SetWindowPos(0, 0, 0, iconWidth, iconHeight, SWP_NOMOVE|SWP_NOZORDER);
    LogoImage.loadImage(0, 0, IDR_ICONMAINNEW, false, GetSysColor(COLOR_BTNFACE));

    auto* ver = AppRuntimeInfo::instance()->GetAppVersion();
    auto* translator = ServiceLocator::instance()->translator();

    m_WebSiteLink.SubclassWindow(GetDlgItem(IDC_SITELINK));
    m_WebSiteLink.m_dwExtendedStyle |= HLINK_UNDERLINEHOVER;

    m_GoogleCodeLink.SubclassWindow(GetDlgItem(IDC_GOOGLECODELINK));
    m_GoogleCodeLink.m_dwExtendedStyle |= HLINK_UNDERLINEHOVER;

    m_ReportBugLink.SubclassWindow(GetDlgItem(IDC_FOUNDABUG));
    m_ReportBugLink.m_dwExtendedStyle |= HLINK_UNDERLINEHOVER;
    m_ReportBugLink.SetLabel(TR("Found a bug? Send a bug report to the author."));
    m_ReportBugLink.SetHyperLink(_T("https://github.com/zenden2k/uptooda/issues"));

    iconsByIcons8Link.SubclassWindow(GetDlgItem(IDC_ICONSBYLABEL));
    iconsByIcons8Link.m_dwExtendedStyle |= HLINK_UNDERLINEHOVER;
    iconsByIcons8Link.SetHyperLink(_T("https://icons8.com"));

    m_CommitHashLink.SubclassWindow(GetDlgItem(IDC_COMMITHASH));
    m_CommitHashLink.m_dwExtendedStyle |= HLINK_UNDERLINEHOVER;
    m_CommitHashLink.SetLabel(CString(ver->CommitHashShort.c_str()));
    m_CommitHashLink.SetHyperLink(CString(("https://github.com/zenden2k/uptooda/commit/" + ver->CommitHash).c_str()));

    m_EmailLink.SubclassWindow(GetDlgItem(IDC_AUTHORNAMELABEL));
    m_EmailLink.m_dwExtendedStyle |= HLINK_UNDERLINEHOVER;
    m_EmailLink.SetLabel(CString(_T("\u200E"))+(translator->getCurrentLanguage() == "Russian" ?
        U2W("\xD0\xA1\xD0\xB5\xD1\x80\xD0\xB3\xD0\xB5\xD0\xB9\x20\xD0\xA1\xD0\xB2\xD0\xB8\xD1\x81\xD1\x82\xD1\x83\xD0\xBD\xD0\xBE\xD0\xB2")
        : _T("Sergey Svistunov")) + CString(_T(" \u200E(")) + authorEmail + CString(_T(")\u200E")));
    m_EmailLink.SetHyperLink(CString(_T("mailto:")) + authorEmail);

    CString memoText;

    memoText += TR("Thanks to:") + CString("\r\n\r\n");
    memoText += TR("Contributors:") + CString("\r\n");
    memoText += L"arhangelsoft\thttps://github.com/arhangelsoft\r\nTahir Yilmaz\thttps://github.com/thrylmz\r\nAlex_Qwerty\r\n"
                "johnyxpro\thttps://github.com/johnyxpro\r\n\r\n"
        ;

    CString translatorName = TR("translator_name");
    if ( translatorName == "translator_name") {
        translatorName.Empty();
    }
    if ( !translatorName.IsEmpty() ) {
        memoText += TR("Translators:") + CString("\r\n");

        CString trans;
        CLang lang;
        trans.Format(_T("%s:"), (LPCTSTR)U2W(translator->getLanguageDisplayName()));
        translatorName.Replace(_T("\n"), _T("\r\n"));
        memoText += L"\r\n"+ trans + L"\r\n"+ translatorName +L"\r\n\r\n";
    }

    memoText += TR("Beta-testers:")+ CString("\r\n");
    memoText += L"Graf, CKA3O4H1K, Geslot, Alex_Qwerty\r\n\r\n";

    //memoText += CString(T_R("Thanks to the authors of the following open-source libraries:"))+L"\r\n\r\n";
    memoText += CString(TR("Thanks to the authors of the following open-source libraries:"))+L"\r\n\r\n";
    // \u200E is Unicode Left-To-Right order mark
    // We need to use it to display URLs properly when RTL language is chosen
    memoText += CString(L"WTL")+ L"\t\thttps://sourceforge.net/projects/wtl/\u200E\r\n";
    memoText += CString(L"libcurl") + L"\t\thttps://curl.se/\u200E\r\n";
//#ifdef USE_OPENSSL
    memoText += CString(L"openssl") + L"\t\thttps://www.openssl.org\r\n";
//#endif
    memoText += CString(L"zlib") + L"\t\thttps://www.zlib.net/\r\n";
    memoText += CString(L"squirrel") + L"\t\thttp://squirrel-lang.org\r\n";
    memoText += CString(L"sqrat") + L"\t\thttp://scrat.sourceforge.net\r\n";
    memoText += CString(L"ffmpeg") + L"\t\thttps://www.ffmpeg.org\r\n";
    memoText += CString(L"MediaInfo") + L"\thttps://mediaarea.net/\u200E\r\n";
    memoText += CString(L"pcre") + L"\t\thttps://www.pcre.org/\r\n";
    memoText += CString(L"pcre++") + L"\t\thttps://www.daemon.de/projects/pcrepp/\r\n";

    memoText += CString(L"tinyxml") + L"\t\thttps://sourceforge.net/projects/tinyxml/\u200E\r\n";
    memoText += CString(L"tinyxml2") + L"\t\thttps://github.com/leethomason/tinyxml2\u200E\r\n";
    memoText += CString(L"gumbo parser") + L"\thttps://github.com/google/gumbo-parser\r\n";
    memoText += CString(L"gumbo-query") + L"\thttps://github.com/lazytiger/gumbo-query\r\n";
    memoText += CString(L"glog") + L"\t\thttps://github.com/google/glog\r\n";
    memoText += CString(L"libwebp") + L"\t\thttps://github.com/webmproject/libwebp\r\n";


    memoText += CString(L"minizip") + L"\t\thttps://www.winimage.com/zLibDll/minizip.html\r\n";
    memoText += CString(L"jsoncpp") + L"\t\thttps://github.com/open-source-parsers/jsoncpp\r\n";
    memoText += CString(L"Boost") + L"\t\thttps://www.boost.org/\r\n";
    memoText += CString(L"FastDelegate") + L"\thttps://www.codeproject.com/Articles/7150/Member-Function-Pointers-and-the-Fastest-Possible\r\n";
    memoText += CString(L"WTL Browser") + L"\thttps://www.codeproject.com/Articles/7147/WTL-Browser\r\n";
    memoText += CString(L"CRegistry") + L"\t\thttps://www.codeproject.com/Articles/19/Registry-Class\r\n";
    memoText += CString(L"TParser") + L"\t\thttps://rsdn.ru/article/files/Classes/tparser.xml\r\n";
    memoText += CString(L"CWinHotkeyCtrl") + L"\thttps://rsdn.ru/article/controls/WinHotkeyCtrl.xml\r\n";
    memoText += CString(L"UTF-8 CPP") + L"\thttps://github.com/nemtrif/utfcpp\u200E\r\n";
    memoText += CString(L"CUnzipper") + L"\thttps://www.codeproject.com/Articles/4288/Win-Wrapper-classes-for-Gilles-Volant-s-Zip-Unzi\r\n";
    memoText += CString(L"CThread") + L"\t\thttps://viksoe.dk/code/thread.htm\r\n";
    memoText += CString(L"CPropertyList") + L"\thttps://viksoe.dk/code/propertylist.htm\r\n";
    memoText += CString(L"uriparser") + L"\t\thttps://uriparser.github.io/\u200E\r\n";
    memoText += CString(L"GDI+ helper") + L"\thttps://www.codeproject.com/Articles/4969/GDI-and-MFC-memory-leak-detection\r\n";
    memoText += CString(L"xbbcode") + L"\t\thttps://github.com/patorjk/Extendible-BBCode-Parser\r\n";
    memoText += CString(L"entities.c") + L"\t\thttps://bitbucket.org/cggaertner/cstuff/\u200E\r\n";
    memoText += CString(L"base64") + L"\t\thttps://github.com/aklomp/base64/\u200E\r\n";
    memoText += CString(L"Mega SDK") + L"\thttps://github.com/meganz/sdk\r\n";
    memoText += CString(L"Crypto++") + L" \thttps://www.cryptopp.com/\u200E\r\n";
    memoText += CString(L"c-ares") + L"\t\thttps://c-ares.haxx.se/\u200E\r\n";
    memoText += CString(L"libuv") + L"\t\thttps://github.com/libuv/libuv\r\n";
    memoText += CString(L"WinToast") + L"\t\thttps://github.com/mohabouje/WinToast\r\n";
    memoText += CString(L"xdgmime") + L"\t\thttps://gitlab.freedesktop.org/xdg/xdgmime\r\n\r\n";

    memoText += TR("Settings file path:") + CString(_T("\r\n")) + settings->getSettingsFileName() + _T("\r\n\r\n");
    memoText += TR("Data file path:") + CString(_T("\r\n")) + IuCommonFunctions::GetDataFolder() + _T("\r\n\r\n");
    memoText += CString(L"Build date: ") + CString(ver->BuildDate.c_str()) + _T("\r\n");
    memoText +=  CString(L"Built with: \r\n") + CString(BOOST_COMPILER) +  _T("\r\n");
    CString targetPlatform = BOOST_PLATFORM;
    targetPlatform += _T(" ");
#if defined(_M_ARM64)
    targetPlatform += "ARM64";
#endif
    targetPlatform += _T(" \u200E(");
    targetPlatform += WinUtils::IntToStr(sizeof(void*) * CHAR_BIT);
    targetPlatform += _T(" bit)\u200E");
    memoText +=  CString(L"Target platform: ") + targetPlatform + _T("\r\n\r\n");
    memoText += TR("Libraries:")+ CString("\r\n");
    memoText +=  IuCoreUtils::Utf8ToWstring( curl_version()).c_str() + CString("\r\n");
    CString boostVersion { BOOST_LIB_VERSION };
    boostVersion.Replace(L'_', L'.');
    CString versionLabel;
    versionLabel.Format(_T("Boost: v%s\r\n"), boostVersion.GetString());
    memoText += versionLabel;

    int webpVersion = WebPGetDecoderVersion();
    CString webpVersionStr;
    webpVersionStr.Format(_T("%u.%u.%u"), (webpVersion >> 16) & 0xff, (webpVersion >> 8) & 0xff, webpVersion & 0xff);

    memoText += CString(L"libwebp: v") + webpVersionStr + L"\r\n";
    memoText += CString(L"sqlite: v") + sqlite3_libversion() + L"\r\n";
    memoText += CString(L"libheif: v") + LIBHEIF_VERSION + L"\r\n";

    /*if ( Settings.IsFFmpegAvailable() ) { // Can't determine actual ffmpeg version
        memoText += TR("FFmpeg version:")+ CString("\r\n");
        memoText += FFMPEG_VERSION + CString("\r\n");
    }*/
#ifdef IU_ENABLE_MEDIAINFO
    memoText += MediaInfoHelper::GetLibraryVersion() + _T("\r\n");
#endif
#ifdef IU_ENABLE_MEGANZ
    memoText += str(IuStringUtils::FormatWideNoExcept(L"MEGA SDK: v%d.%d.%d\r\n") % MEGA_MAJOR_VERSION % MEGA_MINOR_VERSION % MEGA_MICRO_VERSION).c_str();
#endif
    memoText += _T("\r\n");

    SYSTEMTIME systime;
    memset(&systime, 0, sizeof(systime));
    int fieldNum = sscanf_s(ver->BuildDate.c_str(), "%hu.%hu.%hu", &systime.wDay, &systime.wMonth, &systime.wYear);
    CString dateStr;
    if (fieldNum == 3) {
        WCHAR pFmt[MAX_PATH] = { 0 };
        GetDateFormat(LOCALE_USER_DEFAULT, 0, &systime, nullptr, pFmt, MAX_PATH);
        dateStr = pFmt;
    } else {
        dateStr = ver->BuildDate.c_str();
    }
    memoText += CString(L"Build options: \r\n");
#ifdef USE_OPENSSL
    memoText += CString(L"USE_OPENSSL\r\n");
#endif
#ifdef IU_ENABLE_MEGANZ
    memoText += CString(L"IU_ENABLE_MEGANZ\r\n");
#endif
#ifdef IU_ENABLE_SFTP
    memoText += CString(L"IU_ENABLE_SFTP\r\n");
#endif
#ifdef IU_ENABLE_MEDIAINFO
    memoText += CString(L"IU_ENABLE_MEDIAINFO\r\n");
#endif
#ifdef IU_ENABLE_WEBVIEW2
    memoText += CString(L"IU_ENABLE_WEBVIEW2\r\n");
#endif
#ifdef IU_ENABLE_FFMPEG
    memoText += CString(L"IU_ENABLE_FFMPEG\r\n");
#endif
#ifdef IU_FFMPEG_STANDALONE
    memoText += CString(L"IU_FFMPEG_STANDALONE\r\n");
#endif
#ifdef IU_LIBHEIF_WITH_DAV1D
    memoText += CString(L"IU_LIBHEIF_WITH_DAV1D\r\n");
#endif
#ifdef IU_ENABLE_SERVERS_CHECKER
    memoText += CString(L"IU_ENABLE_SERVERS_CHECKER\r\n");
#endif
#ifdef IU_ENABLE_NETWORK_DEBUGGER
    memoText += CString(L"IU_ENABLE_NETWORK_DEBUGGER\r\n");
#endif
#ifdef IU_STATIC_RUNTIME
    memoText += CString(L"IU_STATIC_RUNTIME\r\n");
#endif
    SetDlgItemText(IDC_MEMO, memoText);

    CString buildInfo;
    buildInfo.Format(_T("\u200Ebuild %lu (%zu bit)"), ver->Build, sizeof(void*) * 8);

/*#ifdef USE_OPENSSL
    buildInfo += _T(" (with OpenSSL)");
#endif*/

    CString text;
    if (ver->FullVersion.find("nightly") == std::string::npos) {
        text = _T("v");
    }
    text += ver->FullVersion.c_str();
    text += _T(" ");
    text += buildInfo;

    SetDlgItemText(IDC_VERSIONEDIT, text);
    SetDlgItemText(IDC_IMAGEUPLOADERLABEL, dateStr);
    CenterWindow(GetParent());

    TRC(IDOK, "OK");
    TRC(IDC_AUTHORLABEL, "Author:");
    TRC(IDC_WEBSITELABEL, "Website:");

    std::wstring title = str(IuStringUtils::FormatWideNoExcept(TR("About %s")) % APP_NAME);
    SetWindowText(title.c_str());
    return TRUE;
}

LRESULT CAboutDlg::OnCloseCmd(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    EndDialog(wID);
    return 0;
}

LRESULT CAboutDlg::OnCtlColorStatic(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    HDC hdcStatic = reinterpret_cast<HDC>(wParam);
    HWND hwndStatic = reinterpret_cast<HWND>(lParam);

    if (hwndStatic != GetDlgItem(IDC_IMAGEUPLOADERLABEL)) {
        bHandled = false;
        return 0;
    }
    COLORREF textColor = GetSysColor(COLOR_WINDOWTEXT);
    if (textColor == 0) {
        SetTextColor(hdcStatic, RGB(100,100,100));
        SetBkColor(hdcStatic, GetSysColor(COLOR_BTNFACE));
    } else {
        SetTextColor(hdcStatic, GetSysColor(COLOR_GRAYTEXT));
        SetBkColor(hdcStatic, GetSysColor(COLOR_BTNFACE));
        //SetTextColor(hdcStatic, RGB(100, 100, 100));
    }

    return reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_BTNFACE));
}

LRESULT CAboutDlg::OnContextMenu(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    HWND wnd = reinterpret_cast<HWND>(wParam);
    HWND authorNameLabelWnd = GetDlgItem(IDC_AUTHORNAMELABEL);

    if (wnd == authorNameLabelWnd) {
        RECT rc;
        ::GetWindowRect(authorNameLabelWnd, &rc);
        POINT menuOrigin { rc.left, rc.bottom };

        CMenu contextMenu;
        contextMenu.CreatePopupMenu();
        contextMenu.AppendMenu(MF_STRING, ID_COPYAUTHOREMAIL, TR("Copy e-mail"));
        //contextMenu.SetMenuDefaultItem(ID_COPYAUTHOREMAIL, FALSE);
        contextMenu.TrackPopupMenu(TPM_LEFTALIGN | TPM_LEFTBUTTON, menuOrigin.x, menuOrigin.y, m_hWnd);
    }

    return 0;
}

LRESULT CAboutDlg::OnCopyAuthorEmail(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    WinUtils::CopyTextToClipboard(authorEmail);
    return 0;
}
