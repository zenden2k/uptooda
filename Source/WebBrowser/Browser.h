/*
Filename: BROWSER.H
Description: Defines an interface for a web browser control, including event sinking.
Date: 08/09/2003

Copyright (c) 2003 by Gilad Novik.  (Web: http://gilad.gsetup.com, Email: gilad@gsetup.com)
All rights reserved.

Copyright / Usage Details
-------------------------
You are allowed to include the source code in any product (commercial, shareware, freeware or otherwise)
when your product is released in binary form. You are allowed to modify the source code in any way you want
except you cannot modify the copyright details at the top of the module. If you want to distribute source
code with your application, then you are only allowed to distribute versions released by the author. This is
to maintain a single distribution point for the source code.
*/

#ifndef _BROWSER_H
#define _BROWSER_H

#include <ExDispid.h>
#include <ComDef.h>
#include <mshtml.h>
#include "Core/Logging.h"

#define CHECK_POINTER(p)\
    ATLASSERT(p != NULL);\
    if(p == NULL)\
    {\
    ShowError(_T("NULL pointer"));\
    return false;\
    }

class CWebBrowser2EventsBase
{
protected:
    static _ATL_FUNC_INFO StatusTextChangeStruct;
    static _ATL_FUNC_INFO TitleChangeStruct;
    static _ATL_FUNC_INFO PropertyChangeStruct;
    static _ATL_FUNC_INFO OnQuitStruct;
    static _ATL_FUNC_INFO OnToolBarStruct;
    static _ATL_FUNC_INFO OnMenuBarStruct;
    static _ATL_FUNC_INFO OnStatusBarStruct;
    static _ATL_FUNC_INFO OnFullScreenStruct;
    static _ATL_FUNC_INFO OnTheaterModeStruct;
    static _ATL_FUNC_INFO DownloadBeginStruct;
    static _ATL_FUNC_INFO DownloadCompleteStruct;
    static _ATL_FUNC_INFO NewWindow2Struct; 
    static _ATL_FUNC_INFO CommandStateChangeStruct;
    static _ATL_FUNC_INFO BeforeNavigate2Struct;
    static _ATL_FUNC_INFO ProgressChangeStruct;
    static _ATL_FUNC_INFO NavigateComplete2Struct;
    static _ATL_FUNC_INFO DocumentComplete2Struct;
    static _ATL_FUNC_INFO OnVisibleStruct;
    static _ATL_FUNC_INFO SetSecureLockIconStruct;
    static _ATL_FUNC_INFO NavigateErrorStruct;
    static _ATL_FUNC_INFO PrivacyImpactedStateChangeStruct;
    //static _ATL_FUNC_INFO FileDownloadStruct;   // Unsafe to use
    //static _ATL_FUNC_INFO WindowClosingStruct;
};
template<class T,UINT nID=0>
class CWebBrowser2 :   public CWebBrowser2EventsBase , public IDispEventSimpleImpl<nID, CWebBrowser2<T,nID>, &DIID_DWebBrowserEvents2>
{
public:
    typedef CWebBrowser2<T, nID> thisClass;
#if defined(_WTL_USE_CSTRING) || defined(__ATLSTR_H__)
    typedef CString String;
#else
    typedef CComBSTR String;
#endif
public:
    enum RefreshConstants
    {
        REFRESH_NORMAL = 0,
        REFRESH_IFEXPIRED = 1,
        REFRESH_CONTINUE = 2,
        REFRESH_COMPLETELY = 3
    };
    
    BEGIN_MSG_MAP(thisClass)
        MESSAGE_HANDLER(WM_CREATE, OnCreate)
        MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    END_MSG_MAP()
        
    BEGIN_SINK_MAP(thisClass)
        SINK_ENTRY_INFO(nID, DIID_DWebBrowserEvents2, DISPID_STATUSTEXTCHANGE, __StatusTextChange, &StatusTextChangeStruct)
        SINK_ENTRY_INFO(nID, DIID_DWebBrowserEvents2, DISPID_PROGRESSCHANGE, __ProgressChange, &ProgressChangeStruct)
        SINK_ENTRY_INFO(nID, DIID_DWebBrowserEvents2, DISPID_COMMANDSTATECHANGE, __CommandStateChange, &CommandStateChangeStruct)
        SINK_ENTRY_INFO(nID, DIID_DWebBrowserEvents2, DISPID_DOWNLOADBEGIN, __DownloadBegin, &DownloadBeginStruct)
        SINK_ENTRY_INFO(nID, DIID_DWebBrowserEvents2, DISPID_DOWNLOADCOMPLETE, __DownloadComplete, &DownloadCompleteStruct)
        SINK_ENTRY_INFO(nID, DIID_DWebBrowserEvents2, DISPID_TITLECHANGE, __TitleChange, &TitleChangeStruct)
        SINK_ENTRY_INFO(nID, DIID_DWebBrowserEvents2, DISPID_NAVIGATECOMPLETE2, __NavigateComplete2, &NavigateComplete2Struct)
        SINK_ENTRY_INFO(nID, DIID_DWebBrowserEvents2, DISPID_BEFORENAVIGATE2, __BeforeNavigate2, &BeforeNavigate2Struct)
        SINK_ENTRY_INFO(nID, DIID_DWebBrowserEvents2, DISPID_PROPERTYCHANGE, __PropertyChange, &PropertyChangeStruct)
        SINK_ENTRY_INFO(nID, DIID_DWebBrowserEvents2, DISPID_NEWWINDOW2, __NewWindow2, &NewWindow2Struct)
        SINK_ENTRY_INFO(nID, DIID_DWebBrowserEvents2, DISPID_DOCUMENTCOMPLETE, __DocumentComplete, &DocumentComplete2Struct)
        SINK_ENTRY_INFO(nID, DIID_DWebBrowserEvents2, DISPID_ONQUIT, __OnQuit, &OnQuitStruct)
        SINK_ENTRY_INFO(nID, DIID_DWebBrowserEvents2, DISPID_ONVISIBLE, __OnVisible, &OnVisibleStruct)
        SINK_ENTRY_INFO(nID, DIID_DWebBrowserEvents2, DISPID_ONTOOLBAR, __OnToolBar, &OnToolBarStruct)
        SINK_ENTRY_INFO(nID, DIID_DWebBrowserEvents2, DISPID_ONMENUBAR, __OnMenuBar, &OnMenuBarStruct)
        SINK_ENTRY_INFO(nID, DIID_DWebBrowserEvents2, DISPID_ONSTATUSBAR, __OnStatusBar, &OnStatusBarStruct)
        SINK_ENTRY_INFO(nID, DIID_DWebBrowserEvents2, DISPID_ONFULLSCREEN, __OnFullScreen, &OnFullScreenStruct)
        SINK_ENTRY_INFO(nID, DIID_DWebBrowserEvents2, DISPID_ONTHEATERMODE, __OnTheaterMode, &OnTheaterModeStruct)
        SINK_ENTRY_INFO(nID, DIID_DWebBrowserEvents2, DISPID_SETSECURELOCKICON, __SetSecureLockIcon, &SetSecureLockIconStruct)
        SINK_ENTRY_INFO(nID, DIID_DWebBrowserEvents2, DISPID_NAVIGATEERROR, __NavigateError, &NavigateErrorStruct)
        SINK_ENTRY_INFO(nID, DIID_DWebBrowserEvents2, DISPID_PRIVACYIMPACTEDSTATECHANGE, __PrivacyImpactedStateChange, &PrivacyImpactedStateChangeStruct)
        //SINK_ENTRY_INFO(nID, DIID_DWebBrowserEvents2, DISPID_WINDOWCLOSING, __WindowClosing, &WindowClosingStruct)
        //SINK_ENTRY_INFO(nID, DIID_DWebBrowserEvents2, DISPID_FILEDOWNLOAD, __FileDownload, &FileDownloadStruct)
    END_SINK_MAP()
        
    CWebBrowser2()
    {
        m_bState[back]=m_bState[forward]=FALSE;
    }
    ~CWebBrowser2()
    {
        return;
    }
    
    LRESULT OnCreate(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
    {
        T* pT = static_cast<T*>(this);
        LRESULT lResult=pT->DefWindowProc();
        HRESULT hResult=pT->QueryControl(IID_IWebBrowser2, (void**)&m_pBrowser);
        if (SUCCEEDED(hResult))
        {
            if (FAILED(DispEventAdvise(m_pBrowser,&DIID_DWebBrowserEvents2)))
            {
                ATLASSERT(FALSE);
                m_pBrowser.Release();
            }
        }
        return lResult;
    }
    
    LRESULT OnDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled)
    {
        if (m_pBrowser)
        {
            DispEventUnadvise(m_pBrowser, &DIID_DWebBrowserEvents2);
            m_pBrowser.Release();
        }
        bHandled=FALSE;
        return 0;
    }
    
    void OnSetSecureLockIcon(long nSecureLockIcon) { }
    BOOL OnNavigateError(IDispatch* pDisp, const String& szURL, const String& szTargetFrameName, LONG nStatusCode)
    {   // Return TRUE to cancel
        return FALSE;
    }
    void OnPrivacyImpactedStateChange(BOOL bImpacted) { }
    void OnStatusTextChange(const String& szText) { }
    void OnProgressChange(long nProgress, long nProgressMax) { }
    void OnCommandStateChange(long nCommand, BOOL bEnable)
    {
        switch (nCommand)
        {
        case CSC_NAVIGATEBACK:
            m_bState[back]=bEnable;
            break;
        case CSC_NAVIGATEFORWARD:
            m_bState[forward]=bEnable;
            break;
        }
    }
    void OnDownloadBegin() { }
    void OnDownloadComplete() { }
    void OnTitleChange(const String& szTitle) { }
    void OnNavigateComplete2(IDispatch* pDisp, const String& szURL) { }
    BOOL OnBeforeNavigate2(IDispatch* pDisp, const String& szURL, DWORD dwFlags, const String& szTargetFrameName, CSimpleArray<BYTE>& pPostedData, const String& szHeaders)
    {   // Return TRUE to cancel
        return FALSE;
    }
    void OnPropertyChange(const String& szProperty) { }
    BOOL OnNewWindow2(IDispatch** ppDisp)
    {   // Return TRUE to cancel
        return FALSE;
    }
    void OnDocumentComplete(IDispatch* pDisp, const String& szURL) { }
    void OnQuit() { }
    void OnVisible(BOOL bVisible) { }
    void OnToolBar(BOOL bToolBar) { }
    void OnMenuBar(BOOL bMenuBar) { }
    void OnStatusBar(BOOL bStatusBar) { }
    void OnFullScreen(BOOL bFullScreen) { }
    void OnTheaterMode(BOOL bTheaterMode) { }
    /*
    BOOL OnWindowClosing(BOOL bChildWindow)
    {   // Return TRUE to cancel
        return FALSE;
    }
    */
    /*
    BOOL OnFileDownload()
    {   // Return TRUE to cancel
        return FALSE;
    }
    */
    
    // Properties
    __declspec(property(get=GetAddressBar,put=PutAddressBar)) BOOL AddressBar;
    __declspec(property(get=GetApplication)) IDispatchPtr Application;
    __declspec(property(get=GetBusy)) BOOL Busy;
    __declspec(property(get=GetFullName)) String FullName;
    __declspec(property(get=GetLocationName)) String LocationName;
    __declspec(property(get=GetLocationURL)) String LocationURL;
    __declspec(property(get=GetRegisterAsBrowser,put=PutRegisterAsBrowser)) BOOL RegisterAsBrowser;
    __declspec(property(get=GetRegisterAsDropTarget,put=PutRegisterAsDropTarget)) BOOL RegisterAsDropTarget;
    __declspec(property(get=GetTheaterMode,put=PutTheaterMode)) BOOL TheaterMode;
    __declspec(property(get=GetVisible,put=PutVisible)) BOOL Visible;
    __declspec(property(get=GetMenuBar,put=PutMenuBar)) BOOL MenuBar;
    __declspec(property(get=GetToolBar,put=PutToolBar)) BOOL ToolBar;
    __declspec(property(get=GetOffline,put=PutOffline)) BOOL Offline;
    __declspec(property(get=GetSilent,put=PutSilent)) BOOL Silent;
    __declspec(property(get=GetFullScreen,put=PutFullScreen)) BOOL FullScreen;
    __declspec(property(get=GetStatusBar,put=PutStatusBar)) BOOL StatusBar;
    __declspec(property(get=GetLeft,put=PutLeft)) LONG Left;
    __declspec(property(get=GetTop,put=PutTop)) LONG Top;
    __declspec(property(get=GetWidth,put=PutWidth)) LONG Width;
    __declspec(property(get=GetHeight,put=PutHeight)) LONG Height;
    __declspec(property(get=GetDocument)) IDispatchPtr Document;

    BOOL CanBack() const
    {
        return m_bState[back];
    }

    BOOL CanForward() const
    {
        return m_bState[forward];
    }
    
    void Quit()
    {
        ATLASSERT(m_pBrowser);
        m_pBrowser->Quit();
    }
    
    void PutAddressBar(BOOL bNewValue)
    {
        ATLASSERT(m_pBrowser);
        m_pBrowser->put_AddressBar(bNewValue ? VARIANT_TRUE : VARIANT_FALSE);
    }
    BOOL GetAddressBar()
    {
        ATLASSERT(m_pBrowser);
        VARIANT_BOOL bResult;
        m_pBrowser->get_AddressBar(&bResult);
        return bResult ? TRUE : FALSE;
    }
    IDispatchPtr GetApplication()
    {
        ATLASSERT(m_pBrowser);
        IDispatchPtr pDispatch;
        m_pBrowser->get_Application(&pDispatch);
        return pDispatch;
    }
    BOOL GetBusy()
    {
        ATLASSERT(m_pBrowser);
        VARIANT_BOOL bResult;
        m_pBrowser->get_Busy(&bResult);
        return bResult ? TRUE : FALSE;
    }
    String GetFullName() const
    {
        ATLASSERT(m_pBrowser);
        CComBSTR szResult;
        m_pBrowser->get_FullName(&szResult);
        return szResult;
    }
    String GetLocationName() const
    {
        ATLASSERT(m_pBrowser);
        CComBSTR szResult;
        m_pBrowser->get_LocationName(&szResult);
        return String(szResult);
    }
    String GetLocationURL() const
    {
        ATLASSERT(m_pBrowser);
        CComBSTR szResult;
        m_pBrowser->get_LocationURL(&szResult);
        return String(szResult);
    }
    void PutRegisterAsBrowser(BOOL bNewValue)
    {
        ATLASSERT(m_pBrowser);
        m_pBrowser->put_RegisterAsBrowser(bNewValue ? VARIANT_TRUE : VARIANT_FALSE);
    }
    BOOL GetRegisterAsBrowser()
    {
        ATLASSERT(m_pBrowser);
        VARIANT_BOOL bResult;
        m_pBrowser->get_RegisterAsBrowser(&bResult);
        return bResult ? TRUE : FALSE;
    }
    void PutRegisterAsDropTarget(BOOL bNewValue)
    {
        ATLASSERT(m_pBrowser);
        m_pBrowser->put_RegisterAsDropTarget(bNewValue ? VARIANT_TRUE : VARIANT_FALSE);
    }
    BOOL GetRegisterAsDropTarget()
    {
        ATLASSERT(m_pBrowser);
        VARIANT_BOOL bResult;
        m_pBrowser->get_RegisterAsDropTarget(&bResult);
        return bResult ? TRUE : FALSE;
    }
    void PutTheaterMode(BOOL bNewValue)
    {
        ATLASSERT(m_pBrowser);
        m_pBrowser->put_TheaterMode(bNewValue ? VARIANT_TRUE : VARIANT_FALSE);
    }
    BOOL GetTheaterMode()
    {
        ATLASSERT(m_pBrowser);
        VARIANT_BOOL bResult;
        m_pBrowser->get_TheaterMode(&bResult);
        return bResult ? TRUE : FALSE;
    }
    void PutVisible(BOOL bNewValue)
    {
        ATLASSERT(m_pBrowser);
        m_pBrowser->put_Visible(bNewValue ? VARIANT_TRUE : VARIANT_FALSE);
    }
    BOOL GetVisible()
    {
        ATLASSERT(m_pBrowser);
        VARIANT_BOOL bResult;
        m_pBrowser->get_Visible(&bResult);
        return bResult ? TRUE : FALSE;
    }
    void PutMenuBar(BOOL bNewValue)
    {
        ATLASSERT(m_pBrowser);
        m_pBrowser->put_MenuBar(bNewValue ? VARIANT_TRUE : VARIANT_FALSE);
    }
    BOOL GetMenuBar()
    {
        ATLASSERT(m_pBrowser);
        VARIANT_BOOL bResult;
        m_pBrowser->get_MenuBar(&bResult);
        return bResult ? TRUE : FALSE;
    }
    void PutToolBar(int nNewValue)
    {
        ATLASSERT(m_pBrowser);
        m_pBrowser->put_ToolBar(nNewValue);
    }
    BOOL GetToolBar()
    {
        ATLASSERT(m_pBrowser);
        int bResult;
        m_pBrowser->get_ToolBar(&bResult);
        return bResult ? TRUE : FALSE;
    }
    void PutOffline(BOOL bNewValue)
    {
        ATLASSERT(m_pBrowser);
        m_pBrowser->put_Offline(bNewValue ? VARIANT_TRUE : VARIANT_FALSE);
    }
    BOOL GetOffline()
    {
        ATLASSERT(m_pBrowser);
        VARIANT_BOOL bResult;
        m_pBrowser->get_Offline(&bResult);
        return bResult ? TRUE : FALSE;
    }
    void PutSilent(BOOL bNewValue)
    {
        ATLASSERT(m_pBrowser);
        m_pBrowser->put_Silent(bNewValue ? VARIANT_TRUE : VARIANT_FALSE);
    }
    BOOL GetSilent()
    {
        ATLASSERT(m_pBrowser);
        VARIANT_BOOL bResult;
        m_pBrowser->get_Silent(&bResult);
        return bResult ? TRUE : FALSE;
    }
    void PutFullScreen(BOOL bNewValue)
    {
        ATLASSERT(m_pBrowser);
        m_pBrowser->put_FullScreen(bNewValue ? VARIANT_TRUE : VARIANT_FALSE);
    }
    BOOL GetFullScreen()
    {
        ATLASSERT(m_pBrowser);
        VARIANT_BOOL bResult;
        m_pBrowser->get_FullScreen(&bResult);
        return bResult ? TRUE : FALSE;
    }
    void PutStatusBar(BOOL bNewValue)
    {
        ATLASSERT(m_pBrowser);
        m_pBrowser->put_StatusBar(bNewValue ? VARIANT_TRUE : VARIANT_FALSE); 
    }
    BOOL GetStatusBar()
    {
        ATLASSERT(m_pBrowser);
        VARIANT_BOOL bResult;
        m_pBrowser->get_StatusBar(&bResult);
        return bResult ? TRUE : FALSE;
    }
    void PutLeft(LONG nNewValue)
    {
        ATLASSERT(m_spBrowser);
        m_pBrowser->put_Left(nNewValue);
    }
    LONG GetLeft()
    {
        ATLASSERT(m_pBrowser);
        LONG nResult;
        m_pBrowser->get_Left(&nResult);
        return nResult;
    }
    void PutTop(LONG nNewValue)
    {
        ATLASSERT(m_pBrowser);
        m_pBrowser->put_Top(nNewValue);
    }
    LONG GetTop()
    {
        ATLASSERT(m_pBrowser);
        LONG nResult;
        m_pBrowser->get_Top(&nResult);
        return nResult;
    }
    void PutWidth(LONG nNewValue)
    {
        ATLASSERT(m_pBrowser);
        m_pBrowser->put_Width(nNewValue);
    }
    LONG GetWidth()
    {
        ATLASSERT(m_pBrowser);
        LONG nResult;
        m_pBrowser->get_Width(&nResult);
        return nResult;
    }
    void PutHeight(LONG nNewValue)
    {
        ATLASSERT(m_pBrowser);
        m_pBrowser->put_Height(nNewValue);
    }
    LONG GetHeight()
    {
        ATLASSERT(m_pBrowser);
        LONG nResult;
        m_pBrowser->get_Height(&nResult);
        return nResult;
    }
    IDispatchPtr GetDocument()
    {
        ATLASSERT(m_pBrowser);
        IDispatchPtr pDispatch;
        m_pBrowser->get_Document(&pDispatch);
        return pDispatch;
    }
    HRESULT PutProperty(LPCTSTR szProperty,const VARIANT& vtValue)
    {
        ATLASSERT(m_pBrowser);
        USES_CONVERSION;
        return m_pBrowser->PutProperty(CComBSTR(szProperty),vtValue);
    }
    CComVariant GetProperty(LPCTSTR szProperty)
    {
        ATLASSERT(m_pBrowser);
        CComDispatchDriver pDriver(m_pBrowser);
        ATLASSERT(pDriver);
        CComVariant vtResult;
        pDriver.GetPropertyByName(CComBSTR(szProperty),&vtResult);
        return vtResult;
    }
    
    // Methods
    HRESULT ClientToWindow(LPPOINT pPoint)
    {
        ATLASSERT(m_pBrowser);
        ATLASSERT(pPoint);
        return m_pBrowser->ClientToWindow(&(pPoint->x),&(pPoint->y));
    }
    HRESULT ExecWB(OLECMDID nCmd,OLECMDEXECOPT nCmdOptions,VARIANT* pvInput=NULL,VARIANT* pvOutput=NULL)
    {
        ATLASSERT(m_pBrowser);
        return m_pBrowser->ExecWB(nCmd,nCmdOptions,pvInput,pvOutput);
    }
    HRESULT GoBack()
    {
        ATLASSERT(m_pBrowser);
        return m_pBrowser->GoBack();
    }
    HRESULT GoForward()
    {
        ATLASSERT(m_pBrowser);
        return m_pBrowser->GoForward();
    }
    HRESULT GoHome()
    {
        ATLASSERT(m_pBrowser);
        return m_pBrowser->GoHome();
    }
    HRESULT GoSearch()
    {
        ATLASSERT(m_pBrowser);
        return m_pBrowser->GoSearch();
    }
    HRESULT Navigate(LPCTSTR szURL,DWORD dwFlags=0,LPCTSTR szTargetFrameName=NULL,LPCVOID pPostData=NULL,DWORD dwPostDataLength=0,LPCTSTR szHeaders=NULL)
    {
        USES_CONVERSION;
        ATLASSERT(m_pBrowser);
        ATLASSERT(szURL);
        CComVariant vtTargetFrameName,vtPostData,vtHeaders;
        if (szTargetFrameName)
            vtTargetFrameName=szTargetFrameName;
        if (pPostData && dwPostDataLength>0)
        {
            vtPostData.ChangeType(VT_ARRAY|VT_UI1);
            SAFEARRAYBOUND Bound;
            Bound.cElements=dwPostDataLength;
            Bound.lLbound=0;
            vtPostData.parray=SafeArrayCreate(VT_UI1,1,&Bound);
            ATLASSERT(vtPostData.parray);
            if (vtPostData.parray==NULL)
                return E_OUTOFMEMORY;
            LPVOID pData;
            SafeArrayAccessData(vtPostData.parray,&pData);
            CopyMemory(pData,pPostData,dwPostDataLength);
            SafeArrayUnaccessData(vtPostData.parray);
        }
        if (szHeaders)
            vtHeaders=szHeaders;
        CComVariant flags((LONG)dwFlags);
        return m_pBrowser->Navigate(T2BSTR(szURL), &flags,&vtTargetFrameName,&vtPostData,&vtHeaders);
    }
    HRESULT Navigate2(LPCTSTR szURL,DWORD dwFlags=0,LPCTSTR szTargetFrameName=NULL,LPCVOID pPostData=NULL,DWORD dwPostDataLength=0,LPCTSTR szHeaders=NULL)
    {
        ATLASSERT(m_pBrowser);
        ATLASSERT(szURL);
        CComVariant vtTargetFrameName,vtPostData,vtHeaders;
        if (szTargetFrameName)
            vtTargetFrameName=szTargetFrameName;
        if (pPostData && dwPostDataLength>0)
        {
            vtPostData.ChangeType(VT_ARRAY|VT_UI1);
            SAFEARRAYBOUND Bound;
            Bound.cElements=dwPostDataLength;
            Bound.lLbound=0;
            vtPostData.parray=SafeArrayCreate(VT_UI1,1,&Bound);
            ATLASSERT(vtPostData.parray);
            if (vtPostData.parray==NULL)
                return E_OUTOFMEMORY;
            LPVOID pData=NULL;
            SafeArrayAccessData(vtPostData.parray,&pData);
            ATLASSERT(pData);
            CopyMemory(pData,pPostData,dwPostDataLength);
            SafeArrayUnaccessData(vtPostData.parray);
        }
        if (szHeaders)
            vtHeaders=szHeaders;
        return m_pBrowser->Navigate2(&CComVariant(szURL),&CComVariant((LONG)dwFlags),&vtTargetFrameName,&vtPostData,&vtHeaders);
    }
    HRESULT Refresh()
    {
        ATLASSERT(m_pBrowser);
        return m_pBrowser->Refresh();
    }
    HRESULT Refresh2(LONG nLevel)
    {
        ATLASSERT(m_pBrowser);
        return m_pBrowser->Refresh2(CComVariant(nLevel));
    }
    HRESULT Stop()
    {
        ATLASSERT(m_pBrowser);
        return m_pBrowser->Stop();
    }
    IWebBrowser2* GetBrowserInterface() {
        return m_pBrowser;
    }
    OLECMDF QueryStatusWB(OLECMDID nCmd)
    {
        ATLASSERT(m_pBrowser);
        OLECMDF nResult;
        m_pBrowser->QueryStatusWB(nCmd,&nResult);
        return nResult;
    }

    // https://msdn.microsoft.com/en-us/library/aa752047(v=vs.85).aspx
    HRESULT LoadWebBrowserFromStream(IWebBrowser* pWebBrowser, IStream* pStream)
    {
        HRESULT hr;
        IDispatch* pHtmlDoc = NULL;
        IPersistStreamInit* pPersistStreamInit = NULL;

        // Retrieve the document object.
        hr = pWebBrowser->get_Document( &pHtmlDoc );
        if ( SUCCEEDED(hr) )
        {
            // Query for IPersistStreamInit.
            hr = pHtmlDoc->QueryInterface( IID_IPersistStreamInit,  (void**)&pPersistStreamInit );
            if ( SUCCEEDED(hr) )
            {
                // Initialize the document.
                hr = pPersistStreamInit->InitNew();
                if ( SUCCEEDED(hr) )
                {
                    // Load the contents of the stream.
                    hr = pPersistStreamInit->Load( pStream );
                    return hr;
                }
                pPersistStreamInit->Release();
            }
            pHtmlDoc->Release();
        }
        return S_FALSE;
    }

    
    HRESULT displayHTML(const CString& szHTMLText) {
        // Create a stream containing the HTML.
        HRESULT hr = E_FAIL;
         HGLOBAL hHTMLText;
          IStream* pStream = NULL;
        size_t cchLength = szHTMLText.GetLength();
        //  TODO: Safely determine the length of szHTMLText in TCHAR.
        hHTMLText = GlobalAlloc( GPTR, (cchLength+1 ) * sizeof(TCHAR));

        if ( hHTMLText )
        {
            //size_t cchMax = 256;
            lstrcpy((TCHAR*)hHTMLText, /*cchMax + 1,*/ szHTMLText);
            //  TODO: Add error handling code here.

            hr = CreateStreamOnHGlobal( hHTMLText, TRUE, &pStream );
            if ( SUCCEEDED(hr) )
            {
                // Call the helper function to load the browser from the stream.
                LoadWebBrowserFromStream( m_pBrowser, pStream  );
                pStream->Release();
            }
            GlobalFree( hHTMLText );
        }

        return hr;
    }

    bool injectJavaScript(const CString& javascript) {
        CComQIPtr<IHTMLDocument2,&IID_IHTMLDocument2> pDocument2(GetDocument());
        CComPtr<IHTMLElement> pScriptElement; 
        if(pDocument2 && 
            SUCCEEDED(pDocument2->createElement(CComBSTR("SCRIPT"), &pScriptElement)))
        {
            if(pScriptElement)
            {
                CComQIPtr<IHTMLScriptElement> pScript(pScriptElement);

                if(pScript &&
                    SUCCEEDED(pScript->put_text(ATL::CComBSTR(javascript)))) {
                        return true;
                }
            }
        }
        return false;
    }

    bool injectJavaScript2(const CString& javascript) {
        CComPtr<IHTMLElement> bodyEle;
        CComQIPtr<IHTMLDocument2,&IID_IHTMLDocument2> spHTML(GetDocument());
        IHTMLWindow2 window;
        spHTML->get_parentWindow(&window);
        //window->execScript()
        spHTML->get_body(&bodyEle);
        CComBSTR code = "<div style='display:none;'>&nbsp;<script type='text/javascript'>"+ javascript +     "</script></div>";
        CComBSTR whereStr(L"afterBegin");
        return bodyEle->insertAdjacentHTML(whereStr, code) == ERROR_SUCCESS;
    }


    const CString GetSystemErrorMessage(DWORD dwError)
    {
        CString strError;
        LPTSTR lpBuffer;

        if(!FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,  dwError,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_SYS_DEFAULT),
            (LPTSTR) &lpBuffer, 0, NULL))

        {
            strError = "FormatMessage Netive Error" ;
        }
        else
        {
            strError = lpBuffer;
            LocalFree(lpBuffer);
        }
        return strError;
    }

    inline void ShowError(LPCTSTR lpszText)
    {
        LOG(ERROR) << "JSCall Error:\n" + CString(lpszText);
    }



    bool GetJScript(CComPtr<IDispatch>& spDisp)
    {
        CComQIPtr<IHTMLDocument2,&IID_IHTMLDocument2> m_spDoc(GetDocument());
        CHECK_POINTER(m_spDoc);
        HRESULT hr = m_spDoc->get_Script(&spDisp);
        ATLASSERT(SUCCEEDED(hr));
        return SUCCEEDED(hr);
    }

    bool GetJScripts(CComPtr<IHTMLElementCollection>& spColl)
    {
        CHECK_POINTER(m_spDoc);
        HRESULT hr = m_spDoc->get_scripts(&spColl);
        ATLASSERT(SUCCEEDED(hr));
        return SUCCEEDED(hr);
    }

    bool CallJScript(const CString& strFunc,CComVariant* pVarResult)
    {
        CAtlArray<CString> paramArray;
        return CallJScript(strFunc,paramArray,pVarResult);
    }

    bool CallJScript(const CString& strFunc,const CString& strArg1,CComVariant* pVarResult)
    {
        CAtlArray<CString> paramArray;
        paramArray.Add(strArg1);
        return CallJScript(strFunc,paramArray,pVarResult);
    }

    bool CallJScript(const CString& strFunc,const CString& strArg1,const CString strArg2,CComVariant* pVarResult)
    {
        CAtlArray<CString> paramArray;
        paramArray.Add(strArg1);
        paramArray.Add(strArg2);
        return CallJScript(strFunc,paramArray,pVarResult);
    }

    bool CallJScript(const CString& strFunc,const CString& strArg1,const CString& strArg2,const CString& strArg3,CComVariant* pVarResult)
    {
        CAtlArray<CString> paramArray;
        paramArray.Add(strArg1);
        paramArray.Add(strArg2);
        paramArray.Add(strArg3);
        return CallJScript(strFunc,paramArray,pVarResult);
    }

    bool CallJScript(const CString& strFunc, const CAtlArray<CString>& paramArray,CComVariant* pVarResult)
    {
        CComPtr<IDispatch> spScript;
        if(!GetJScript(spScript))
        {
            ShowError(_T("Cannot GetScript"));
            return false;
        }
        CComBSTR bstrMember(strFunc);
        DISPID dispid = NULL;
        HRESULT hr = spScript->GetIDsOfNames(IID_NULL,&bstrMember,1,
            LOCALE_SYSTEM_DEFAULT,&dispid);
        if(FAILED(hr))
        {
            ShowError(GetSystemErrorMessage(hr));
            return false;
        }

        const int arraySize = paramArray.GetCount();

        DISPPARAMS dispparams;
        memset(&dispparams, 0, sizeof dispparams);
        dispparams.cArgs = arraySize;
        dispparams.rgvarg = new VARIANT[dispparams.cArgs];

        for( int i = 0; i < arraySize; i++)
        {
            CComBSTR bstr = paramArray.GetAt(arraySize - 1 - i); // back reading
            bstr.CopyTo(&dispparams.rgvarg[i].bstrVal);
            dispparams.rgvarg[i].vt = VT_BSTR;
        }
        dispparams.cNamedArgs = 0;

        EXCEPINFO excepInfo;
        memset(&excepInfo, 0, sizeof excepInfo);
        CComVariant vaResult;
        UINT nArgErr = (UINT)-1;  // initialize to invalid arg

        hr = spScript->Invoke(dispid,IID_NULL,0,
            DISPATCH_METHOD,&dispparams,&vaResult,&excepInfo,&nArgErr);

        delete [] dispparams.rgvarg;
        if(FAILED(hr))
        {
            ShowError(GetSystemErrorMessage(hr));
            return false;
        }

        if(pVarResult)
        {
            *pVarResult = vaResult;
        }
        return true;
    }


    HRESULT LoadFromResource(UINT nID) 
    {
        TCHAR szFilename[MAX_PATH];
        GetModuleFileName(_Module.GetModuleInstance(),szFilename,sizeof(szFilename)/sizeof(TCHAR));
        TCHAR szURL[4096];
        _stprintf(szURL,_T("res://%s/%d"),szFilename,nID);
        return Navigate(szURL);
    }
    HRESULT LoadFromResource(LPCTSTR szID) 
    {
        TCHAR szFilename[MAX_PATH];
        GetModuleFileName(_Module.GetModuleInstance(),szFilename,sizeof(szFilename)/sizeof(TCHAR));
        TCHAR szURL[4096];
        _stprintf(szURL,_T("res://%s/%s"),szFilename,szID);
        return Navigate(szURL);
    }

    CString GetHTML() {
        IDispatchPtr spDisp = GetDocument();
        /*IPersistStream* pPersistStream = 0;
        HRESULT hr = doc->QueryInterface(
            IID_IPersistStream, (void**)&pPersistStream);*/
        CComQIPtr<IHTMLDocument2, &IID_IHTMLDocument2> spHTML; 
        spHTML = spDisp; 
        if (spHTML) 
        { 
            HRESULT hr; 
            CComPtr<IHTMLElement> pBody; 
            hr = spHTML->get_body(&pBody); 
            if (SUCCEEDED(hr)) 
            { 
                BSTR bstrHTMLText; 
                hr = pBody->get_outerHTML(&bstrHTMLText); 
                if (SUCCEEDED(hr)) 
                { 
                    CString res = bstrHTMLText;
                    return res;
                    /*int iLen = SysStringLen(bstrHTMLText), iRes; 
                    LPTSTR psz = new TCHAR[iLen+1]; 
                    ZeroMemory(psz, iLen+1); 
                    iRes = WideCharToMultiByte(CP_ACP, 0, bstrHTMLText, iLen, psz, 
                        iLen, NULL, NULL); 
                    if (iRes == iLen) 
                    { 
                        // Do the job right now ! 
                        delete [] psz; 
                    } */
                } 
            } 
            else 
            { 
                // not an HTML doc 
            } 
        }
        return CString();
    }
    
protected:
    enum
    {
        back = 0,
        forward
    };
    CComPtr<IWebBrowser2> m_pBrowser;
    BOOL m_bState[2];
    
private:
    void __stdcall __SetSecureLockIcon(long nSecureLockIcon)
    {
        T* pT = static_cast<T*>(this);
        pT->OnSetSecureLockIcon(nSecureLockIcon);
    }
    
    void __stdcall __NavigateError(IDispatch* pDisp, VARIANT* pvURL, VARIANT* pvTargetFrameName, VARIANT* pvStatusCode, VARIANT_BOOL* pbCancel)
    {
        T* pT = static_cast<T*>(this);
        ATLASSERT(V_VT(pvURL) == VT_BSTR);
        ATLASSERT(V_VT(pvTargetFrameName) == VT_BSTR);
        ATLASSERT(V_VT(pvStatusCode) == (VT_I4));
        ATLASSERT(pbCancel != NULL);
        *pbCancel = static_cast<VARIANT_BOOL>(pT->OnNavigateError(pDisp, V_BSTR(pvURL), V_BSTR(pvTargetFrameName), V_I4(pvStatusCode)));
    }
    
    void __stdcall __PrivacyImpactedStateChange(VARIANT_BOOL bImpacted)
    {
        T* pT = static_cast<T*>(this);
        pT->OnPrivacyImpactedStateChange(bImpacted);
    }
    
    void __stdcall __StatusTextChange(BSTR szText)
    {
        T* pT = static_cast<T*>(this);
        pT->OnStatusTextChange(szText);
    }
    
    void __stdcall __ProgressChange(long nProgress, long nProgressMax)
    {
        T* pT = static_cast<T*>(this);
        pT->OnProgressChange(nProgress, nProgressMax);
    }
    
    void __stdcall __CommandStateChange(long nCommand, VARIANT_BOOL bEnable)
    {
        T* pT = static_cast<T*>(this);
        pT->OnCommandStateChange(nCommand, (bEnable==VARIANT_TRUE) ? TRUE : FALSE);
    }
    
    void __stdcall __DownloadBegin()
    {
        T* pT = static_cast<T*>(this);
        pT->OnDownloadBegin();
    }
    
    void __stdcall __DownloadComplete()
    {
        T* pT = static_cast<T*>(this);
        pT->OnDownloadComplete();
    }
    
    void __stdcall __TitleChange(BSTR szText)
    {
        T* pT = static_cast<T*>(this);
        pT->OnTitleChange(szText);
    }
    
    void __stdcall __NavigateComplete2(IDispatch* pDisp, VARIANT* pvURL)
    {
        T* pT = static_cast<T*>(this);
        ATLASSERT(V_VT(pvURL) == VT_BSTR);
        pT->OnNavigateComplete2(pDisp, V_BSTR(pvURL));
    }
    
    void __stdcall __BeforeNavigate2(IDispatch* pDisp, VARIANT* pvURL, VARIANT* pvFlags, VARIANT* pvTargetFrameName, VARIANT* pvPostData, VARIANT* pvHeaders, VARIANT_BOOL* pbCancel)
    {
        T* pT = static_cast<T*>(this);
        ATLASSERT(V_VT(pvURL) == VT_BSTR);
        ATLASSERT(V_VT(pvTargetFrameName) == VT_BSTR);
        ATLASSERT(V_VT(pvPostData) == (VT_VARIANT | VT_BYREF));
        ATLASSERT(V_VT(pvHeaders) == VT_BSTR);
        ATLASSERT(pbCancel != NULL);
        
        VARIANT* vtPostedData = V_VARIANTREF(pvPostData);
        CSimpleArray<BYTE> pArray;
        if (V_VT(vtPostedData) & VT_ARRAY)
        {
            ATLASSERT(V_ARRAY(vtPostedData)->cDims == 1 && V_ARRAY(vtPostedData)->cbElements == 1);
            long nLowBound=0,nUpperBound=0;
            SafeArrayGetLBound(V_ARRAY(vtPostedData), 1, &nLowBound);
            SafeArrayGetUBound(V_ARRAY(vtPostedData), 1, &nUpperBound);
            DWORD dwSize=nUpperBound+1-nLowBound;
            LPVOID pData=NULL;
            SafeArrayAccessData(V_ARRAY(vtPostedData),&pData);
            ATLASSERT(pData);
            
            pArray.m_nSize=pArray.m_nAllocSize=dwSize;
            pArray.m_aT=(BYTE*)malloc(dwSize);
            ATLASSERT(pArray.m_aT);
            CopyMemory(pArray.GetData(), pData, dwSize);
            SafeArrayUnaccessData(V_ARRAY(vtPostedData));
        }
        *pbCancel=pT->OnBeforeNavigate2(pDisp, V_BSTR(pvURL), V_I4(pvFlags), V_BSTR(pvTargetFrameName), pArray, V_BSTR(pvHeaders)) ? VARIANT_TRUE : VARIANT_FALSE;
    }
    
    void __stdcall __PropertyChange(BSTR szProperty)
    {
        T* pT = static_cast<T*>(this);
        pT->OnPropertyChange(szProperty);
    }
    
    void __stdcall __NewWindow2(IDispatch** ppDisp, VARIANT_BOOL* pbCancel)
    {
        T* pT = static_cast<T*>(this);
        *pbCancel = pT->OnNewWindow2(ppDisp) ? VARIANT_TRUE : VARIANT_FALSE;
    }
    
    void __stdcall __DocumentComplete(IDispatch* pDisp, VARIANT* pvURL)
    {
        T* pT = static_cast<T*>(this);
        ATLASSERT(V_VT(pvURL) == VT_BSTR);
        pT->OnDocumentComplete(pDisp, V_BSTR(pvURL));
    }
    
    void __stdcall __OnQuit()
    {
        T* pT = static_cast<T*>(this);
        pT->OnQuit();
    }
    
    void __stdcall __OnVisible(VARIANT_BOOL bVisible)
    {
        T* pT = static_cast<T*>(this);
        pT->OnVisible(bVisible == VARIANT_TRUE ? TRUE : FALSE);
    }
    
    void __stdcall __OnToolBar(VARIANT_BOOL bToolBar)
    {
        T* pT = static_cast<T*>(this);
        pT->OnToolBar(bToolBar == VARIANT_TRUE ? TRUE : FALSE);
    }
    
    void __stdcall __OnMenuBar(VARIANT_BOOL bMenuBar)
    {
        T* pT = static_cast<T*>(this);
        pT->OnMenuBar(bMenuBar == VARIANT_TRUE ? TRUE : FALSE);
    }
    
    void __stdcall __OnStatusBar(VARIANT_BOOL bStatusBar)
    {
        T* pT = static_cast<T*>(this);
        pT->OnStatusBar(bStatusBar == VARIANT_TRUE ? TRUE : FALSE);
    }
    
    void __stdcall __OnFullScreen(VARIANT_BOOL bFullScreen)
    {
        T* pT = static_cast<T*>(this);
        pT->OnFullScreen(bFullScreen == VARIANT_TRUE ? TRUE : FALSE);
    }
    
    void __stdcall __OnTheaterMode(VARIANT_BOOL bTheaterMode)
    {
        T* pT = static_cast<T*>(this);
        pT->OnTheaterMode(bTheaterMode == VARIANT_TRUE ? TRUE : FALSE);
    }
    
    /*
    void __stdcall __FileDownload(VARIANT_BOOL* pbCancel)
    {
        T* pT = static_cast<T*>(this);
        *pbCancel = pT->OnFileDownload() ? VARIANT_TRUE : VARIANT_FALSE;
    }
    */
    /*
    void __stdcall __WindowClosing(VARIANT_BOOL bChildWindow,VARIANT_BOOL* pbCancel)
    {
        T* pT = static_cast<T*>(this);
        *pbCancel=pT->OnWindowClosing(bChildWindow ? TRUE : FALSE);
    }
    */
};

__declspec(selectany) _ATL_FUNC_INFO CWebBrowser2EventsBase::StatusTextChangeStruct = {CC_STDCALL, VT_EMPTY, 1, {VT_BSTR}};
__declspec(selectany) _ATL_FUNC_INFO CWebBrowser2EventsBase::TitleChangeStruct = {CC_STDCALL, VT_EMPTY, 1, {VT_BSTR}};
__declspec(selectany) _ATL_FUNC_INFO CWebBrowser2EventsBase::PropertyChangeStruct = {CC_STDCALL, VT_EMPTY, 1, {VT_BSTR}};
__declspec(selectany) _ATL_FUNC_INFO CWebBrowser2EventsBase::DownloadBeginStruct = {CC_STDCALL, VT_EMPTY, 0, {NULL}};
__declspec(selectany) _ATL_FUNC_INFO CWebBrowser2EventsBase::DownloadCompleteStruct = {CC_STDCALL, VT_EMPTY, 0, {NULL}};
__declspec(selectany) _ATL_FUNC_INFO CWebBrowser2EventsBase::OnQuitStruct = {CC_STDCALL, VT_EMPTY, 0, {NULL}};
__declspec(selectany) _ATL_FUNC_INFO CWebBrowser2EventsBase::NewWindow2Struct = {CC_STDCALL, VT_EMPTY, 2, {VT_BYREF|VT_BOOL,VT_BYREF|VT_DISPATCH}}; 
__declspec(selectany) _ATL_FUNC_INFO CWebBrowser2EventsBase::CommandStateChangeStruct = {CC_STDCALL, VT_EMPTY, 2, {VT_I4,VT_BOOL}};
__declspec(selectany) _ATL_FUNC_INFO CWebBrowser2EventsBase::BeforeNavigate2Struct = {CC_STDCALL, VT_EMPTY, 7, {VT_DISPATCH,VT_BYREF|VT_VARIANT,VT_BYREF|VT_VARIANT,VT_BYREF|VT_VARIANT,VT_BYREF|VT_VARIANT,VT_BYREF|VT_VARIANT,VT_BYREF|VT_BOOL}};
__declspec(selectany) _ATL_FUNC_INFO CWebBrowser2EventsBase::ProgressChangeStruct = {CC_STDCALL, VT_EMPTY, 2, {VT_I4,VT_I4}};
__declspec(selectany) _ATL_FUNC_INFO CWebBrowser2EventsBase::NavigateComplete2Struct = {CC_STDCALL, VT_EMPTY, 2, {VT_DISPATCH, VT_BYREF|VT_VARIANT}};
__declspec(selectany) _ATL_FUNC_INFO CWebBrowser2EventsBase::DocumentComplete2Struct = {CC_STDCALL, VT_EMPTY, 2, {VT_DISPATCH, VT_BYREF|VT_VARIANT}};
__declspec(selectany) _ATL_FUNC_INFO CWebBrowser2EventsBase::OnVisibleStruct = {CC_STDCALL, VT_EMPTY, 1, {VT_BOOL}};
__declspec(selectany) _ATL_FUNC_INFO CWebBrowser2EventsBase::OnToolBarStruct = {CC_STDCALL, VT_EMPTY, 1, {VT_BOOL}};
__declspec(selectany) _ATL_FUNC_INFO CWebBrowser2EventsBase::OnMenuBarStruct = {CC_STDCALL, VT_EMPTY, 1, {VT_BOOL}};
__declspec(selectany) _ATL_FUNC_INFO CWebBrowser2EventsBase::OnStatusBarStruct = {CC_STDCALL, VT_EMPTY, 1, {VT_BOOL}};
__declspec(selectany) _ATL_FUNC_INFO CWebBrowser2EventsBase::OnFullScreenStruct = {CC_STDCALL, VT_EMPTY, 1, {VT_BOOL}};
__declspec(selectany) _ATL_FUNC_INFO CWebBrowser2EventsBase::OnTheaterModeStruct = {CC_STDCALL, VT_EMPTY, 1, {VT_BOOL}};
__declspec(selectany) _ATL_FUNC_INFO CWebBrowser2EventsBase::SetSecureLockIconStruct = {CC_STDCALL, VT_EMPTY, 1, {VT_I4}};
__declspec(selectany) _ATL_FUNC_INFO CWebBrowser2EventsBase::NavigateErrorStruct = {CC_STDCALL, VT_EMPTY, 5, {VT_BYREF|VT_BOOL,VT_BYREF|VT_VARIANT,VT_BYREF|VT_VARIANT,VT_BYREF|VT_VARIANT,VT_BYREF|VT_DISPATCH}};
__declspec(selectany) _ATL_FUNC_INFO CWebBrowser2EventsBase::PrivacyImpactedStateChangeStruct = {CC_STDCALL, VT_EMPTY, 1, {VT_BOOL}};
//__declspec(selectany) _ATL_FUNC_INFO CWebBrowser2EventsBase::FileDownloadStruct = {CC_STDCALL, VT_EMPTY, 1, {VT_BYREF|VT_BOOL}};
//__declspec(selectany) _ATL_FUNC_INFO CWebBrowser2EventsBase::WindowClosingStruct = {CC_STDCALL, VT_EMPTY, 2, {VT_BYREF|VT_BOOL,VT_BOOL}};

#endif
