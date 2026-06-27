#ifndef IU_GUI_DIALOGS_SCREENRECORDERWINDOW_H
#define IU_GUI_DIALOGS_SCREENRECORDERWINDOW_H

#pragma once

#include <memory>
#include <boost/signals2.hpp>
#include <boost/enable_shared_from_this.hpp>
#include "atlheaders.h"
#include "resource.h"
#include "ImageEditor/Gui/Toolbar.h"
#include "ScreenCapture/ScreenRecorder/ScreenRecorder.h"
#include "Gui/Components/trayicon.h"
#include "Gui/Dialogs/ScreenRecordingDlg.h"

class CHotkeyList;

class TimeDelegate : public ImageEditor::Toolbar::ToolbarItemDelegate {
public:
   // enum { kOffset = 7, kSquareSize = 16, kPadding = 3 };

    TimeDelegate(ImageEditor::Toolbar* toolbar, int itemIndex);
    SIZE CalcItemSize(ImageEditor::Toolbar::Item& item, int x, int y, float dpiScaleX, float dpiScaleY) override;
    void DrawItem(ImageEditor::Toolbar::Item& item, Gdiplus::Graphics* gr, int x, int y, float dpiScaleX, float dpiScaleY) override;
    void setText(CString text);
    bool needClick() override;

protected:
    int toolbarItemIndex_;
    ImageEditor::Toolbar* toolbar_;
    CString text_;
    std::unique_ptr<Gdiplus::Font> font_;
};

class ScreenRecorderWindow : public boost::enable_shared_from_this<ScreenRecorderWindow>,
                             public CWindowImpl<ScreenRecorderWindow>,
                             public CMessageFilter,
                             public CTrayIconImpl<ScreenRecorderWindow>
{
 public:
    DECLARE_WND_CLASS(_T("ScreenRecorderWindow"))

    ScreenRecorderWindow();

    enum
    {
        ID_STOP = 1000, ID_PAUSE
    };

    UINT taskBarCreatedMsg_;

    BEGIN_MSG_MAP(ScreenRecorderWindow)
        MESSAGE_HANDLER(WM_CREATE, onCreate)
        MESSAGE_HANDLER(WM_DESTROY, onDestroy)
        MESSAGE_HANDLER(WM_PAINT, onPaint)
        MESSAGE_HANDLER(WM_ERASEBKGND, onEraseBkgnd)
        MESSAGE_HANDLER(WM_TIMER, onTimer)
        MESSAGE_HANDLER(WM_TRAYICON, onTrayIcon)
        MESSAGE_HANDLER(WM_DPICHANGED, onDpiChanged)
        MESSAGE_HANDLER(taskBarCreatedMsg_, onTaskBarCreated)

        //MESSAGE_HANDLER(WM_NCHITTEST, onNcHitTest)
        COMMAND_ID_HANDLER(IDCANCEL, onCancel)
        COMMAND_ID_HANDLER(ID_STOP, onStop)
        COMMAND_ID_HANDLER(ID_PAUSE, onPause)
        MSG_WM_HOTKEY(OnHotKey)
        //REFLECT_NOTIFICATIONS()
    END_MSG_MAP()

    enum DialogResult
    {
        drCancel, drSuccess
    };

    inline static constexpr auto kTimer = 1, kStartTimer = 2;

    DialogResult doModal(HWND parent, const ScreenRecordingRuntimeParams& params, bool forceShowParentAfter = false);
    BOOL PreTranslateMessage(MSG* pMsg) override;
    CString outFileName() const;
    void stop();

private:

    // Handler prototypes (uncomment arguments if needed):
    //    LRESULT MessageHandler(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/)
    //    LRESULT CommandHandler(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
    //    LRESULT NotifyHandler(int /*idCtrl*/, LPNMHDR /*pnmh*/, BOOL& /*bHandled*/)


    LRESULT onCreate(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT onDestroy(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT onPaint(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT onEraseBkgnd(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT onTimer(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT onDpiChanged(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT onTaskBarCreated(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT onCancel(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT onStop(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT onPause(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
    LRESULT onNcHitTest(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT onTrayIcon(UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/, BOOL& /*bHandled*/);
    LRESULT OnHotKey(int HotKeyID, UINT flags, UINT vk);

    DialogResult dialogResult_;
    void endDialog(DialogResult dr);
    void createToolbar();
    void updateTimeLabel();
    void registerHotkeys();
    void unRegisterHotkeys();
    void updateWindowSize();
    void createTrayIcon();
    std::string prepareFileName(int width, int height);

    CIcon icon_, iconSmall_;
    CRect captureRect_;
    CString outFileName_;
    ImageEditor::Toolbar toolbar_;
    std::unique_ptr<TimeDelegate> timeDelegate_;
    std::shared_ptr<ScreenRecorder> screenRecorder_;
    void statusChangeCallback(ScreenRecorder::Status status);
    ScreenRecorder::Status previousStatus_ = ScreenRecorder::Status::Invalid;
    uint64_t elapsedTime_ = 0;
    bool cancelRequested_ = false;
    bool stopRequested_ = false;
    std::shared_ptr<Gdiplus::Bitmap> iconResume_, iconPause_;
    GUID trayIconGuid_{};
    bool hasStarted_ = false;
    ScreenRecordingRuntimeParams screenRecordingParams_;
    std::unique_ptr<CHotkeyList> hotkeys_;
    CRect frameRect_;
    bool fullScreen_ = false;
    int fileNameCounter_ = 1;
}; 

#endif
