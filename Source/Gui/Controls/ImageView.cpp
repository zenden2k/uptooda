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

#include "ImageView.h"

#include <algorithm>
#include <chrono>

#include <glog/logging.h>

#include "Core/Images/Utils.h"
#include "Core/ServiceLocator.h"
#include "Core/TaskDispatcher.h"

// CImageView
CImageViewWindow::CImageViewWindow() {
    callback_ = nullptr;
    currentParent_ = nullptr;
}

CImageViewWindow::~CImageViewWindow() {
}

LRESULT CImageViewWindow::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    RECT rc = { 380, 37, 636, 240 };
    imageControl_.Create(m_hWnd, rc, 0, WS_CHILD | WS_VISIBLE);
    imageControl_.setHideParent(true);

    return TRUE;
}

LRESULT CImageViewWindow::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    resetView();
    return 0;
}

LRESULT CImageViewWindow::OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    hide();
    return 0;
}

LRESULT CImageViewWindow::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    if (wParam == IMAGE_LOAD_TIMER) {
        checkImageLoad();
    }
    return 0;
}

LRESULT CImageViewWindow::onClick(UINT flags, CPoint point) {
    hide();
    return 0;
}

bool CImageViewWindow::ViewImage(const CImageViewItem& item, HWND parent) {
    if (!SetTimer(IMAGE_LOAD_TIMER, USER_TIMER_MINIMUM)) {
        return false;
    }

    std::packaged_task<std::unique_ptr<GdiPlusImage>()> loadImage([fileName = item.fileName] {
        return ImageUtils::LoadImageFromFileExtended(fileName, ImageFrameMode::ALL_FRAMES);
    });
    imageLoad_ = std::make_shared<std::future<std::unique_ptr<GdiPlusImage>>>(loadImage.get_future());
    currentItem_ = item;
    currentParent_ = parent;
    requestForeground_ = ::GetForegroundWindow();
    loadingDeadline_ = ::GetTickCount64() + LOADING_DELAY_MS;
    ServiceLocator::instance()->taskDispatcher()->post(
        [loadImage = std::move(loadImage), request = std::weak_ptr(imageLoad_)]() mutable {
            // Skip queued loads that were superseded or dismissed.
            if (!request.expired()) {
                loadImage();
            }
        });
    return true;
}

void CImageViewWindow::checkImageLoad() {
    if (!imageLoad_) {
        return;
    }
    if (!IsWindowVisible() && ::GetForegroundWindow() != requestForeground_) {
        hide();
        return;
    }
    if (imageLoad_->wait_for(std::chrono::milliseconds(0)) != std::future_status::ready) {
        if (!progressRing_ && ::GetTickCount64() >= loadingDeadline_) {
            showLoadingIndicator();
        }
        return;
    }

    KillTimer(IMAGE_LOAD_TIMER);
    auto imageLoad = std::move(imageLoad_);
    std::unique_ptr<GdiPlusImage> image;
    try {
        image = imageLoad->get();
    }
    catch (const std::exception& ex) {
        LOG(WARNING) << "Could not load ImageView preview: " << ex.what();
    }

    if (image && image->getBitmap()) {
        showImage(std::move(image));
    } else if (hasPreview_) {
        hideLoadingIndicator();
        imageControl_.ShowWindow(SW_SHOW);
        imageControl_.setPause(false);
    } else {
        hide();
    }
}

void CImageViewWindow::showLoadingIndicator() {
    if (!IsWindowVisible()) {
        MyCenterWindow(currentParent_, MIN_PREVIEW_WIDTH, MIN_PREVIEW_HEIGHT);
    }

    constexpr int RING_SIZE = 36;
    CRect clientRect;
    GetClientRect(&clientRect);
    int left = (clientRect.Width() - RING_SIZE) / 2;
    int top = (clientRect.Height() - RING_SIZE) / 2;
    auto progressRing = std::make_unique<CProgressRingControl>();
    CStatic placeholder;
    if (!placeholder.Create(m_hWnd, CRect(left, top, left + RING_SIZE, top + RING_SIZE), nullptr,
                            WS_CHILD | WS_DISABLED)) {
        return;
    }
    if (!progressRing->SubclassWindow(placeholder)) {
        placeholder.DestroyWindow();
        return;
    }

    progressRing_ = std::move(progressRing);
    imageControl_.setPause(true);
    imageControl_.ShowWindow(SW_HIDE);
    progressRing_->ShowWindow(SW_SHOW);
    showPreviewWindow();
}

void CImageViewWindow::hideLoadingIndicator() {
    if (progressRing_) {
        progressRing_->DestroyWindow();
        progressRing_.reset();
    }
}

void CImageViewWindow::showPreviewWindow() {
    if (IsWindowVisible()) {
        return;
    }

    ShowWindow(SW_SHOW);
    SetForegroundWindow(m_hWnd);
    if (::GetActiveWindow() == m_hWnd) {
        SetFocus();
    }
}

void CImageViewWindow::showImage(std::unique_ptr<GdiPlusImage> image) {
    float width = static_cast<float>(GetSystemMetrics(SM_CXSCREEN) - 12);
    float height = static_cast<float>(GetSystemMetrics(SM_CYSCREEN) - 50);
    float imageWidth = static_cast<float>(image->getWidth());
    float imageHeight = static_cast<float>(image->getHeight());
    float previewWidth = imageWidth;
    float previewHeight = imageHeight;

    if (previewWidth > width || previewHeight > height) {
        if (imageWidth / imageHeight >= width / height) {
            previewWidth = width;
            previewHeight = width / imageWidth * imageHeight;
        } else {
            previewHeight = height;
            previewWidth = height / imageHeight * imageWidth;
        }
    }

    int windowWidth = std::max(MIN_PREVIEW_WIDTH, static_cast<int>(previewWidth + 2));
    int windowHeight = std::max(MIN_PREVIEW_HEIGHT, static_cast<int>(previewHeight + 2));
    hideLoadingIndicator();
    imageControl_.MoveWindow(0, 0, windowWidth, windowHeight);
    imageControl_.loadPreview(std::move(image));
    imageControl_.ShowWindow(SW_SHOW);
    hasPreview_ = true;
    MyCenterWindow(currentParent_, windowWidth, windowHeight);
    showPreviewWindow();
}

LRESULT CImageViewWindow::OnActivate(UINT state, BOOL fMinimized, HWND hwndActDeact) {
    if (state == WA_INACTIVE) {
        hide();
    }
    return 0;
}

void CImageViewWindow::MyCenterWindow(HWND hWndCenter, int width, int height) {
    ATLASSERT(::IsWindow(m_hWnd));

    // determine owner window to center against
    DWORD dwStyle = GetStyle();

    RECT rcArea;
    RECT rcCenter;
    HWND hWndParent;
    if (!(dwStyle & WS_CHILD)) {
        // don't center against invisible or minimized windows
        /*if (hWndCenter != NULL) {
            DWORD dwStyleCenter = ::GetWindowLong(hWndCenter, GWL_STYLE);
            if (!(dwStyleCenter & WS_VISIBLE) || (dwStyleCenter & WS_MINIMIZE))
                hWndCenter = NULL;
        }*/

        // center within screen coordinates
        HMONITOR hMonitor = NULL;
        if (hWndCenter != NULL) {
            hMonitor = ::MonitorFromWindow(hWndCenter, MONITOR_DEFAULTTONEAREST);
        } else {
            hMonitor = ::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
        }

        MONITORINFO minfo;
        minfo.cbSize = sizeof(MONITORINFO);
        ::GetMonitorInfo(hMonitor, &minfo);

        rcArea = minfo.rcWork;

        // if (hWndCenter == NULL)
        rcCenter = rcArea;
        /*else
            ::GetWindowRect(hWndCenter, &rcCenter);*/
    } else {
        // center within parent client coordinates
        hWndParent = ::GetParent(m_hWnd);
        ATLASSERT(::IsWindow(hWndParent));

        ::GetClientRect(hWndParent, &rcArea);
        ATLASSERT(::IsWindow(hWndCenter));
        ::GetClientRect(hWndCenter, &rcCenter);
        ::MapWindowPoints(hWndCenter, hWndParent, (POINT*)&rcCenter, 2);
    }

    int DlgWidth = width;
    int DlgHeight = height;

    // find dialog's upper left based on rcCenter
    int xLeft = (rcCenter.left + rcCenter.right) / 2 - DlgWidth / 2;
    int yTop = (rcCenter.top + rcCenter.bottom) / 2 - DlgHeight / 2;

    // if the dialog is outside the screen, move it inside
    if (xLeft + DlgWidth > rcArea.right)
        xLeft = rcArea.right - DlgWidth;
    if (xLeft < rcArea.left)
        xLeft = rcArea.left;

    if (yTop + DlgHeight > rcArea.bottom)
        yTop = rcArea.bottom - DlgHeight;
    if (yTop < rcArea.top)
        yTop = rcArea.top;

    // map screen coordinates to child coordinates
    SetWindowPos(NULL, xLeft, yTop, width, height, SWP_NOZORDER | SWP_NOACTIVATE);
}

LRESULT CImageViewWindow::OnKeyDown(TCHAR vk, UINT cRepeat, UINT flags) {
    switch (vk) {
    case VK_RIGHT:
        {
            if (callback_) {
                CImageViewItem item = callback_->getNextImgViewItem(currentItem_);
                if (!item.fileName.IsEmpty()) {
                    ViewImage(item, currentParent_);
                }
            }
        }
        break;
    case VK_LEFT:
        {
            if (callback_) {
                CImageViewItem item = callback_->getPrevImgViewItem(currentItem_);
                if (!item.fileName.IsEmpty()) {
                    ViewImage(item, currentParent_);
                }
            }
        }
        break;
    case VK_RETURN:
    case VK_ESCAPE:
        hide();
    }
    return 0;
}

void CImageViewWindow::setCallback(CImageViewCallback* callback) {
    callback_ = callback;
}

void CImageViewWindow::hide() {
    if (IsWindowVisible()) {
        ShowWindow(SW_HIDE);
    } else {
        resetView();
    }
}

void CImageViewWindow::onShowWindow(BOOL show, UINT status) {
    if (!show && status == 0) {
        resetView();
    }
}

void CImageViewWindow::resetView() {
    KillTimer(IMAGE_LOAD_TIMER);
    imageLoad_.reset();
    hideLoadingIndicator();
    imageControl_.reset();
    hasPreview_ = false;
}
