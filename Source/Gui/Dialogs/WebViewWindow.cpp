#include "WebViewWindow.h"

#include "Core/AppRuntimeInfo.h"

#include "Gui/GuiTools.h"
#include "Core/Utils/CoreUtils.h"
#include "resource.h"   

#ifdef IU_ENABLE_WEBVIEW2
#include "Gui/Controls/Webview2BrowserView.h"
#endif
#include "Gui/Controls/WTLBrowserView.h"

CWebViewWindow::CWebViewWindow() {
    isModal_ = false;
    messageLoopIsRunning_ = false;
    hWndClient_ = nullptr;
}

CWebViewWindow::~CWebViewWindow() {

}

LRESULT CWebViewWindow::OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    icon_ = GuiTools::LoadBigIcon(IDR_MAINFRAME);
    iconSmall_ = GuiTools::LoadSmallIcon(IDR_MAINFRAME);

    SetIcon(icon_, TRUE);
    SetIcon(iconSmall_, FALSE);

    HWND hWnd = m_hWnd;
    ::CoInitialize(NULL);
    HRESULT hr = E_FAIL;
    RECT rc;
    GetWindowRect(&rc);
    bool created = false;
#ifdef IU_ENABLE_WEBVIEW2
    browserView_ = std::make_unique<Webview2BrowserView>();
    created = browserView_->createBrowserView(m_hWnd, rc);
#endif
    if (!created) {
        browserView_ = std::make_unique<CWTLBrowserView>();
        browserView_->createBrowserView(m_hWnd, rc); 
    }

    browserView_->setOnUrlChanged(onUrlChanged_);
    browserView_->setOnDocumentComplete(onDocumentComplete_);
    browserView_->setOnNavigateError(onNavigateError_);

    if (!initialUrl_.IsEmpty()) {
        browserView_->navigateTo(initialUrl_);
    } else if (!initialHtml_.IsEmpty()) {
        browserView_->showHTML(initialHtml_);
    }

    return 0;
}

LRESULT CWebViewWindow::OnResize(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    if (browserView_) {
        RECT clientRect;
        GetClientRect(&clientRect);
        browserView_->resize(clientRect);
    }
   
    return 0;
}

bool CWebViewWindow::NavigateTo(const CString& url) {
    if (browserView_) {
        browserView_->navigateTo(url);
    } else {
        initialUrl_ = url;
    }

    return true;
}

int CWebViewWindow::DoModal(HWND parent, bool show )
{
    isModal_ = true;

    if ( show ) {
        ShowWindow(SW_SHOW);
        SetActiveWindow();
        SetForegroundWindow(m_hWnd);
    }

    if ( show && parent ) {
        ::EnableWindow(parent, false);
    }

    CenterWindow(parent);

    CMessageLoop loop;
    int res = loop.Run();

    if ( show ) {
        if ( parent ) {
            ::EnableWindow(parent, true);
        }
        ShowWindow(SW_HIDE);
        if ( parent ) {
            ::SetActiveWindow(parent);
        }
    }
    return res;
}

int CWebViewWindow::exec()
{
    return DoModal(0, false);
}

void CWebViewWindow::close(int retCode)
{
    PostQuitMessage(retCode);
}

void CWebViewWindow::abortFromAnotherThread()
{
    PostMessage(WM_CLOSE);
}

void CWebViewWindow::destroyFromAnotherThread()
{
    SendMessage(WM_DESTROYWEBVIEWWINDOW);
}


LRESULT CWebViewWindow::OnClose(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    if ( isModal_ ) {
        PostQuitMessage(0);
    } else {
        ShowWindow(SW_HIDE);
    }
    bHandled = true;
    return 1;
}

LRESULT CWebViewWindow::OnDestroyFromAnotherThread(UINT, WPARAM, LPARAM, BOOL&)
{
    DestroyWindow();
    m_hWnd = 0;
    return 0;
}

bool CWebViewWindow::displayHTML(const CString& html) {
    if (browserView_) {
        browserView_->showHTML(html);
    } else {
        initialHtml_ = html;
    }
    return true;
}

void CWebViewWindow::setOnUrlChanged(std::function<void(const CString&)> cb) {
    onUrlChanged_ = std::move(cb);
}

void CWebViewWindow::setOnDocumentComplete(std::function<void(const CString&)> cb) {
    onDocumentComplete_ = std::move(cb);
}

void CWebViewWindow::setOnNavigateError(std::function<bool(const CString&, LONG)> cb) {
    onNavigateError_ = std::move(cb);
}

std::string CWebViewWindow::runJavaScript(const CString& js) {
    if (browserView_) {
        return browserView_->runJavaScript(js);
    }
    return {};
}

CString CWebViewWindow::getUrl() {
    if (browserView_) {
        return browserView_->getUrl();
    }
    return {};
}
CString CWebViewWindow::getTitle() {
    if (browserView_) {
        return browserView_->getTitle();
    }
    return {};
}

void CWebViewWindow::setBrowserFocus() {
    if (browserView_) {
        browserView_->setFocus();
    } else {
        SetFocus();
    }
}
