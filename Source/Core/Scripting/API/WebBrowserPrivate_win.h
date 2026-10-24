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

#ifndef IU_CORE_SCRIPTAPI_WEBBROWSERPRIVATE_WIN_H
#define IU_CORE_SCRIPTAPI_WEBBROWSERPRIVATE_WIN_H

#include "ScriptAPI.h"

#include "WebBrowserPrivateBase.h"
#include "../../../WebBrowser/WebViewWindow.h"
#include "Core/Utils/CoreUtils.h"
#include "Core/Logging.h"
#include "Core/Scripting/Squirrelnc.h"
#include "HtmlDocument.h"
#include "3rdpart/Registry.h"
#include "Func/WinUtils.h"
#include "Core/3rdpart/pcreplusplus.h"
#include "Core/ServiceLocator.h"

namespace ScriptAPI {
class CWebBrowser;

using namespace Sqrat;

class WebBrowserPrivate : public WebBrowserPrivateBase {
public:
    WebBrowserPrivate(CWebBrowser * browser) {
        owningThread_ = std::this_thread::get_id();
        browser_ = browser;
        using namespace std::placeholders;
        webViewWindow_.setOnUrlChanged(std::bind(&WebBrowserPrivate::OnUrlChanged, this, _1));
        webViewWindow_.setOnNavigateError(std::bind(&WebBrowserPrivate::OnNavigateError, this, _1, _2));
        webViewWindow_.setOnDocumentComplete(std::bind(&WebBrowserPrivate::OnDocumentComplete, this, _1));
        initialWidth_ = 800;
        initialHeight_ = 600;
        initialTitle_ = _T("Web Browser");
        timerInterval_ = 0;
    }

    ~WebBrowserPrivate() override {
        assert(owningThread_ == std::this_thread::get_id());
        // WTF? WebViewWindow should be destroyed in the same thread it was created
        if ( IsWindow(webViewWindow_.m_hWnd) ) {
            webViewWindow_.destroyFromAnotherThread();
        }
        webViewWindow_.m_hWnd = 0;
    }

    bool navigateToUrl(const std::string& url) {
        return webViewWindow_.NavigateTo(IuCoreUtils::Utf8ToWstring(url).c_str());
    }

    bool showModal() {
        auto programWindow = ServiceLocator::instance()->programWindow();
        HWND parent = programWindow ? programWindow->getNativeHandle() : nullptr;

        create(parent);

        if ( !initialUrl_.IsEmpty() ) {
            webViewWindow_.NavigateTo(initialUrl_);
        } else if ( !initialHtml_.IsEmpty()) {
            webViewWindow_.displayHTML(initialHtml_);
        }

        bool res = webViewWindow_.DoModal(parent) != 0;
        clearCallbacks();
        return res;
    }

    bool exec() {
        create();
        if (!initialUrl_.IsEmpty()) {
            webViewWindow_.NavigateTo(initialUrl_);
        } else if ( !initialHtml_.IsEmpty()) {
            webViewWindow_.displayHTML(initialHtml_);
        }

        bool res = webViewWindow_.exec()!=0;
        clearCallbacks();
        return res;
    }

    void show() {
        create();
        webViewWindow_.ShowWindow(SW_SHOW);
    }

    void hide() {
        webViewWindow_.ShowWindow(SW_HIDE);
    }

    void close() override {
        if (webViewWindow_.m_hWnd) {
            webViewWindow_.close();
        }
    }

    void abort() override
    {
        WebBrowserPrivateBase::abort();
        if (webViewWindow_.m_hWnd) {
            webViewWindow_.abortFromAnotherThread(); // close as user
        }
    }

    bool setHtml(const std::string& html) {
        return webViewWindow_.displayHTML(U2W(html));
    }

    void resize(int w, int h) {
        if ( webViewWindow_.m_hWnd) {
            CClientDC dc(webViewWindow_.m_hWnd);
            float dpiScaleX_ = GetDeviceCaps(dc, LOGPIXELSX) / 96.0f;
            float dpiScaleY_ = GetDeviceCaps(dc, LOGPIXELSY) / 96.0f;
            webViewWindow_.ResizeClient(static_cast<int>(w * dpiScaleX_), static_cast<int>(h * dpiScaleY_));

        } else {
            initialWidth_ = w;
            initialHeight_ = h;
        }
    }

    void setTitle(const std::string& title) {
        if ( webViewWindow_.m_hWnd) {
            webViewWindow_.SetWindowText(IuCoreUtils::Utf8ToWstring(title).c_str());
        } else {
            initialTitle_ = IuCoreUtils::Utf8ToWstring(title).c_str();
        }
    }

    std::string url() {
        if ( webViewWindow_.m_hWnd) {
            return IuCoreUtils::WstringToUtf8((LPCTSTR)webViewWindow_.getUrl());
        }
        return std::string();
    }

    std::string title() {
        return IuCoreUtils::WstringToUtf8((LPCTSTR)webViewWindow_.getTitle());
    }

    std::string getDocumentContents() {
        // TODO: not implemented
        return {};
        //return W2U(webViewWindow_.getDocumentContents());
    }

    void addTrustedSite(const std::string& site) {

        pcrepp::Pcre reg("(.+?)://(.+)/?", "");
        std::string domain = site;
        std::string protocol = "http";
        if ( reg.search(site)) {
            protocol = reg.get_match(1);
            domain = reg.get_match(2);
        }
        CRegistry Reg;
        Reg.SetRootKey( HKEY_CURRENT_USER );
        if ( Reg.SetKey( CString(_T("Software\\Microsoft\\Windows\\CurrentVersion\\Internet Settings\\ZoneMap\\Domains\\"))+domain.c_str(), true ) ) {
            Reg.WriteDword(CString(protocol.c_str()), 2);
        }
    }

    int getMajorVersion() {
        return WinUtils::GetInternetExplorerMajorVersion();
    }

    std::string runJavaScript(const std::string& code) {
        return webViewWindow_.runJavaScript(IuCoreUtils::Utf8ToWstring(code).c_str());
    }

    std::string callJavaScriptFunction(const std::string& funcName, Sqrat::Array args) {
        /*if (webViewWindow_.m_hWnd) {
            CComVariant res;
            webViewWindow_.view_.CallJScript(IuCoreUtils::Utf8ToWstring(funcName).c_str(), &res);
            return ComVariantToString(res);
        } else {
            LOG(ERROR) << "injectJavaScript: WebBrowser control is not created yet.";
        }*/
        return std::string();
    }

    void setFocus();
protected:
    CWebViewWindow webViewWindow_;
    CString initialUrl_;
    CString initialTitle_;
    CString initialHtml_;
    CWebBrowser* browser_;
    int initialWidth_;
    int initialHeight_;
    std::thread::id owningThread_;

    void create(HWND parent = 0 ) {
        if ( webViewWindow_.m_hWnd) {
            return;
        }

        //CRect r(0,0, initialWidth_, initialWidth_);
        webViewWindow_.Create(parent,CWindow::rcDefault,initialTitle_, WS_POPUP | WS_OVERLAPPEDWINDOW    );
        CClientDC dc(webViewWindow_.m_hWnd);
        float dpiScaleX_ = dc.GetDeviceCaps(LOGPIXELSX) / 96.0f;
        float dpiScaleY_ = dc.GetDeviceCaps(LOGPIXELSY) / 96.0f;

        if ( initialWidth_ && initialHeight_ ) {
            webViewWindow_.ResizeClient(static_cast<int>(initialWidth_ * dpiScaleX_), static_cast<int>(initialHeight_ * dpiScaleY_));
        }
    }
    void OnUrlChanged(const CString& url);
    void OnDocumentComplete(const CString& url);
    bool OnNavigateError(const CString& url, LONG statusCode);
};

}
#endif
