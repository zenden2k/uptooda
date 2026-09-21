#include "ScreenRecordingSettingsPage.h"

#include "Core/CommonDefs.h"
#include "WizardDlg.h"
#include "Gui/GuiTools.h"
#include "Func/WinUtils.h"
#include "Gui/Components/NewStyleFolderDialog.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "FFmpegSettingsPage.h"
#include "DXGISettingsPage.h"
#include "Gui/Helpers/DPIHelper.h"
#include "Gui/Constants.h"

CScreenRecordingSettingsPage::CScreenRecordingSettingsPage() {
    settings_ = ServiceLocator::instance()->settings<WtlGuiSettings>();
}

void CScreenRecordingSettingsPage::TranslateUI() {
    TRC(IDC_BACKENDLABEL, "Backend:");
    TRC(IDC_OUTFOLDERLABEL, "Video recordings folder:");
    TRC(IDC_OUTFOLDERBROWSEBUTTON, "Browse...");
    TRC(IDC_FRAMERATELABEL, "Frame rate:");
    TRC(IDC_FILENAMETEMPLATELABEL, "Filename and path template:");
}

template <typename T, typename... Args>
std::unique_ptr<T> createPageObject(HWND hWnd, RECT& rc, Args&&... args)
{
    auto dlg = std::make_unique<T>(std::forward<Args>(args)...);
    dlg->Create(hWnd, rc);
    dlg->PageWnd = dlg->m_hWnd;
    return dlg;
}
void CScreenRecordingSettingsPage::showSubPage(SubPage pageId) {
    if (curPage_ == pageId || pageId < 0 || pageId >= std::size(subPages_)) {
        return;
    }

    if (!subPages_[pageId]) {
        RECT rc = { 150, 3, 636, 400 };
        auto createObject = [&]() -> std::unique_ptr<CSettingsPage> {
            switch (pageId) {
            case spFFmpegSettings:
                return createPageObject<CFFmpegSettingsPage>(m_hWnd, rc);
            case spDirectXSettings:
                return createPageObject<CDXGISettingsPage>(m_hWnd, rc);
            default:
                LOG(ERROR) << "No such page " << pageId;
                return {};
            }
        };

        std::unique_ptr<CSettingsPage> page = createObject();
        if (!page) {
            return;
        }
        subPages_[pageId] = std::move(page);

        WINDOWPLACEMENT wp;
        HWND placeholder = GetDlgItem(IDC_SUBPAGEPLACEHOLDER);
        ::GetWindowPlacement(placeholder, &wp);
        ::SetWindowPos(subPages_[pageId]->PageWnd, placeholder, wp.rcNormalPosition.left, wp.rcNormalPosition.top, -wp.rcNormalPosition.left + wp.rcNormalPosition.right, -wp.rcNormalPosition.top + wp.rcNormalPosition.bottom, 0);

        subPages_[pageId]->fixBackground();
    }

    if (!subPages_[pageId]) {
        return;
    }

    HWND wnd = subPages_[pageId]->PageWnd;

    if (wnd) {
        ::ShowWindow(wnd, SW_SHOW);
    }

    if (curPage_ != spNone && subPages_[curPage_]) {
        ::ShowWindow(subPages_[curPage_]->PageWnd, SW_HIDE);
    }
    curPage_ = pageId;
}

void CScreenRecordingSettingsPage::createResources() {
    const int dpi = DPIHelper::GetDpiForDialog(m_hWnd);
    const int iconWidth = DPIHelper::GetSystemMetricsForDpi(SM_CXSMICON, dpi);
    const int iconHeight = DPIHelper::GetSystemMetricsForDpi(SM_CYSMICON, dpi);

    if (helpButtonIcon_) {
        helpButtonIcon_.DestroyIcon();
    }

    helpButtonIcon_.LoadIconWithScaleDown(MAKEINTRESOURCE(IDI_ICON_HELP_DROPDOWN), iconWidth, iconHeight);
    helpButton_.SetIcon(helpButtonIcon_);

    if (iconInfo_) {
        iconInfo_.DestroyIcon();
    }
    iconInfo_.LoadIconWithScaleDown(MAKEINTRESOURCE(IDI_ICONINFO), iconWidth, iconHeight);
    fileNameMacrosButton_.SetIcon(iconInfo_);
}

LRESULT CScreenRecordingSettingsPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    TranslateUI();
    DoDataExchange(FALSE);

    int backendComboIndex = -1;
    for (const auto& backendName : WtlGuiSettings::ScreenRecordingBackends) {
        int index = backendCombobox_.AddString(U2WC(backendName));
        if (backendName == settings_->ScreenRecordingSettings.Backend) {
            backendComboIndex = index;
        }
    }
    backendCombobox_.SetCurSel(backendComboIndex);

    showSubPage(static_cast<SubPage>(backendComboIndex));

    outFolderEditControl_.SetWindowText(U2W(settings_->ScreenRecordingSettings.OutDirectory));
    fileNameTemplateEditControl_.SetWindowText(U2W(settings_->ScreenRecordingSettings.FileNameTemplate));

    frameRateUpDownControl_.SetRange(1, 60);
    frameRateUpDownControl_.SetPos(settings_->ScreenRecordingSettings.FrameRate);

    toolTip_.Create(m_hWnd);
    CString tooltipText = TR("Help");
    CToolInfo tip(TTF_SUBCLASS, helpButton_, 0, nullptr, const_cast<LPTSTR>(tooltipText.GetString()));
    toolTip_.AddTool(tip);

    tooltipText = TR("Macros list");
    CToolInfo fileNameMacrosTooltip(TTF_SUBCLASS, fileNameMacrosButton_, 0, nullptr, const_cast<LPTSTR>(tooltipText.GetString()));
    toolTip_.AddTool(fileNameMacrosTooltip);

    createResources();

    return 1;  // Let the system set the focus
}

LRESULT CScreenRecordingSettingsPage::OnBackendChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    int backendIndex = backendCombobox_.GetCurSel();

    if (backendIndex >= 0 && backendIndex < std::size(subPages_)) {
        showSubPage(static_cast<SubPage>(backendIndex));
    }
    return 0;
}

LRESULT CScreenRecordingSettingsPage::OnBnClickedHelpButton(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    WinUtils::ShellOpenFileOrUrl(_T("https://svistunov.dev/screen-recording"), m_hWnd);
    return 0;
}

LRESULT CScreenRecordingSettingsPage::OnMyDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    createResources();
    return 0;
}

bool CScreenRecordingSettingsPage::apply() { 
    if (DoDataExchange(TRUE)) {
        const CString fileNameTemplate = GuiTools::GetWindowText(fileNameTemplateEditControl_);

        if (fileNameTemplate.IsEmpty()) {
            throw ValidationException(TR("The filename template cannot be empty!"), fileNameTemplateEditControl_);
        }

        if (fileNameTemplate.FindOneOf(FORBIDDEN_FILEPATH_TEMPLATE_CHARACTERS) != -1) {
            throw ValidationException(TR("The filename template contains forbidden characters!"), fileNameTemplateEditControl_);
        }

        for (int i = 0; i < std::size(subPages_); i++) {
            const auto& page = subPages_[i];
            if (!page) {
                continue;
            }
            page->clearErrors();

            if (!page->validate()) {
                showSubPage(static_cast<SubPage>(i));
                const auto& errors = page->errors();
                if (!errors.empty()) {
                    CString msg;
                    for (const auto& error : errors) {
                        msg += error.Message;
                        msg += _T("\r\n");
                    }
                    GuiTools::LocalizedMessageBox(m_hWnd, msg, TR("Error"), MB_ICONERROR);
                    if (errors[0].Control) {
                        ::SetFocus(errors[0].Control);
                    }
                }
                return false;
            }
        }

        for (int i = 0; i < std::size(subPages_); i++) {
            try {
                if (subPages_[i] && !subPages_[i]->apply()) {
                    showSubPage(static_cast<SubPage>(i));
                    return false;
                }
            } catch (ValidationException& ex) {
                showSubPage(static_cast<SubPage>(i));
                if (!ex.errors_.empty()) {
                    GuiTools::LocalizedMessageBox(m_hWnd, ex.errors_[0].Message, TR("Error"), MB_ICONERROR);
                    if (ex.errors_[0].Control) {
                        ::SetFocus(ex.errors_[0].Control);
                    }
                }

                return false;
                // If some tab cannot apply changes - do not close dialog
            }
        }

        auto& recodingSettings = settings_->ScreenRecordingSettings;
        int backendComboIndex_ = backendCombobox_.GetCurSel();
        if (backendComboIndex_ >= 0 && backendComboIndex_ < CommonGuiSettings::ScreenRecordingBackends.size()) {
            recodingSettings.Backend = CommonGuiSettings::ScreenRecordingBackends[backendComboIndex_];
        }

        recodingSettings.OutDirectory = W2U(GuiTools::GetWindowText(outFolderEditControl_));
        recodingSettings.FileNameTemplate = W2U(fileNameTemplate);

        recodingSettings.FrameRate = frameRateUpDownControl_.GetPos();
       
        return true;
    }

    return false;
}

LRESULT CScreenRecordingSettingsPage::OnBnClickedBrowseButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    CString folder;
    outFolderEditControl_.GetWindowText(folder);
    CNewStyleFolderDialog fd(m_hWnd, folder, TR("Select folder"));

    fd.SetFolder(folder);

    if (fd.DoModal(m_hWnd) == IDOK) {
        outFolderEditControl_.SetWindowText(fd.GetFolderPath());
        return true;
    }
    
    return 0;
}

LRESULT CScreenRecordingSettingsPage::OnFilenameMacrosButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl)
{
    const std::vector<std::pair<CString, CString>> items {
            { _T("%y"), TR("year")},
            { _T("%m"), TR("month")},
            { _T("%d"), TR("day")},
            { _T("%h"), TR("hour")},
            { _T("%n"), TR("minute")},
            { _T("%s"), TR("second")},
            { _T("%i"), TR("index")},
            { _T("%width%"), TR("video width")},
            { _T("%height%"), TR("video height") }
        };
    RECT rc {};
    ::GetWindowRect(hWndCtl, &rc);
    POINT menuOrigin { rc.left, rc.bottom };

    CMenu macrosMenu;

    int id = 1;
    macrosMenu.CreatePopupMenu();

    for (const auto& item : items) {
        CString title = item.first + _T(" - ") + item.second;
        macrosMenu.AppendMenu(MF_STRING, id++, title);
    }

    TPMPARAMS excludeArea;
    ZeroMemory(&excludeArea, sizeof(excludeArea));
    excludeArea.cbSize = sizeof(excludeArea);
    excludeArea.rcExclude = rc;

    int result = macrosMenu.TrackPopupMenuEx(TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_RETURNCMD | TPM_NONOTIFY, menuOrigin.x, menuOrigin.y, m_hWnd, &excludeArea);
    if (result && (result - 1 < items.size())) {
        fileNameTemplateEditControl_.ReplaceSel(items[result - 1].first, TRUE);
    }

    return 0;
}