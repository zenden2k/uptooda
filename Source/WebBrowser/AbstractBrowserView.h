#ifndef IU_GUI_CONTROLS_ABSTRACTBROWSERVIEW_H
#define IU_GUI_CONTROLS_ABSTRACTBROWSERVIEW_H

#include "atlheaders.h"

#include <functional>
#include <string>

class AbstractBrowserView {
public:
    virtual ~AbstractBrowserView() = default;
    virtual bool createBrowserView(HWND parentWnd, const RECT& bounds) = 0;
    virtual void resize(const RECT& rc) = 0;
    virtual void navigateTo(CString url) = 0;
    virtual bool showHTML(CString html) = 0;
    virtual std::string runJavaScript(CString code) = 0;
    virtual CString getUrl() = 0;
    virtual CString getTitle() = 0;
    virtual void setFocus() = 0;

    void setOnUrlChanged(std::function<void(const CString&)> cb) {
        onUrlChanged_ = std::move(cb);
    }
    void setOnDocumentComplete(std::function<void(const CString&)> cb) {
        onDocumentComplete_ = std::move(cb);
    }
    void setOnNavigateError(std::function<bool(const CString&, LONG)> cb) {
        onNavigateError_ = std::move(cb);
    }

protected:
    std::function<void(const CString&)> onUrlChanged_;
    std::function<void(const CString&)> onDocumentComplete_;
    std::function<bool(const CString&, LONG)> onNavigateError_;
};

#endif
