#ifndef IU_GUI_CONTROLS_WEBVIEW2BROWSERVIEW_H
#define IU_GUI_CONTROLS_WEBVIEW2BROWSERVIEW_H

#pragma once

#include "AbstractBrowserView.h"

#include <wrl.h>
#include <wil/com.h>
#include "WebView2.h"

class Webview2BrowserView: public AbstractBrowserView {
public:
    bool createBrowserView(HWND parentWnd, const RECT& bounds) override;
    void resize(const RECT& rc) override;
    void navigateTo(CString url) override;
    bool showHTML(CString html) override;
    std::string runJavaScript(CString code) override;
    CString getUrl() override;
    CString getTitle() override;
    void setFocus() override;
private:
    CComPtr<ICoreWebView2Controller> webviewController_;
    CComPtr<ICoreWebView2> webviewWindow_;
    CString initialUrl_;
};

#endif