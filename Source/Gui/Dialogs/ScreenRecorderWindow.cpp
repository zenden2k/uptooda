#include "ScreenRecorderWindow.h"

#include <ctime>
#include <filesystem>

#include "Core/Logging.h"
#include "Core/i18n/Translator.h"
#include "Gui/GuiTools.h"
#include "Core/Images/Utils.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "ImageEditor/Gui/ImageEditorWindow.h"
#include "Core/TaskDispatcher.h"
#include "ScreenCapture/ScreenRecorder/FFmpegScreenRecorder.h"
#include "ScreenCapture/ScreenRecorder/DXGIScreenRecorder.h"
#include "ScreenCapture/ScreenRecorder/Sources/GDIGrabSource.h"
#include "ScreenCapture/ScreenRecorder/Sources/DDAGrabSource.h"
#include "ScreenCapture/WindowsHider.h"
#include "ScreenCapture/ScreenshotHelper.h"
#include "Gui/Dialogs/HotkeyEditor.h"
#include "ScreenCapture/ScreenRecorder/ScreenRecorderUtils.h"
#include "Func/Common.h"
#include "Func/IuCommonFunctions.h"

constexpr auto PANEL_HEIGHT = 40;


namespace {

// {45CE8BB0-4973-4560-88DF-72A7CC407FF9}
//static const GUID ScreenRecorderBaseGUID = { 0x45ce8bb0, 0x4973, 0x4560, { 0x88, 0xdf, 0x72, 0xa7, 0xcc, 0x40, 0x7f, 0xf9 } };

constexpr COLORREF WINDOW_TRANSPARENT_COLOR = RGB(255, 0, 255); // Magenta
}

int ScreenRecorderWindow::fileNameCounter_ = 1;

ScreenRecorderWindow::ScreenRecorderWindow():
        dialogResult_(drCancel),
        toolbar_(ImageEditor::Toolbar::orHorizontal, false) {
    memset(&trayIconGuid_, 0, sizeof(GUID));
    hotkeys_ = std::make_unique<CHotkeyList>();
    taskBarCreatedMsg_ = RegisterWindowMessage(_T("TaskbarCreated"));
}

LRESULT ScreenRecorderWindow::onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    GuiTools::SetWindowPointer(m_hWnd, this);
    SetLayeredWindowAttributes(m_hWnd, WINDOW_TRANSPARENT_COLOR, 0, LWA_COLORKEY);

    CWindowDC hdc(nullptr);
    int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
    int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);

    CRect clientRect;
    GetClientRect(clientRect);

    int toolbarHeight = MulDiv(PANEL_HEIGHT, dpiX, USER_DEFAULT_SCREEN_DPI);

    frameRect_ = clientRect;

    if (toolbar_.Create(m_hWnd, false, true, WINDOW_TRANSPARENT_COLOR, false)) {
        toolbar_.setShowButtonText(true);

        auto loadToolbarIcon = [](int resource)
        {
            return std::shared_ptr<Gdiplus::Bitmap>(ImageUtils::BitmapFromResource(GetModuleHandle(0), MAKEINTRESOURCE(resource), _T("PNG")));
        };
        toolbar_.addButton(ImageEditor::Toolbar::Item(TR("Finish"), loadToolbarIcon(IDB_ICONOK), ID_STOP, TR("Finish")));
        iconPause_ = loadToolbarIcon(IDB_ICONPAUSE);
        iconResume_ = loadToolbarIcon(IDB_ICONPLAY);
        toolbar_.addButton(ImageEditor::Toolbar::Item(TR("Pause"), iconPause_, ID_PAUSE, TR("Pause")));

        int index = toolbar_.addButton(ImageEditor::Toolbar::Item(CString(TR("Cancel")), loadToolbarIcon(IDB_ICONCANCEL), IDCANCEL, TR("Cancel")));

        ImageEditor::Toolbar::Item timeLabel(CString(), loadToolbarIcon(IDB_ICONUNDOPNG), IDCANCEL, CString(), ImageEditor::Toolbar::itLabel, false);

        timeDelegate_ = std::make_unique<TimeDelegate>(&toolbar_, index + 1);
        timeLabel.itemDelegate = timeDelegate_.get();
        toolbar_.addButton(timeLabel);
        toolbar_.SetWindowPos(nullptr, clientRect.left, clientRect.bottom, 0, 0, SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
        toolbar_.AutoSize();

        if (!fullScreen_) {
            CRect toolbarRect;
            toolbar_.GetClientRect(toolbarRect);
            SetWindowPos(nullptr, 0, 0, std::max(toolbarRect.Width(), clientRect.Width()), toolbarRect.Height() + clientRect.Height(), SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        }
        toolbar_.ShowWindow(SW_SHOW);
    }
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();

    icon_.LoadIconMetric(IDI_ICONRECORD, LIM_LARGE);
    iconSmall_.LoadIconMetric(IDI_ICONRECORD, LIM_SMALL);

    SetIcon(icon_, TRUE);
    SetIcon(iconSmall_, FALSE);

    createTrayIcon();

     CRect rect = captureRect_;
     bool isPrimaryMonitor = true;
     MONITORINFO monitorInfo = {};
     monitorInfo.cbSize = sizeof(MONITORINFO);
     bool monitorInfoSuccess = GetMonitorInfo(screenRecordingParams_.monitor(), &monitorInfo);
     if (monitorInfoSuccess) {
         isPrimaryMonitor = monitorInfo.dwFlags & MONITORINFOF_PRIMARY;
     }

     if (screenRecordingParams_.monitor() && !(settings->ScreenRecordingSettings.Backend == ScreenRecordingStruct::ScreenRecordingBackendFFmpeg
         && settings->ScreenRecordingSettings.FFmpegSettings.VideoSourceId == GDIGrabSource::SOURCE_ID)
         && monitorInfoSuccess
         ) {
        rect.OffsetRect(-monitorInfo.rcMonitor.left, -monitorInfo.rcMonitor.top);
     }

    if (settings->ScreenRecordingSettings.Backend == ScreenRecordingStruct::ScreenRecordingBackendFFmpeg) {
        std::string ffmpegCLIPath = settings->ScreenRecordingSettings.FFmpegSettings.FFmpegCLIPath;
        if (ffmpegCLIPath.empty()) {
            ffmpegCLIPath = FFMpegOptionsManager::findFFmpegExecutable();
        }
        if (!ffmpegCLIPath.empty()) {
           
            FFmpegOptions options;
            const auto& ffmpegSettings = settings->ScreenRecordingSettings.FFmpegSettings;

            options.framerate = settings->ScreenRecordingSettings.FrameRate;

            options.source = ffmpegSettings.VideoSourceId;
            options.codec = ffmpegSettings.VideoCodecId;
            options.preset = ffmpegSettings.VideoPresetId;
            options.quality = ffmpegSettings.VideoQuality;
            options.bitrate = ffmpegSettings.VideoBitrate;
            options.useQuality = ffmpegSettings.UseQuality;
            options.audioSource = ffmpegSettings.AudioSourceId;
            options.audioCodec = ffmpegSettings.AudioCodecId;
            options.audioQuality = ffmpegSettings.AudioQuality;
            options.showCursor = settings->ScreenRecordingSettings.CaptureCursor;

            if (options.source == GDIGrabSource::SOURCE_ID && screenRecordingParams_.monitor() && rect.IsRectEmpty()) {
                if (monitorInfoSuccess) {
                    rect = monitorInfo.rcMonitor;
                }
            } 
           
            UINT adapterIndex = 0, outputIndex = 0;
            auto ad = GetAdapterForMonitor(screenRecordingParams_.monitor(), &adapterIndex, &outputIndex);
            if (ad) {
                if (!isPrimaryMonitor) {
                    options.hwAdapterIndex = adapterIndex;
                }
                options.outputIdx = outputIndex;
            }

            std::string fileName = prepareFileName(rect.right - rect.left, rect.bottom - rect.top);

            screenRecorder_ = std::make_shared<FFmpegScreenRecorder>(ffmpegCLIPath, fileName, rect, std::move(options));
        } else {
            GuiTools::LocalizedMessageBox(m_hWnd, TR("Could not find ffmpeg executable!"), TR("Error"), MB_ICONERROR);
        }
    } else if (settings->ScreenRecordingSettings.Backend == ScreenRecordingStruct::ScreenRecordingBackendDirectX) {  
            DXGIOptions options;
            const auto& dxgiSettings = settings->ScreenRecordingSettings.DXGISettings;

            options.framerate = settings->ScreenRecordingSettings.FrameRate;
            options.codec = dxgiSettings.VideoCodecId;
            options.preset = dxgiSettings.VideoPresetId;
            options.quality = dxgiSettings.VideoQuality;
            options.bitrate = dxgiSettings.VideoBitrate;
            options.useQuality = dxgiSettings.UseQuality;
            options.audioSources = dxgiSettings.AudioSources;
            options.audioCodec = dxgiSettings.AudioCodecId;
            options.audioBitrate = dxgiSettings.AudioBitrate;
            options.showCursor = settings->ScreenRecordingSettings.CaptureCursor;

            std::string fileName = prepareFileName(rect.right - rect.left, rect.bottom - rect.top);
            screenRecorder_ = std::make_shared<DXGIScreenRecorder>(fileName, /*screenRecordingParams_.selectedWindow*/nullptr, rect, screenRecordingParams_.monitor(), std::move(options));
    }
    if (!screenRecorder_) {
        return -1;
    }

    screenRecorder_->addStatusChangeCallback(
        ScreenRecorder::StatusChangeSignal::slot_type(
            [this](auto && PH1) { statusChangeCallback(std::forward<decltype(PH1)>(PH1)); }
        )
        .track(shared_from_this())
    );
    timeDelegate_->setText(TR("Waiting..."));
    registerHotkeys();
    int windowHidingDelay = 0;
    if (::IsWindowVisible(GetParent())) {
        windowHidingDelay = settings->ScreenshotSettings.WindowHidingDelay;
    }
    SetTimer(kStartTimer, std::max(windowHidingDelay, settings->ScreenRecordingSettings.Delay * 1000));
    return 0;
}


std::string ScreenRecorderWindow::prepareFileName(int width, int height) {
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    namespace fs = std::filesystem;
    fs::path basePath = settings->ScreenRecordingSettings.OutDirectory.empty()
        ? W2U(WinUtils::GetSystemSpecialPath(CSIDL_MYVIDEO))
        : settings->ScreenRecordingSettings.OutDirectory;

    fs::path relPath = W2U(IuCommonFunctions::GenerateFileName(U2WC(settings->ScreenRecordingSettings.FileNameTemplate),
                                                               fileNameCounter_++, CPoint(width, height), time(nullptr),
                                                               TR("Screen Recording")));

    fs::path fullPath = basePath / relPath;
    fs::path parentPath = fullPath.parent_path();
    bool folderExists = fs::exists(parentPath);
    if (folderExists && !fs::is_directory(parentPath)) {
        LOG(ERROR) << parentPath << " is not a folder";
        return {};
    }

    if (!folderExists &&  !fs::create_directories(fullPath)){
        std::string msg = str(IuStringUtils::FormatNoExcept(_("Failed to create the directory '%s'.")) % parentPath.string());
        GuiTools::LocalizedMessageBox(m_hWnd, U2WC(msg), TR("Error"), MB_ICONERROR);
        return {};
    }

    return fullPath.string();
}

LRESULT ScreenRecorderWindow::onDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    GuiTools::ClearWindowPointer(m_hWnd);
    unregisterHotkeys();
    return 0;
}

ScreenRecorderWindow::DialogResult ScreenRecorderWindow::doModal(HWND parent, const ScreenRecordingRuntimeParams& params, bool forceShowParentAfter) {
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();

    CWindowDC hdc(nullptr);
    int dpiX = GetDeviceCaps(hdc, LOGPIXELSX);
    int dpiY = GetDeviceCaps(hdc, LOGPIXELSY);
    screenRecordingParams_ = params;
    CRect captureRect = params.selectedRegion;

    if (/* screenRecordingParams_.selectedRegion.IsRectEmpty()&& */screenRecordingParams_.selectedWindow) {
        ScreenshotHelper::getActualWindowRect(screenRecordingParams_.selectedWindow, &captureRect);
        captureRect.right = captureRect.left + ((captureRect.Width() + 1) & ~1); // Rounding an integer up to the nearest even number
        captureRect.bottom = captureRect.top + ((captureRect.Height() + 1) & ~1);
    }

    bool isGdiGrab = settings->ScreenRecordingSettings.Backend == ScreenRecordingStruct::ScreenRecordingBackendFFmpeg
        && settings->ScreenRecordingSettings.FFmpegSettings.VideoSourceId == GDIGrabSource::SOURCE_ID;

    if (!screenRecordingParams_.monitor() && !isGdiGrab) {
        HMONITOR monitor = MonitorFromRect(captureRect, MONITOR_DEFAULTTONULL);
        screenRecordingParams_.setMonitor(monitor);
        if (monitor) {
            MONITORINFO info;
            memset(&info, 0, sizeof(info));
            info.cbSize = sizeof(MONITORINFO);
            if (GetMonitorInfo(screenRecordingParams_.monitor(), &info)) {
                captureRect.IntersectRect(captureRect, &info.rcMonitor);
                // captureRect.OffsetRect(-info.rcMonitor.left, -info.rcMonitor.top);
            }
        }
    }

    if (!screenRecordingParams_.monitor() && isGdiGrab) {
        CRect screenBounds;
        GuiTools::GetScreenBounds(screenBounds);
        captureRect.IntersectRect(captureRect, screenBounds);
    }

    captureRect.right = captureRect.left + (captureRect.Width() & ~1);
    captureRect.bottom = captureRect.top + (captureRect.Height() & ~1);

    captureRect_ = captureRect;

    DWORD initialWindowStyle = WS_POPUP | WS_CLIPCHILDREN;
    CRect windowRect = captureRect;
    if (!windowRect.IsRectEmpty()) {
        windowRect.InflateRect(1, 1, 1, 1);
        //windowRect.bottom += MulDiv(PANEL_HEIGHT, dpiX, USER_DEFAULT_SCREEN_DPI);
    } else {
        fullScreen_ = true;
    }
    CRgn exclude;

    // Parent window should be null here!
    if (Create(nullptr, windowRect, _T("Screen Recorder"), initialWindowStyle, WS_EX_TOPMOST | WS_EX_TOOLWINDOW | WS_EX_NOACTIVATE | WS_EX_LAYERED) == NULL) {
        LOG(ERROR) << "Screen Recorder window creation failed!\n";
        return drCancel;
    }
    
    ShowWindow(SW_SHOW);
    SetForegroundWindow(m_hWnd);
    //BringWindowToTop();
    //m_view.Invalidate(false);
    WindowsHider hider(parent);

    if (parent) {
        ::EnableWindow(parent, false);
    }

    CMessageLoop loop;
    loop.AddMessageFilter(this);
    loop.Run();

    if (parent) {
        ::EnableWindow(parent, true);
    }
    hider.show();
    if (forceShowParentAfter) {
        ::ShowWindow(parent, SW_SHOWNORMAL);
    }

    ShowWindow(SW_HIDE);
    if (parent) {
        ::SetForegroundWindow(parent);
        ::SetActiveWindow(parent); 
        ::SetFocus(parent); 
    }

    DestroyWindow();
    loop.RemoveMessageFilter(this);

    return dialogResult_;
}

void ScreenRecorderWindow::endDialog(DialogResult dr) {
    dialogResult_ = dr;
    PostQuitMessage(0);
}

BOOL ScreenRecorderWindow::PreTranslateMessage(MSG* pMsg) {
    /*if (TranslateAccelerator(m_hWnd, accelerators_, pMsg)) {
        return TRUE;
    }*/
    return FALSE;
}

LRESULT ScreenRecorderWindow::onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    CRect clientRect;
    GetClientRect(clientRect);
    CPaintDC dc(m_hWnd);
    int dpiX = dc.GetDeviceCaps(LOGPIXELSX);
    int dpiY = dc.GetDeviceCaps(LOGPIXELSY);
    CBrush br;
    br.CreateSolidBrush(WINDOW_TRANSPARENT_COLOR);

    dc.FillRect(clientRect, br);
    int panelHeight = MulDiv(PANEL_HEIGHT, dpiX, USER_DEFAULT_SCREEN_DPI);
    CPen pen;
    pen.CreatePen(PS_DASH, 1, RGB(0, 255, 0));
    HPEN oldPen = dc.SelectPen(pen);
    HBRUSH oldBr = dc.SelectBrush(br);
    dc.SetBkMode(TRANSPARENT);
    dc.Rectangle(frameRect_.left, frameRect_.top, frameRect_.right, frameRect_.bottom);
    dc.SelectBrush(oldBr);
    dc.SelectPen(oldPen);
    return 0;
}

LRESULT ScreenRecorderWindow::onEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
    bHandled = true;
    return 1;
}

LRESULT ScreenRecorderWindow::onCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    if (!hasStarted_ || screenRecorder_->status() == ScreenRecorder::Status::Canceled 
        || screenRecorder_->status() == ScreenRecorder::Status::Invalid
        || screenRecorder_->status() == ScreenRecorder::Status::Failed
        || screenRecorder_->status() == ScreenRecorder::Status::Finished
        //|| !screenRecorder_->isRunning() 
    ) {
        endDialog(drCancel);
    } else {
        cancelRequested_ = true;
        toolbar_.enableButton(toolbar_.getItemIndexByCommand(IDCANCEL), false);
        screenRecorder_->cancel();
    }

    return 0;
}

LRESULT ScreenRecorderWindow::onTimer(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    switch (wParam) {
        case kTimer:
            elapsedTime_++;
            updateTimeLabel();
            break;
        case kStartTimer:
            KillTimer(kStartTimer);
            hasStarted_ = true;
            screenRecorder_->start();
            updateTimeLabel();
            break;
    }

    return 0;
}

LRESULT ScreenRecorderWindow::onDpiChanged(UINT /*uMsg*/, WPARAM wParam, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    if (!fullScreen_) {
        //toolbar_.SendMessage(WM_MY_DPICHANGED, wParam);
        int dpiX = LOWORD(wParam);
        int dpiY = HIWORD(wParam);
        toolbar_.setDPI(dpiX, dpiY);
        toolbar_.AutoSize();
        updateWindowSize();
    }
    return 0;
}

LRESULT ScreenRecorderWindow::onTaskBarCreated(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/) {
    createTrayIcon();
    return 0;
}

LRESULT ScreenRecorderWindow::onPause(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    CRect clientRect;
    GetClientRect(&clientRect);
    ClientToScreen(&clientRect);

    screenRecorder_->setOffset(clientRect.left + 1, clientRect.top + 1);

    if (!hasStarted_ || stopRequested_ || cancelRequested_) {
        return 0;
    }

    if (screenRecorder_->status() == ScreenRecorder::Status::Recording) {
        //LOG(ERROR) << "screenRecorder_->pause()";
        screenRecorder_->pause();
    } else if (screenRecorder_->status() == ScreenRecorder::Status::Paused) {
        //LOG(ERROR) << "screenRecorder_->start()";
        screenRecorder_->start();
    }

    return 0;
}

void ScreenRecorderWindow::updateTimeLabel() {
    CString timeLabel;
    timeLabel.Format(_T("%02d:%02d:%02d.%d"), (int)(elapsedTime_ / 36000), (int)(elapsedTime_ / 600 % 60), (int)elapsedTime_ / 10 % 60, (int)elapsedTime_ % 10);
    timeDelegate_->setText(timeLabel);
}

LRESULT ScreenRecorderWindow::onStop(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    stop();
    return 0;
}

LRESULT ScreenRecorderWindow::onNcHitTest(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& bHandled) {
    bHandled = true;
    return HTCAPTION;
}


LRESULT ScreenRecorderWindow::onTrayIcon(UINT /*uMsg*/, WPARAM wParam, LPARAM lParam, BOOL& /*bHandled*/) {
    int msg = LOWORD(lParam);
    if (msg == WM_CONTEXTMENU) {
        CPoint pos(GET_X_LPARAM(wParam), GET_Y_LPARAM(wParam)); 
        
        CMenu trayIconMenu;
        trayIconMenu.CreatePopupMenu();
        trayIconMenu.AppendMenu(MF_STRING, ID_STOP, HotkeyToString("screenrecording_stop", TR("Finish")));
        trayIconMenu.EnableMenuItem(ID_STOP, MF_BYCOMMAND | (hasStarted_ ? MF_ENABLED : MF_DISABLED));

        CString msg;
        bool enablePauseItem = true;
        msg = TR("Pause");
        switch (screenRecorder_->status()) {
        case ScreenRecorder::Status::Recording:
            break;
        case ScreenRecorder::Status::Paused:
            msg = TR("Resume");
            break;
        default:
            enablePauseItem = false;
        }

        trayIconMenu.AppendMenu(MF_STRING, ID_PAUSE, HotkeyToString("screenrecording_pause", msg));
        trayIconMenu.EnableMenuItem(ID_PAUSE, MF_BYCOMMAND | ( enablePauseItem ? MF_ENABLED : MF_DISABLED));
        trayIconMenu.AppendMenu(MF_STRING, IDCANCEL, HotkeyToString("screenrecording_cancel", TR("Cancel")));
        trayIconMenu.SetMenuDefaultItem(ID_STOP);

        // Ensure the window is foreground to handle menu messages correctly
        // and allow the menu to close properly 
        ::SetForegroundWindow(m_hWnd);
        trayIconMenu.TrackPopupMenu( TPM_LEFTALIGN | TPM_RIGHTBUTTON, pos.x, pos.y, m_hWnd);
        PostMessage(WM_NULL);
    } else if (msg == WM_LBUTTONDBLCLK || msg == WM_LBUTTONUP) {
        SendMessage(WM_COMMAND, MAKEWPARAM(ID_STOP, 0));
    }
    return 0;
}

void ScreenRecorderWindow::statusChangeCallback(ScreenRecorder::Status status) {
    auto self = shared_from_this();
    ServiceLocator::instance()->taskRunner()->runInGuiThread([this, self, wnd = this->m_hWnd, status] {
        if (!GuiTools::CheckWindowPointer(wnd, this)) {
            return;
        }
        if (status == ScreenRecorder::Status::Recording) {
            SetTimer(kTimer, 100);
        } else {
            KillTimer(kTimer);
        }

        toolbar_.setMoveParent(status == ScreenRecorder::Status::Paused);

        if (status == ScreenRecorder::Status::Finished) {
            outFileName_ = U2W(screenRecorder_->outFileName());
            endDialog(drSuccess);
        } else if (
            status == ScreenRecorder::Status::Canceled
            || status == ScreenRecorder::Status::Failed
            || status == ScreenRecorder::Status::Paused
        ) {
            bool failed = status == ScreenRecorder::Status::Failed;
            if (failed) {
                GuiTools::LocalizedMessageBox(wnd, TR("An error occurred during screen recording. For details, please check the error log."), TR("Error"), MB_ICONERROR);
            }
            if (cancelRequested_ || failed) {
                endDialog(drCancel);
                return;
            }
        }

        int pauseItemIndex = toolbar_.getItemIndexByCommand(ID_PAUSE);
        auto item = toolbar_.getItem(pauseItemIndex);
        CString newTitle = status == ScreenRecorder::Status::Paused ? TR("Resume") : TR("Pause");
        if (item->title != newTitle) {
            item->title = newTitle;
            item->hint = newTitle;
            item->icon = status == ScreenRecorder::Status::Paused ? iconResume_ : iconPause_;
            toolbar_.AutoSize();
            CRect toolbarRect;
            toolbar_.GetClientRect(toolbarRect);

            SetWindowPos(nullptr, 0, 0, std::max(toolbarRect.Width(), frameRect_.Width()), toolbarRect.Height() + frameRect_.Height(), SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
            toolbar_.Invalidate();
        }

        if (status == ScreenRecorder::Status::RunningConversion) {
            timeDelegate_->setText(TR("Converting..."));
        } else if (status == ScreenRecorder::Status::RunningConcatenation) {
            timeDelegate_->setText(TR("Concatenation..."));
        }

    }, true);
    previousStatus_ = status;
}

CString ScreenRecorderWindow::outFileName() const {
    return outFileName_;
}

void ScreenRecorderWindow::stop() {
    if (!hasStarted_ || stopRequested_ || cancelRequested_) {
        return;
    }
    stopRequested_ = true;

    toolbar_.enableButton(toolbar_.getItemIndexByCommand(ID_STOP), false);

    screenRecorder_->stop();
}

void ScreenRecorderWindow::registerHotkeys() {
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    *hotkeys_ = settings->Hotkeys;

    for (size_t i = 0; i < hotkeys_->size(); i++) {
        const auto& hotkey = (*hotkeys_)[i];
        if (hotkey.groupId == HOTKEY_GROUP_SCREEN_RECORDER_WINDOW && hotkey.globalKey.keyCode) {
            if (!RegisterHotKey(m_hWnd, i, hotkey.globalKey.keyModifier, hotkey.globalKey.keyCode)) {
                const std::wstring msg = str(IuStringUtils::FormatWideNoExcept(
                    TR("Cannot register global hotkey:\n%s.\n Maybe it is being used by another process."))
                    % hotkey.globalKey.toString().GetString()
                );

                ServiceLocator::instance()->logger()->write(ILogger::logWarning, _T("Screen Recorder Hotkeys"), msg.c_str());
            }
        }
    }
}

void ScreenRecorderWindow::unregisterHotkeys() {
    for (size_t i = 0; i < hotkeys_->size(); i++) {
        const auto& hotkey = (*hotkeys_)[i];
        if (hotkey.groupId == HOTKEY_GROUP_SCREEN_RECORDER_WINDOW && hotkey.globalKey.keyCode) {
            UnregisterHotKey(m_hWnd, i);
        }
    }
    hotkeys_->clear();
}

void ScreenRecorderWindow::updateWindowSize() {
    if (!fullScreen_) {
        CRect toolbarRect;
        toolbar_.GetClientRect(toolbarRect);

        SetWindowPos(nullptr, 0, 0, std::max(toolbarRect.Width(), frameRect_.Width()), toolbarRect.Height() + frameRect_.Height(), SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        toolbar_.Invalidate();
    }
}

void ScreenRecorderWindow::createTrayIcon() {
    GUID* guid = nullptr;

    // Use GUID for uniquely identifying the system tray icon.
    // Generating fake GUID ensures that the icon is consistently identified across sessions, even if the app is restarted from a different folder.
    // Note: On Windows 7, using the same GUID but from a different path (folder) might fail to add the icon due to strict path checks.
    // On Windows 10/11, GUID is considered the unique identifier, and the icon can be added even from a different path.
    // trayIconGuid_ = WinUtils::GenerateFakeUUIDv4();
    // guid = &trayIconGuid_;


    std::wstring title = str(IuStringUtils::FormatWideNoExcept(TR("%s (screen recording)")) % APP_NAME);

    if (!InstallIcon(title.c_str(), iconSmall_, NULL, guid)) {
        LOG(WARNING) << "Failed to create tray icon!";
        /*guid = nullptr;
        if (!InstallIcon(iconTitle, icon_, NULL)) {
            LOG(WARNING) << "Failed to create tray icon again. I give up.";
        }*/
    }

    NOTIFYICONDATA nid;
    ZeroMemory(&nid, sizeof(nid));
    nid.cbSize = sizeof(NOTIFYICONDATA);
    nid.hWnd = m_hWnd;
    nid.uVersion = NOTIFYICON_VERSION_4;
    if (guid) {
        nid.uFlags = NIF_GUID;
        nid.guidItem = trayIconGuid_;
    }
    Shell_NotifyIcon(NIM_SETVERSION, &nid);
}

LRESULT ScreenRecorderWindow::onHotKey(int hotKeyID, UINT flags, UINT vk) {
    if (hotKeyID < 0 || hotKeyID >= hotkeys_->size()) {
        return 0;
    }

    const auto& hotkey = (*hotkeys_)[hotKeyID];
    if (hotkey.setForegroundWindow) {
        SetActiveWindow();
        SetForegroundWindow(m_hWnd);
    }
        
    SendMessage(WM_COMMAND, MAKEWPARAM(hotkey.commandId, 0));
    return 0;
}

TimeDelegate::TimeDelegate(ImageEditor::Toolbar* toolbar, int itemIndex):  toolbarItemIndex_(itemIndex),
                                                                           toolbar_(toolbar)
{
    CFont systemFont = GuiTools::GetSystemDialogFont();
    CWindowDC dc(toolbar->m_hWnd);
    font_ = std::make_unique<Gdiplus::Font>(dc, systemFont);
}

void TimeDelegate::setText(CString text) {
    text_ = text;
    if (toolbar_->m_hWnd) {
        toolbar_->repaintItem(toolbarItemIndex_);
    }
}

bool TimeDelegate::needClick() {
    return false;
}

SIZE TimeDelegate::CalcItemSize(ImageEditor::Toolbar::Item& item, int x, int y, float dpiScaleX, float dpiScaleY) {
    SIZE sz = { static_cast<LONG>(120 * dpiScaleX), static_cast<LONG>(25 * dpiScaleY) };
    return sz;
}

void TimeDelegate::DrawItem(ImageEditor::Toolbar::Item& item, Gdiplus::Graphics* gr, int x, int y, float dpiScaleX, float dpiScaleY) {
    using namespace Gdiplus;
    SolidBrush brush(Gdiplus::Color(0, 0, 0));
    SIZE sz = CalcItemSize(item, x, y, dpiScaleX, dpiScaleY);
    StringFormat format;
    format.SetLineAlignment(StringAlignmentCenter);
    format.SetAlignment(StringAlignmentCenter);
    gr->DrawString(text_, text_.GetLength(), font_.get(), RectF(x, y, sz.cx, sz.cy), &format, &brush);
}
