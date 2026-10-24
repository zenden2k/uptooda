// WTLBrowserView.cpp : implementation of the CWTLBrowserView class
//
/////////////////////////////////////////////////////////////////////////////


#include "WTLBrowserView.h"

#include <urlmon.h>

#include "Core/Logging.h"
#include "Core/Scripting/API/COMUtils.h"
#include "Core/Utils/CoreUtils.h"
#include "Func/WinUtils.h"

bool CWTLBrowserView::createBrowserView(HWND parentWnd, const RECT& bounds) {
    WinUtils::UseLatestInternetExplorerVersion(false);
    CoInternetSetFeatureEnabled(FEATURE_DISABLE_NAVIGATION_SOUNDS, SET_FEATURE_ON_PROCESS, true);
    HWND res = Create(parentWnd, const_cast<RECT&>(bounds), _T("about:blank"), WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN | WS_HSCROLL | WS_VSCROLL, 0);
    if (!res) {
        return false;
    }
    PutSilent(TRUE); // Suppress javascript errors http://stackoverflow.com/questions/7646055/supressing-script-error-in-ie8-c
    return true;
}

void CWTLBrowserView::resize(const RECT& rc) {
    SetWindowPos(NULL, &rc, SWP_NOMOVE | SWP_NOZORDER);
}

BOOL CWTLBrowserView::PreTranslateMessage(MSG* pMsg)
{
    if((pMsg->message < WM_KEYFIRST || pMsg->message > WM_KEYLAST) &&
       (pMsg->message < WM_MOUSEFIRST || pMsg->message > WM_MOUSELAST))
        return FALSE;

    // give HTML page a chance to translate this message
    return (BOOL)SendMessage(WM_FORWARDMSG, 0, (LPARAM)pMsg);
}


void CWTLBrowserView::OnStatusTextChange(const String& szText)
{
    //m_StatusBar.SetPaneText(ID_DEFAULT_PANE,szText);
}

void CWTLBrowserView::OnProgressChange(long nProgress, long nProgressMax)
{
    CString szText;
    if (nProgressMax>0)
        szText.Format(_T("%d%%"),nProgress*100/nProgressMax);
}

void CWTLBrowserView::OnSetSecureLockIcon(long nSecureLockIcon)
{
    m_bSecured=nSecureLockIcon>0;
}

void CWTLBrowserView::OnNavigateComplete2(IDispatch* pDisp, const CString& szURL)
{
    if ( onUrlChanged_ ) {
        onUrlChanged_(szURL);
    }
}

BOOL CWTLBrowserView::OnNavigateError(IDispatch* pDisp, const String& szURL, const String& szTargetFrameName, LONG nStatusCode)
{
    if ( onNavigateError_ ) {
        return onNavigateError_(szURL, nStatusCode);
    }
    return FALSE;
}

void CWTLBrowserView::OnPropertyChange(const String& szProperty)
{
    //LOG(ERROR) << szProperty;
}

void CWTLBrowserView::OnDocumentComplete(IDispatch* pDisp, const String& szURL)
{
    IUnknown* pUnkBrowser = NULL; 
    IUnknown* pUnkDisp = NULL;
    CString url = szURL;
    /*if ( url.IsEmpty() ) {
        url = GetLocationURL();
    }*/
    HRESULT hr = m_pBrowser->QueryInterface( IID_IUnknown, (void**)&pUnkBrowser ); 
    if ( SUCCEEDED(hr) ) { 
        hr = pDisp->QueryInterface( IID_IUnknown, (void**)&pUnkDisp ); 
        if ( SUCCEEDED(hr) ) { 
            if ( pUnkBrowser == pUnkDisp ) {
                CComQIPtr<IHTMLDocument2> doc2(GetDocument());
                if ( doc2 ) {
                    CComQIPtr<IHTMLWindow2>  pWindow;
                    doc2->get_parentWindow(&pWindow);
                    if ( pWindow && ::IsWindowVisible(GetParent()) ) {
                        pWindow->focus();
                    }
                }
                if ( onDocumentComplete_ ) {
                    onDocumentComplete_(url);
                }
            }
        }
    }
    pUnkDisp->Release(); 
    pUnkBrowser->Release();
}

void CWTLBrowserView::navigateTo(CString url) {
    Navigate(url);
}

bool CWTLBrowserView::showHTML(CString html) {
    return SUCCEEDED(displayHTML(html));
}

std::string CWTLBrowserView::runJavaScript(CString code) {
    CComVariant res;
    if (CallJScript(_T("eval"), code, &res)) {
        return ScriptAPI::ComVariantToString(res);
    }
    return {};
}

CString CWTLBrowserView::getUrl() {
    return GetLocationURL();
}

CString CWTLBrowserView::getTitle() {
    return GetLocationName();
}

void CWTLBrowserView::setFocus() {
    SetFocus();
}