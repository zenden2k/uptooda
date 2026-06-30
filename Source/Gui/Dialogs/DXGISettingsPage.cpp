#include "DXGISettingsPage.h"

#include "Core/CommonDefs.h"
#include "WizardDlg.h"
#include "Gui/GuiTools.h"
#include "Func/WinUtils.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "ScreenCapture/ScreenRecorder/DXGIOptionsManager.h"
#include "Gui/Components/MyFileDialog.h"

CDXGISettingsPage::CDXGISettingsPage() {
    settings_ = ServiceLocator::instance()->settings<WtlGuiSettings>();
    optionsManager_ = std::make_unique<DXGIOptionsManager>();
}

void CDXGISettingsPage::TranslateUI() {
    TRC(IDC_VIDEOCODECLABEL, "Video codec:");
    TRC(IDC_VIDEOQUALITYRADIO, "Quality:");
    TRC(IDC_LOWQUALITYLABEL, "low quality (0)");
    TRC(IDC_HIGHQUALITYLABEL, "high quality (100)");
    TRC(IDC_VIDEOBITRATELABEL, "Bitrate:");
    TRC(IDC_VIDEOCODECPRESETLABEL, "Preset:");
    TRC(IDC_AUDIOCODECLABEL, "Audio codec:");
    TRC(IDC_AUDIOQUALITYLABEL, "Quality:");
    TRC(IDC_AUDIOSOURCELABEL, "Audio source:");
    TRC(IDC_AUDIOBITRATELABEL, "Bitrate:");
}
    
void CDXGISettingsPage::updateVideoQualityLabel() {
    int pos = videoQualityTrackBar_.GetPos();
    std::wstring text = str(IuStringUtils::FormatWideNoExcept(TR("%d%%")) % pos);
    videoQualityPercentLabel_.SetWindowText(text.c_str());
}

void CDXGISettingsPage::videoRadioChanged() {
    BOOL videoQualitySelected = videoQualityRadio_.GetCheck() == BST_CHECKED;
    BOOL codecUsesQuality = codecInfo_ ? codecInfo_->CanUseQuality : TRUE;
    BOOL codecUsesBitrate = codecInfo_ ? codecInfo_->CanUseBitrate : TRUE;

    BOOL videoQualityEnabled = videoQualitySelected && codecUsesQuality;
    videoQualityTrackBar_.EnableWindow(videoQualityEnabled);
    videoQualityPercentLabel_.EnableWindow(videoQualityEnabled);
    lowQualityLabel_.EnableWindow(videoQualityEnabled);
    highQualityLabel_.EnableWindow(videoQualityEnabled);

    BOOL videoBitrateEnabled = codecUsesBitrate && !videoQualitySelected;
    videoBitrateEditControl_.EnableWindow(videoBitrateEnabled);
    videoBitrateUpDownControl_.EnableWindow(videoBitrateEnabled);
    videoBitrateUnitsLabel_.EnableWindow(videoBitrateEnabled);
}

void CDXGISettingsPage::videoCodecChanged(const std::string& currentPresetId) {
    int index = videoCodecComboBox_.GetCurSel();
    if (index < 0 || index >= videoCodecs_.size()) {
        return;
    }

    std::string codecId = videoCodecs_[index].first;
    codecInfo_ = optionsManager_->getVideoCodecInfo(codecId);

    fillVideoCodecPresets(codecId, currentPresetId);

    if (codecInfo_) {
        videoQualityRadio_.EnableWindow(codecInfo_->CanUseQuality);
        videoBitrateRadio_.EnableWindow(codecInfo_->CanUseBitrate);

        if (!codecInfo_->CanUseBitrate) {
            videoBitrateRadio_.SetCheck(BST_UNCHECKED);
            videoQualityRadio_.SetCheck(BST_CHECKED);
            videoRadioChanged();
        } else if (!codecInfo_->CanUseQuality) {
            videoQualityRadio_.SetCheck(BST_UNCHECKED);
            videoBitrateRadio_.SetCheck(BST_CHECKED);

            videoRadioChanged();
        }
    }
}

void CDXGISettingsPage::audioCodecChanged(int bitrate, const std::string& currentPresetId /*= {}*/) {
    int index = audioCodecComboBox_.GetCurSel();
    if (index < 0 || index >= audioCodecs_.size()) {
        return;
    }

    const std::string codecId = audioCodecs_[index].first;
    audioCodecInfo_ = optionsManager_->getAudioCodecInfo(codecId);

    //fillAudioCodecQualities(codecId, quality);
}

void CDXGISettingsPage::fillVideoCodecPresets(const std::string& codecId, const std::string& currentPresetId) {
    videoCodecPresetComboBox_.ResetContent();
    if (codecInfo_) {
        const auto& videoCodecPresets = codecInfo_->Presets;
        const std::string presetId = currentPresetId.empty() ? codecInfo_->DefaultPresetId : currentPresetId;
       
        int selectedIndex = -1;
        for (const auto& preset : videoCodecPresets) {
            int index = videoCodecPresetComboBox_.AddString(U2WC(preset.second));
            if (index >= 0 && presetId == preset.first) {
                selectedIndex = index;
            }
        }
        videoCodecPresetComboBox_.SetCurSel(selectedIndex);
    }
}

LRESULT CDXGISettingsPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    TranslateUI();
    DoDataExchange(FALSE);
    const auto& recordingSettings = settings_->ScreenRecordingSettings.DXGISettings;
    audioSourcesListView_.AddColumn(_T(""), 0);
    audioSourcesListView_.SetColumnWidth(0, LVSCW_AUTOSIZE_USEHEADER);
    audioSourcesListView_.SetExtendedListViewStyle(LVS_EX_DOUBLEBUFFER | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT, LVS_EX_DOUBLEBUFFER | LVS_EX_CHECKBOXES | LVS_EX_FULLROWSELECT);
    audioSources_ = optionsManager_->getAudioSources();

    int selectedItemIndex = -1;
    int i = 0;
    for (const auto& source : audioSources_) {
        CString str = U2W(source.second);
        int index = audioSourcesListView_.AddItem(i++, 0, str);
        if (std::find(recordingSettings.AudioSources.begin(), recordingSettings.AudioSources.end(), source.first) != recordingSettings.AudioSources.end()) {
            audioSourcesListView_.SetCheckState(index, TRUE);

        }
    }

    videoQualityTrackBar_.SetPos(recordingSettings.VideoQuality);
    updateVideoQualityLabel();

    videoBitrateUpDownControl_.SetBase(100);
    videoBitrateUpDownControl_.SetRange32(100, 100000);
    UDACCEL accel { 0, 1 }; 
    accel.nInc = 100; 
    videoBitrateUpDownControl_.SetAccel(1, &accel);
    videoBitrateUpDownControl_.SetPos32(recordingSettings.VideoBitrate);

    videoQualityRadio_.SetCheck(recordingSettings.UseQuality ? BST_CHECKED : BST_UNCHECKED);
    videoBitrateRadio_.SetCheck(recordingSettings.UseQuality ? BST_UNCHECKED : BST_CHECKED);

    videoRadioChanged();

    videoCodecs_ = optionsManager_->getVideoCodecs();
    selectedItemIndex = -1;
    for (const auto& codec : videoCodecs_) {
        int index = videoCodecComboBox_.AddString(U2WC(codec.second));
        if (codec.first == recordingSettings.VideoCodecId) {
            selectedItemIndex = index;
        }
    }

    videoCodecComboBox_.SetCurSel(selectedItemIndex);
    videoCodecChanged(recordingSettings.VideoPresetId);

    // Audio
    audioCodecs_ = optionsManager_->getAudioCodecs();
    audioCodecs_.insert(audioCodecs_.begin(), { "", tr("None") });

    selectedItemIndex = -1;
    for (const auto& codec : audioCodecs_) {
        int index = audioCodecComboBox_.AddString(U2WC(codec.second));
        if (codec.first == recordingSettings.AudioCodecId) {
            selectedItemIndex = index;
        }
    }

    audioCodecComboBox_.SetCurSel(selectedItemIndex);
    audioCodecChanged(recordingSettings.AudioBitrate);

    audioBitrateUpDownControl_.SetRange32(32, 2048);

    UDACCEL accel2 { 0, 1 };
    accel2.nInc = 32;
    audioBitrateUpDownControl_.SetAccel(1, &accel2);
    audioBitrateUpDownControl_.SetPos32(recordingSettings.AudioBitrate);

    return 1;  // Let the system set the focus
}

LRESULT CDXGISettingsPage::OnHScroll(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    HWND wnd = reinterpret_cast<HWND>(lParam);
    if (wnd == videoQualityTrackBar_) {
        updateVideoQualityLabel();
    }
    return 0;
}

LRESULT CDXGISettingsPage::OnVideoRadioClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    videoRadioChanged();
    return 0;
}

LRESULT CDXGISettingsPage::OnVideoCodecChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    videoCodecChanged();
    return 0;
}

LRESULT CDXGISettingsPage::OnAudioCodecChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    audioCodecChanged();
    return 0;
}

bool CDXGISettingsPage::apply() { 
    if (DoDataExchange(TRUE)) {
        auto& recordingSettings = settings_->ScreenRecordingSettings.DXGISettings;

        recordingSettings.VideoQuality = videoQualityTrackBar_.GetPos();
        recordingSettings.VideoBitrate = videoBitrateUpDownControl_.GetPos32();
        recordingSettings.UseQuality = videoQualityRadio_.GetCheck() == BST_CHECKED;

        int videoCodecIndex = videoCodecComboBox_.GetCurSel();
        if (videoCodecIndex >= 0 && videoCodecIndex < videoCodecs_.size()) {
            recordingSettings.VideoCodecId = videoCodecs_[videoCodecIndex].first;
        }

        if (codecInfo_) {
            int videoPresetIndex = videoCodecPresetComboBox_.GetCurSel();
            if (videoPresetIndex >= 0 && videoPresetIndex < codecInfo_->Presets.size()) {
                recordingSettings.VideoPresetId = codecInfo_->Presets[videoPresetIndex].first;
            }
        }

        recordingSettings.AudioSources.clear();
        for (int i = 0; i < audioSources_.size(); i++) {
            if (audioSourcesListView_.GetCheckState(i)) {
                recordingSettings.AudioSources.push_back(audioSources_[i].first);
            }
        }
        /* int audioSourceIndex = audioSourceCombobox_.GetCurSel();
        if (audioSourceIndex >= 0 && audioSourceIndex < audioSources_.size()) {
            recordingSettings.AudioSourceId = audioSources_[audioSourceIndex].first;
        }*/

        int audioCodecIndex = audioCodecComboBox_.GetCurSel();
        if (audioCodecIndex >= 0 && audioCodecIndex < audioCodecs_.size()) {
            recordingSettings.AudioCodecId = audioCodecs_[audioCodecIndex].first;
        }

        /*if (audioCodecInfo_) {
            int audioQualityIndex = audioQualityComboBox_.GetCurSel();
            if (audioQualityIndex >= 0 && audioQualityIndex < audioCodecInfo_->Qualities.size()) {
                recordingSettings.AudioQuality = audioCodecInfo_->Qualities[audioQualityIndex].first;
            }
        }*/

        recordingSettings.AudioBitrate = audioBitrateUpDownControl_.GetPos32();

        return true;
    }

    return false;
}
