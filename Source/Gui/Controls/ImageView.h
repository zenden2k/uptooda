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

#pragma once

#include <future>
#include <memory>

#include "MyImage.h"
#include "ProgressRingControl.h"
#include "atlheaders.h"
#include "resource.h" // main symbols


// CImageView

class GdiPlusImage;
struct CImageViewItem {
    CImageViewItem() {
        index = -1;
    }
    CString fileName;
    int index;
};

class CImageViewCallback {
public:
    virtual CImageViewItem getNextImgViewItem(const CImageViewItem& currentItem) = 0;
    virtual CImageViewItem getPrevImgViewItem(const CImageViewItem& currentItem) = 0;

    virtual ~CImageViewCallback() = default;
};
class CImageViewWindow : public CDialogImpl<CImageViewWindow> {
public:
    CImageViewWindow();
    ~CImageViewWindow();
    enum { IDD = IDD_IMAGEVIEW };

    BEGIN_MSG_MAP(CImageView)
    MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
    MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
    MESSAGE_HANDLER(WM_TIMER, OnTimer)
    MSG_WM_ACTIVATE(OnActivate)
    MSG_WM_SHOWWINDOW(onShowWindow)
    MSG_WM_KEYDOWN(OnKeyDown)
    MSG_WM_LBUTTONDOWN(onClick)
    MSG_WM_RBUTTONDOWN(onClick)
    MSG_WM_MBUTTONUP(onClick)
    COMMAND_HANDLER(IDOK, BN_CLICKED, OnClickedOK)
    COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedOK)
    END_MSG_MAP()
    // Handler prototypes:
    //  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    //  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    //  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

    LRESULT OnKeyDown(TCHAR vk, UINT cRepeat, UINT flags);
    void setCallback(CImageViewCallback* callback);
    bool ViewImage(const CImageViewItem&, HWND Parent = NULL);
    LRESULT OnActivate(UINT state, BOOL fMinimized, HWND hwndActDeact);
    void onShowWindow(BOOL show, UINT status);
    void MyCenterWindow(HWND hWndCenter, int width, int height);
    void hide();

protected:
    CImageViewCallback* callback_;
    CImageViewItem currentItem_;
    HWND currentParent_;
    CMyImage imageControl_;

private:
    static constexpr UINT_PTR IMAGE_LOAD_TIMER = 1;
    static constexpr UINT LOADING_DELAY_MS = 300;
    static constexpr int MIN_PREVIEW_WIDTH = 200;
    static constexpr int MIN_PREVIEW_HEIGHT = 180;

    void checkImageLoad();
    LRESULT onClick(UINT flags, CPoint point);
    void showLoadingIndicator();
    void hideLoadingIndicator();
    void showPreviewWindow();
    void showImage(std::unique_ptr<GdiPlusImage> image);
    void resetView();

    std::shared_ptr<std::future<std::unique_ptr<GdiPlusImage>>> imageLoad_;
    std::unique_ptr<CProgressRingControl> progressRing_;
    ULONGLONG loadingDeadline_ = 0;
    HWND requestForeground_ = nullptr;
    bool hasPreview_ = false;
};
