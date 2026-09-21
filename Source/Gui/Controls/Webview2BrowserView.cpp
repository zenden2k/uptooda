#include "Webview2BrowserView.h"

#include <ComDef.h>


#include "Core/AppRuntimeInfo.h"
#include "Core/Utils/CoreUtils.h"

bool Webview2BrowserView::createBrowserView(HWND parentWnd, const RECT& bounds) {
    using namespace Microsoft::WRL;

    HRESULT hr = CreateCoreWebView2EnvironmentWithOptions(nullptr, U2W(AppRuntimeInfo::instance()->settingsDirectory()), nullptr,
        Callback<ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler>(
            [parentWnd, this](HRESULT result, ICoreWebView2Environment* env) -> HRESULT {

                // Create a CoreWebView2Controller and get the associated CoreWebView2 whose parent is the main window hWnd
                env->CreateCoreWebView2Controller(parentWnd, Callback<ICoreWebView2CreateCoreWebView2ControllerCompletedHandler>(
                    [parentWnd, this](HRESULT result, ICoreWebView2Controller* controller) -> HRESULT {
                        if (controller != nullptr) {
                            webviewController_ = controller;
                            HRESULT hr2 = webviewController_->get_CoreWebView2(&webviewWindow_);
                            if (FAILED(hr2)) {
                                _com_error err(hr2);
                                LOG(ERROR) << err.ErrorMessage();
                                return E_FAIL;
                            }
                        }

                        if (!webviewWindow_) {
                            return E_FAIL;
                        }
                        //LOG(ERROR) << webviewWindow_;
                        // Add a few settings for the webview
                        // The demo step is redundant since the values are the default settings
                        /*ICoreWebView2Settings* Settings;
                        webviewWindow_->get_Settings(&Settings);
                        Settings->put_IsScriptEnabled(TRUE);
                        Settings->put_AreDefaultScriptDialogsEnabled(TRUE);
                        Settings->put_IsWebMessageEnabled(TRUE);*/

                        // Resize WebView to fit the bounds of the parent window
                        RECT bounds;
                        ::GetClientRect(parentWnd, &bounds);
                        HRESULT hr2 = webviewController_->put_Bounds(bounds);
                        if (FAILED(hr2)) {
                            LOG(ERROR) << "Failed to put bounds on webview";
                        }
                        // Schedule an async task to navigate to initial url
                        if (!initialUrl_.IsEmpty()) {
                            webviewWindow_->Navigate(initialUrl_);
                        }

                        EventRegistrationToken token;
                        webviewWindow_->add_NavigationStarting(Callback<ICoreWebView2NavigationStartingEventHandler>(
                            [this](ICoreWebView2* webview, ICoreWebView2NavigationStartingEventArgs* args) -> HRESULT {
                                PWSTR uri;
                                args->get_Uri(&uri);
                                std::wstring source(uri);
                                if (onUrlChanged_) {
                                    onUrlChanged_(source.c_str());
                                }

                                CoTaskMemFree(uri);
                                return S_OK;
                            }).Get(), &token);

                        /*webviewWindow_->add_NavigationCompleted(Callback<ICoreWebView2NavigationCompletedEventHandler>(
                               [this](ICoreWebView2* webview, ICoreWebView2NavigationCompletedEventArgs * args) -> HRESULT {
                                   PWSTR uri;
                                   args->(&uri);
                                   std::wstring source(uri);
                                   if (onUrlChanged_) {
                                       onUrlChanged_(source.c_str());
                                   }

                                   CoTaskMemFree(uri);
                                   return S_OK;
                        }).Get(), &token); */

                        return S_OK;
                    }).Get());
                return S_OK;
            }).Get());

    return SUCCEEDED(hr);
}

void Webview2BrowserView::resize(const RECT& rc) {
    if (webviewController_) {
        webviewController_->put_Bounds(rc);
    }
}

void Webview2BrowserView::navigateTo(CString url) {
    if (webviewWindow_) {
        webviewWindow_->Navigate(url);
    } else {
        initialUrl_ = url;
    }
}

bool Webview2BrowserView::showHTML(CString html) {
    return false; // Not implemented
}

std::string Webview2BrowserView::runJavaScript(CString code) {
    return {}; // Not implemented
}

CString Webview2BrowserView::getUrl() {
    return {}; // Not implemented
}

CString Webview2BrowserView::getTitle() {
    return {}; // Not implemented
}

void Webview2BrowserView::setFocus() {
    // Not implemented
}