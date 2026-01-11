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

#include "ScreenRecordingDlg.h"

#include <dxgi1_6.h>

#include "Gui/GuiTools.h"
#include "Func/MyUtils.h"
#include "Core/Utils/StringUtils.h"
#include "ScreenCapture/MonitorEnumerator.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "ScreenCapture/ScreenCaptureWin.h"
#include "Gui/IconBitmapUtils.h"
#include "ScreenCapture/ScreenRecorder/DXGIOptionsManager.h"
#include "ScreenCapture/ScreenRecorder/FFMpegOptionsManager.h"
#include "RegionSelect.h"
#include "ImageEditor/Gui/ImageEditorWindow.h"
#include "Func/ImageEditorConfigurationProvider.h"
#include "ScreenCapture/WindowsHider.h"
#include "Gui/Helpers/DPIHelper.h"

using namespace ScreenCapture;

namespace {

std::vector<CString> GetMonitorsForAdapter(IDXGIAdapter1* pAdapter) {
    std::vector<CString> result;
    std::vector<CComPtr<IDXGIOutput>> outputs;

    CComPtr<IDXGIOutput> pOutput;
    for (UINT i = 0; pAdapter->EnumOutputs(i, &pOutput) != DXGI_ERROR_NOT_FOUND; ++i) {
        outputs.push_back(pOutput);
        pOutput.Release();
    }

    for (auto& output : outputs) {
        DXGI_OUTPUT_DESC desc;
        if (SUCCEEDED(output->GetDesc(&desc)) && desc.AttachedToDesktop) {
            if (desc.DesktopCoordinates.left == 0 && desc.DesktopCoordinates.top == 0) {
                result.insert(result.begin(), desc.DeviceName);
            } else {
                result.push_back(desc.DeviceName);
            }
        }
    }
    return result;
}

inline unsigned int HwndToUInt(HWND hwnd) {
    return static_cast<unsigned int>(reinterpret_cast<DWORD_PTR>(hwnd));
}

}

CScreenRecordingDlg::CScreenRecordingDlg(ScreenRecordingRuntimeParams params)
    : recordingParams_(std::move(params))
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    m_WhiteBr.CreateSolidBrush(RGB(255,255,255));
    iconBitmapUtils_ = std::make_unique<IconBitmapUtils>();
    if (settings->ScreenRecordingSettings.Backend == ScreenRecordingStruct::ScreenRecordingBackendDirectX) {
        dxgiOptionsManager_ = std::make_unique<DXGIOptionsManager>();
    } else if (settings->ScreenRecordingSettings.Backend == ScreenRecordingStruct::ScreenRecordingBackendFFmpeg) {
        ffmpegOptionsManager_ = std::make_unique<FFMpegOptionsManager>();
    } else {
        throw std::runtime_error("Invalid recording backend: " + settings->ScreenRecordingSettings.Backend);
    }
}

CScreenRecordingDlg::~CScreenRecordingDlg() {
    clearBitmaps();
}

void CScreenRecordingDlg::translateUI() {
    TRC(IDOK, "Start");
    TRC(IDCANCEL, "Cancel");
    TRC(IDC_DELAYLABEL, "Timeout:");
    TRC(IDC_SECLABEL, "sec");
    TRC(IDC_CAPTURECURSORCHECKBOX, "Capture cursor");
    TRC(IDC_AUDIOSOURCELABEL, "Audio source:");
    TRC(IDC_ADAPTERLABEL, "Monitor:");
}

LRESULT CScreenRecordingDlg::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    translateUI();
    DoDataExchange(FALSE);
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    CRect ClientRect;
    GetClientRect(&ClientRect);
    CenterWindow(GetParent());

    icon_.LoadIconMetric(IDI_ICONRECORD, LIM_LARGE);
    iconSmall_.LoadIconMetric(IDI_ICONRECORD, LIM_SMALL);

    SetIcon(icon_, TRUE);
    SetIcon(iconSmall_, FALSE);

    SetWindowText(TR("Screen Recoding"));
    std::wstring labelText = str(IuStringUtils::FormatWideNoExcept(TR("Screen recording backend: %s")) % IuCoreUtils::Utf8ToWstring(settings->ScreenRecordingSettings.Backend));
    SetDlgItemText(IDC_BACKENDLABEL, labelText.c_str());
    regionSelectButton_.SetButtonStyle(BS_SPLITBUTTON);

    delaySpin_.SetRange(0, 90);
    delaySpin_.SetPos(settings->ScreenRecordingSettings.Delay);

    GuiTools::SetCheck(m_hWnd, IDC_CAPTURECURSORCHECKBOX, settings->ScreenRecordingSettings.CaptureCursor);

    audioSourcesListView_.AddColumn(_T(""), 0);
    audioSourcesListView_.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
    DWORD listViewStyle = LVS_EX_DOUBLEBUFFER | LVS_EX_FULLROWSELECT;
    if (settings->ScreenRecordingSettings.Backend == ScreenRecordingStruct::ScreenRecordingBackendDirectX) {
        listViewStyle |= LVS_EX_CHECKBOXES;
    } else {
        audioSourcesListView_.ModifyStyle(0, LVS_SHOWSELALWAYS | LVS_SINGLESEL);
    }
    audioSourcesListView_.SetExtendedListViewStyle(listViewStyle, listViewStyle);

    if (dxgiOptionsManager_) {
        const auto& recordingSettings = settings->ScreenRecordingSettings.DXGISettings;

        audioSources_ = dxgiOptionsManager_->getAudioSources();

        int selectedItemIndex = -1;
        int i = 0;
        for (const auto& source : audioSources_) {
            const CString str = U2W(source.second);
            int index = audioSourcesListView_.AddItem(i++, 0, str);
            if (std::find(recordingSettings.AudioSources.begin(), recordingSettings.AudioSources.end(), source.first) != recordingSettings.AudioSources.end()) {
                audioSourcesListView_.SetCheckState(index, TRUE);
            }
        }
    } else {
        const auto& recordingSettings = settings->ScreenRecordingSettings.FFmpegSettings;

        audioSources_ = ffmpegOptionsManager_->getAudioSources();

        int selectedItemIndex = -1;
        int i = 0;
        for (const auto& source : audioSources_) {
            const CString str = U2W(source.second);
            int index = audioSourcesListView_.AddItem(i++, 0, str);
            if (source.first == recordingSettings.AudioSourceId) {
                selectedItemIndex = index;
            }
        }
        audioSourcesListView_.SelectItem(selectedItemIndex);
    }

    int selectedIndex = 0;

    int itemIndex = monitorCombobox_.AddString(TR("Current monitor"));

    if (itemIndex >= 0) {
        monitorCombobox_.SetItemData(itemIndex, static_cast<DWORD_PTR>(kCurrentMonitor));
        if (settings->ScreenRecordingSettings.MonitorMode == kCurrentMonitor) {
            selectedIndex = itemIndex;
        }
    }

    itemIndex = monitorCombobox_.AddString(TR("All monitors"));

    if (itemIndex >= 0) {
        monitorCombobox_.SetItemData(itemIndex, static_cast<DWORD_PTR>(kAllMonitors));
        if (settings->ScreenRecordingSettings.MonitorMode == kAllMonitors) {
            selectedIndex = itemIndex;
        }
    }

    int i = 0;

    if (monitorEnumerator_.enumDisplayMonitors(0, 0)) {
        for (const MonitorEnumerator::MonitorInfo& monitor : monitorEnumerator_) {
            CString itemTitle;
            itemTitle.Format(_T("(%dx%d) %s"), monitor.rect.Width(), monitor.rect.Height(), monitor.deviceName.GetString());

            itemIndex = monitorCombobox_.AddString(itemTitle);
            if (itemIndex >= 0) {
                monitorCombobox_.SetItemData(itemIndex, static_cast<DWORD_PTR>(kSelectedMonitor + i));
                if (settings->ScreenRecordingSettings.MonitorMode == kSelectedMonitor + i) {
                    selectedIndex = itemIndex;
                }
            }
            i++;
        }
    }

    // Monitor count less than 2
    if (i < 2) {
        //m_monitorCombobox.EnableWindow(FALSE);
        monitorCombobox_.SetCurSel(1);
    } else {
        monitorCombobox_.SetCurSel(selectedIndex);
    }

    updateRegionSelectButtonTitle();
    return 0;
}

LRESULT CScreenRecordingDlg::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    return EndDialog(wID);
}

LRESULT CScreenRecordingDlg::OnCtlColorMsgDlg(HDC hdc, HWND hwndChild)
{
    return reinterpret_cast<LRESULT>(static_cast<HBRUSH>(m_WhiteBr)); // Returning brush solid filled with COLOR_WINDOW color
}

LRESULT CScreenRecordingDlg::OnBnClickedRegionSelectButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    showRegionSelectButtonMenu(hWndCtl);
    return 0;
}

LRESULT CScreenRecordingDlg::OnBnDropdownRegionSelectButton(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
    showRegionSelectButtonMenu(GetDlgItem(idCtrl));
    return 0;
}

LRESULT CScreenRecordingDlg::OnClickedSelectRegion(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    using namespace ImageEditor;

    WindowsHider hider(m_hWnd);
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    ImageEditorConfigurationProvider configProvider;

    CScreenCaptureEngine engine;
    engine.captureScreen(false);
    engine.setMonitorMode(kAllMonitors);
    engine.setDelay(settings->ScreenshotSettings.WindowHidingDelay);

    std::shared_ptr<Gdiplus::Bitmap> res(engine.capturedBitmap());

    if (!res) {
        return 0;
    }
    ImageEditorWindow imageEditor(res, false, &configProvider, nullptr, true);
    imageEditor.setInitialDrawingTool(ImageEditor::DrawingToolType::dtCrop);

    auto dialogResult = imageEditor.DoModal(m_hWnd, nullptr, ImageEditorWindow::wdmFullscreen);

    if (dialogResult == ImageEditorWindow::drContinue) {
        CRect selectedRect = imageEditor.getSelectedRect();

        if (!selectedRect.IsRectEmpty()) {
            recordingParams_.selectedWindow = NULL;
            recordingParams_.selectedRegion = selectedRect;
            updateRegionSelectButtonTitle();
        }
    }

    return 0;
}

LRESULT CScreenRecordingDlg::OnClickedFullScreen(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    recordingParams_.selectedRegion.SetRectEmpty();
    recordingParams_.selectedWindow = NULL;
    updateRegionSelectButtonTitle();
    return 0;
}

LRESULT CScreenRecordingDlg::OnClickedWindow(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    int index = wID - IDM_WINDOW_LIST_FIRST;
    if (index < 0 || index >= hwnds_.size()) {
        return 0;
    }
    recordingParams_.selectedWindow = hwnds_[index];
    updateRegionSelectButtonTitle();
    return 0;
}

LRESULT CScreenRecordingDlg::OnClickedSelectWindow(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    WindowsHider hider(m_hWnd);
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    bool onlyTopWindows = true;
    CScreenCaptureEngine engine;
    engine.setDelay(settings->ScreenshotSettings.WindowHidingDelay);
    engine.setMonitorMode(kAllMonitors);
    engine.captureScreen(false);
    //const auto [monitorMode, monitor] = getSelectedMonitor();

    RegionSelect.setSelectionMode(SelectionMode::smWindowHandles, onlyTopWindows);
    std::shared_ptr<Gdiplus::Bitmap> res(engine.capturedBitmap());
    if (res) {
        HBITMAP gdiBitmap = 0;
        res->GetHBITMAP(Gdiplus::Color(255, 255, 255), &gdiBitmap);
        if (RegionSelect.Execute(gdiBitmap, res->GetWidth(), res->GetHeight(), NULL)) {
            auto rgn = RegionSelect.region();
            if (rgn) {
                auto* whr = dynamic_cast<CWindowHandlesRegion*>(rgn.get());
                if (whr) {
                    if (whr->size()) {
                        recordingParams_.selectedWindow = whr->cbegin()->wnd;
                    }

                    updateRegionSelectButtonTitle();
                }

                DeleteObject(gdiBitmap);
            }
        }
    }

    return 0;
}

ScreenRecordingRuntimeParams CScreenRecordingDlg::recordingParams() const {
    return recordingParams_;
}

LRESULT CScreenRecordingDlg::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    settings->ScreenRecordingSettings.CaptureCursor = GuiTools::GetCheck(m_hWnd, IDC_CAPTURECURSORCHECKBOX);
    settings->ScreenRecordingSettings.Delay = delaySpin_.GetPos();

    if (!recordingParams_.selectedWindow && recordingParams_.selectedRegion.IsRectEmpty()) {
        const auto [monitorMode, monitor] = getSelectedMonitor();
        recordingParams_.setMonitor(monitor);
        settings->ScreenRecordingSettings.MonitorMode = monitorMode;
    } else {
        recordingParams_.setMonitor(nullptr);
        settings->ScreenRecordingSettings.MonitorMode = kAllMonitors;
    }

    if (settings->ScreenRecordingSettings.Backend == ScreenRecordingStruct::ScreenRecordingBackendDirectX) {
        auto& recordingSettings = settings->ScreenRecordingSettings.DXGISettings;
        recordingSettings.AudioSources.clear();
        for (int i = 0; i < audioSources_.size(); i++) {
            if (audioSourcesListView_.GetCheckState(i)) {
                recordingSettings.AudioSources.push_back(audioSources_[i].first);
            }
        }
    } else if (settings->ScreenRecordingSettings.Backend == ScreenRecordingStruct::ScreenRecordingBackendFFmpeg) {
        auto& recordingSettings = settings->ScreenRecordingSettings.FFmpegSettings;
        const int selectedItemIndex = audioSourcesListView_.GetNextItem(-1, LVNI_SELECTED);
        if (selectedItemIndex >= 0 && selectedItemIndex < audioSources_.size()) {
            recordingSettings.AudioSourceId = audioSources_[selectedItemIndex].first;
        } else {
            recordingSettings.AudioSourceId.clear();
        }
    }

    return 0;
}

LRESULT CScreenRecordingDlg::OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled){
    GuiTools::DisableDwmAnimations(m_hWnd);
    EndDialog(IDOK);
    return 0;
}

void CScreenRecordingDlg::showRegionSelectButtonMenu(HWND hWndCtl) {
    const int dpi = DPIHelper::GetDpiForWindow(hWndCtl);
    RECT rc;
    ::GetWindowRect(hWndCtl, &rc);
    POINT menuOrigin = { rc.left, rc.bottom };
    CMenu windowsMenu;
    windowsMenu.CreatePopupMenu();
    hwnds_.clear();
    clearBitmaps();

    EnumWindows([](HWND wnd, LPARAM param) -> BOOL {
        auto* hwnds = reinterpret_cast<HwndVector*>(param);
        if (GuiTools::IsWindowVisibleOnScreen(wnd)) {
            hwnds->push_back(wnd);
        }

        return hwnds->size() < IDM_WINDOW_LIST_LAST - IDM_WINDOW_LIST_FIRST + 1;
    }, reinterpret_cast<LPARAM>(&hwnds_));

    int i = -1;
    HWND parent = GetParent();
    for (const auto& hwnd: hwnds_) {
        i++;
        if (hwnd == m_hWnd || hwnd == parent) {
            continue;
        }
        const CString windowTitle = WinUtils::TrimString(GuiTools::GetWindowText(hwnd), 120);
        HICON ico = GuiTools::GetWindowIcon(hwnd);
        CString itemTitle;
        itemTitle.Format(_T("[%u] %s"), HwndToUInt(hwnd), windowTitle.GetString());
        MENUITEMINFO mi;
        ZeroMemory(&mi, sizeof(mi));
        mi.cbSize = sizeof(mi);
        mi.fMask = MIIM_FTYPE | MIIM_ID | MIIM_STRING;
        mi.fType = MFT_STRING;
        mi.wID = IDM_WINDOW_LIST_FIRST + i;
        mi.dwTypeData = const_cast<LPWSTR>(itemTitle.GetString());
        mi.cch = itemTitle.GetLength();
        HBITMAP bm = iconBitmapUtils_->HIconToBitmapPARGB32(ico, dpi);
        if (bm) {
            mi.hbmpItem = bm;
            mi.fMask |= MIIM_BITMAP;
            bitmaps_.push_back(bm);
        }
        windowsMenu.InsertMenuItem(i, true, &mi);

        if (i > IDM_WINDOW_LIST_LAST) {
            break;
        }
    }

    CMenu popupMenu;
    popupMenu.CreatePopupMenu();

    popupMenu.AppendMenu(MF_STRING, IDM_FULL_SCREEN, TR("Full screen"));
    popupMenu.AppendMenu(MF_STRING, IDM_SELECT_REGION, TR("Select region..."));
    popupMenu.AppendMenu(MF_STRING, IDM_SELECT_WINDOW, TR("Select window..."));
    popupMenu.AppendMenu(0, windowsMenu.Detach(), TR("Window"));

    TPMPARAMS excludeArea;
    ZeroMemory(&excludeArea, sizeof(excludeArea));
    excludeArea.cbSize = sizeof(excludeArea);
    excludeArea.rcExclude = rc;
    popupMenu.TrackPopupMenuEx(TPM_LEFTALIGN | TPM_LEFTBUTTON, menuOrigin.x, menuOrigin.y, m_hWnd, &excludeArea);
}

void CScreenRecordingDlg::updateRegionSelectButtonTitle() {
    CString title;
    bool enableMonitorComboBox = false;
    if (recordingParams_.selectedWindow) {
        CString text = GuiTools::GetWindowText(recordingParams_.selectedWindow);
        if (text.IsEmpty()) {
            title.Format(_T("[%u]"), HwndToUInt(recordingParams_.selectedWindow));
        } else {
            title = WinUtils::TrimString(text, 45);
        }
    } else if (recordingParams_.selectedRegion.IsRectEmpty()) {
        title = TR("Full screen");
        enableMonitorComboBox = true;
    } else {
        title.Format(_T("(%d,%d) %d x %d"), recordingParams_.selectedRegion.left, recordingParams_.selectedRegion.top, recordingParams_.selectedRegion.Width(), recordingParams_.selectedRegion.Height());
    }
    regionSelectButton_.SetWindowText(title);
    if (!enableMonitorComboBox) {
        monitorCombobox_.SetCurSel(1); // 'All monitors'
    }
    monitorCombobox_.EnableWindow(enableMonitorComboBox);
}

std::pair<ScreenCapture::MonitorMode, HMONITOR> CScreenRecordingDlg::getSelectedMonitor() {
    const int itemIndex = monitorCombobox_.GetCurSel();
    if (itemIndex >= 0) {
        auto monitorMode = static_cast<MonitorMode>(monitorCombobox_.GetItemData(itemIndex));

        HMONITOR monitor {};
        int monitorIndex = -1;
        if (monitorMode == kCurrentMonitor) {
            return {
                monitorMode, MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTOPRIMARY)
            };
            //recordingParams_.setMonitor(monitor);
        } else if (monitorMode >= kSelectedMonitor) {
            //monitorIndex = std::max(0, monitorMode);
            auto info = monitorEnumerator_.getByIndex(monitorMode - kSelectedMonitor);
            if (info) {
                return {
                    monitorMode, info->monitor
                };
                //recordingParams_.setMonitor(monitor, monitorIndex);
            }
        }
        return { monitorMode , NULL};
    }
    return {};
}

void CScreenRecordingDlg::clearBitmaps(){
    for (const auto& bitmap : bitmaps_) {
        DeleteObject(bitmap);
    }
    bitmaps_.clear();
}
