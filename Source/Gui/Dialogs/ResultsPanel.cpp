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

#include "ResultsPanel.h"

#include <utility>

#include "Core/3rdpart/pcreplusplus.h"
#include "UploadSettings.h"
#ifdef IU_ENABLE_MEDIAINFO
#include "MediaInfoDlg.h"
#endif
#include "LogWindow.h"
#include "Gui/Dialogs/WebViewWindow.h"
#include "Core/Utils/TextUtils.h"
#include "Func/IuCommonFunctions.h"
#include "Func/WinUtils.h"
#include "Core/ServiceLocator.h"
#include "Gui/Dialogs/WizardDlg.h"
#include "Core/AppRuntimeInfo.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "Gui/GuiTools.h"
#include "Core/OutputGenerator/OutputGeneratorFactory.h"
#include "Core/OutputGenerator/XmlTemplateList.h"
#include "Core/OutputGenerator/XmlTemplateGenerator.h"
#include "GUi/Helpers/DPIHelper.h"

// CResultsPanel
CResultsPanel::CResultsPanel(CWizardDlg *dlg, std::vector<Uptooda::Core::OutputGenerator::UploadObject>& urlList, bool openedFromHistory):
    WizardDlg(dlg), UrlList(urlList)
{
    namespace OG = Uptooda::Core::OutputGenerator;
    m_nImgServer = m_nFileServer = -1;
    openedFromHistory_ = openedFromHistory;
    rectNeeded = {};
    rectNeeded.left = -1;
    shortenUrl_ = false;
    outputChanged_ = false;
    m_Page = OG::clBBCode;
    m_EngineList = nullptr;
    groupByFileName_ = false;
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    CString XmlFileName = IuCommonFunctions::GetDataFolder() + _T("templates.xml");
    std::string userTemplateFile = settings->SettingsFolder + "user_templates.xml";
    templateList_ = std::make_unique<OG::XmlTemplateList>();
    try {
        templateList_->loadFromFile(W2U(XmlFileName));

    } catch (const std::exception& e) {
        ServiceLocator::instance()->logger()->write(ILogger::logWarning, _T("Results Module"), U2W(e.what()));
    }

    if (IuCoreUtils::FileExists(userTemplateFile)) {
        try {
            templateList_->loadFromFile(userTemplateFile);
        }
        catch (const std::exception& e) {
            ServiceLocator::instance()->logger()->write(ILogger::logWarning, _T("Results Module"), U2W(e.what()));
        }
    }
}

CResultsPanel::~CResultsPanel()
{
    if (webViewWindow_ ) {
        if (webViewWindow_->m_hWnd) {
            webViewWindow_->DestroyWindow();
        }
    }
}

LRESULT CResultsPanel::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    TRC(IDC_IMAGEUPLOADERLABEL, "Images per string:");
    TRC(IDC_CODETYPELABEL, "Code type:");
    if(rectNeeded.left != -1)
    {
        SetWindowPos(  nullptr, rectNeeded.left, rectNeeded.top, rectNeeded.right,  rectNeeded.bottom, SWP_NOZORDER);
    }

    if (GetStyle() & WS_CHILD) {
        EnableThemeDialogTexture(m_hWnd, ETDT_ENABLETAB);
    }
    CBitmap hBitmap;


    if (ServiceLocator::instance()->translator()->isRTL()) {
        // Removing WS_EX_RTLREADING style from some controls to look properly when RTL interface language is chosen
        HWND codeEditHwnd = GetDlgItem(IDC_CODEEDIT);
        LONG styleEx = ::GetWindowLong(codeEditHwnd, GWL_EXSTYLE);
        ::SetWindowLong(codeEditHwnd, GWL_EXSTYLE, styleEx & ~WS_EX_RTLREADING);
    }

    createToolbar();

    Toolbar.ShowWindow(SW_SHOW);

    codeTypeComboBox = GetDlgItem(IDC_CODETYPE);

    SetDlgItemInt(IDC_THUMBSPERLINE, settings->ThumbsPerLine);
    SendDlgItemMessage(IDC_THUMBPERLINESPIN, UDM_SETRANGE, 0, (LPARAM) MAKELONG((short)100, (short)0) );

    codeTypeComboBox.AddString(TR("Table of clickable thumbnails"));
    codeTypeComboBox.AddString(TR("Clickable thumbnails"));
    codeTypeComboBox.AddString(TR("Images"));
    codeTypeComboBox.AddString(TR("Links to Images/Files"));

    for(size_t i=0;i<templateList_->size(); i++) {
        codeTypeComboBox.AddString(U2W(templateList_->at(i).Name));
    }

    codeTypeComboBox.SetCurSel(0);

    SetTimer(kOutputTimer, 1000);
    return 1;  // Let the system set the focus
}

LRESULT CResultsPanel::OnTimer(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    if (wParam == kOutputTimer) {
        if (outputChanged_) {
            CString code_ = U2W(GenerateOutput());
            SetDlgItemText(IDC_CODEEDIT, code_);
            outputChanged_ = false;
        }
    }

    return 0;
}

LRESULT CResultsPanel::OnMyDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    createToolbar();
    return 0;
}

void CResultsPanel::SetPage(TabPage Index)
{
    namespace OG = Uptooda::Core::OutputGenerator;
    WtlGuiSettings& Settings = *ServiceLocator::instance()->settings<WtlGuiSettings>();
    ::EnableWindow(GetDlgItem(IDC_CODETYPELABEL), Index != OG::clPlain);
    ::EnableWindow(GetDlgItem(IDC_CODETYPE), Index != OG::clPlain);
    ::EnableWindow(GetDlgItem(IDC_IMAGEUPLOADERLABEL), Index != OG::clPlain);

    ::EnableWindow(GetDlgItem(IDC_IMAGESPERLINELABEL), Index != OG::clPlain);
    ::EnableWindow(GetDlgItem(IDC_THUMBSPERLINE), Index != OG::clPlain);
    ::EnableWindow(GetDlgItem(IDC_THUMBPERLINESPIN), Index != OG::clPlain);

    bool enablePreview = Index != OG::clMarkdown;
    Toolbar.EnableButton(IDC_PREVIEWBUTTON, enablePreview);
    m_Page = Index;


    UpdateOutput(true);
    BOOL temp;

    if (!openedFromHistory_ && !UrlList.empty() && Settings.AutoCopyToClipboard)
        OnBnClickedCopyall(0,0,0,temp);
}

bool CResultsPanel::copyResultsToClipboard() {
    BOOL temp;
    OnBnClickedCopyall(0, 0, 0, temp);
    return true;
}

std::string CResultsPanel::GenerateOutput()
{
    namespace OG = Uptooda::Core::OutputGenerator;
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();

    if (!Toolbar.m_hWnd) return {};
    int Index = GetCodeType();

    CodeType codeType = static_cast<CodeType>(Index);
    std::lock_guard<std::mutex> lock(urlListMutex_);

    int n=UrlList.size();
    int p=GetDlgItemInt(IDC_THUMBSPERLINE);
    if(p>=0 && p<5555)
        settings->ThumbsPerLine = p;
    if (p < 1) {
        p = 4;
    }
    bool useTemplate = settings->UseTxtTemplate;
    bool preferDirectLinks = settings->UseDirectLinks;
    groupByFileName_ = settings->GroupByFilename;

    OG::GeneratorID generatorId = OG::gidBBCode;
    OG::CodeLang lang = OG::clBBCode;

    if (m_Page != OG::clPlain && Index > 3) {
        //template from templates.xml
        generatorId = OG::gidXmlTemplate;
        lang = OG::clUnknown;
    } else if (m_Page >= OG::clBBCode && m_Page <= OG::clJSON) {
        generatorId = static_cast<OG::GeneratorID>(m_Page);
        lang = m_Page;
    }

    OG::AbstractOutputGenerator* generator = createOrGetGenerator(generatorId, lang, static_cast<OG::CodeType>(codeType));

    generator->setPreferDirectLinks(preferDirectLinks);
    generator->setItemsPerLine(p);
    generator->setGroupByFile(groupByFileName_);
    generator->setShortenUrl(shortenUrl_);

    if (generatorId == OG::gidXmlTemplate) {
        int templateIndex = Index - 4;
        auto xmlTemplateGenerator = dynamic_cast<OG::XmlTemplateGenerator*>(generator);
        if (xmlTemplateGenerator) {
            xmlTemplateGenerator->setTemplateIndex(templateIndex);
        }
    }

    return generator->generate(UrlList, useTemplate);
}

void CResultsPanel::UpdateOutput(bool immediately)
{
    if (immediately) {
        CString code_ = U2W(GenerateOutput());
        SetDlgItemText(IDC_CODEEDIT, code_);
    } else {
        outputChanged_ = true;
    }
}


LRESULT CResultsPanel::OnCbnSelchangeCodetype(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    onCodeTypeChanged();
    return 0;
}

void CResultsPanel::onCodeTypeChanged() {
    UpdateOutput(true);
    BOOL temp;
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    if (settings->AutoCopyToClipboard) {
        OnBnClickedCopyall(0, 0, 0, temp);
    }
}

void CResultsPanel::createToolbar() {
    const int dpi = DPIHelper::GetDpiForDialog(m_hWnd);
    const DWORD rtlStyle = ServiceLocator::instance()->translator()->isRTL() ? ILC_MIRROR | ILC_PERITEMMIRROR : 0;
    const int iconWidth = DPIHelper::GetSystemMetricsForDpi(SM_CXSMICON, dpi);
    const int iconHeight = DPIHelper::GetSystemMetricsForDpi(SM_CYSMICON, dpi);
    if (toolbarImageList_) {
        Toolbar.SetImageList(nullptr);
        toolbarImageList_.Destroy();
    }
    toolbarImageList_.Create(iconWidth, iconHeight, (GuiTools::Is32BPP() ? ILC_COLOR32 : ILC_COLOR32 | ILC_MASK) | rtlStyle, 0, 6);

    auto loadToolbarIcon = [&](int resourceId) -> int {
        CIcon icon;
        icon.LoadIconWithScaleDown(MAKEINTRESOURCE(resourceId), iconWidth, iconHeight);
        return toolbarImageList_.AddIcon(icon);
    };

    RECT rc = { 0, 0, 100, 24 };
    GetClientRect(&rc);
    rc.top = rc.bottom - MulDiv(28, dpi, USER_DEFAULT_SCREEN_DPI);
    rc.bottom -= MulDiv(4, dpi, USER_DEFAULT_SCREEN_DPI);
    rc.left = MulDiv(8, dpi, USER_DEFAULT_SCREEN_DPI);
    rc.right -= MulDiv(8, dpi, USER_DEFAULT_SCREEN_DPI);

    if (!Toolbar) {
        Toolbar.Create(m_hWnd, rc, _T(""), WS_CHILD | WS_CHILD | WS_TABSTOP | TBSTYLE_LIST | TBSTYLE_FLAT | CCS_NORESIZE /*|*/ | CCS_BOTTOM | /*CCS_ADJUSTABLE|*/ CCS_NODIVIDER | TBSTYLE_AUTOSIZE);
        Toolbar.SetWindowLong(GWL_ID, IDC_RESULTSTOOLBAR);
        Toolbar.SetButtonStructSize();
    }
    //Toolbar.SetButtonSize(iconWidth + 2, iconHeight + 2);
    int buttonCount = Toolbar.GetButtonCount();

    for (int i = buttonCount - 1; i >= 0; i--) {
        Toolbar.DeleteButton(i);
    }
    Toolbar.SetImageList(toolbarImageList_);
    Toolbar.AddButton(IDC_COPYALL, TBSTYLE_BUTTON | BTNS_AUTOSIZE, TBSTATE_ENABLED, loadToolbarIcon(IDI_CLIPBOARD), TR("Copy to clipboard"), 0);
    Toolbar.AddButton(IDC_MEDIAFILEINFO, TBSTYLE_BUTTON | BTNS_AUTOSIZE, TBSTATE_ENABLED, loadToolbarIcon(IDI_ICONINFO), TR("Info about last video"), 0);
    Toolbar.AddButton(IDC_OPTIONSMENU, TBSTYLE_DROPDOWN | BTNS_AUTOSIZE, TBSTATE_ENABLED, loadToolbarIcon(IDI_ICONSETTINGSGEAR), TR("Options"), 0);
    Toolbar.AddButton(IDC_PREVIEWBUTTON, TBSTYLE_BUTTON | BTNS_AUTOSIZE, TBSTATE_ENABLED, loadToolbarIcon(IDI_ICONPREVIEW), TR("Preview"), 0);
    //Toolbar.HideButton(IDC_MEDIAFILEINFO);
    EnableMediaInfo(isMediaInfoEnabled_);
    Toolbar.AutoSize();
}

LRESULT CResultsPanel::OnBnClickedCopyall(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    namespace OG = Uptooda::Core::OutputGenerator;
    CString buffer = U2W(GenerateOutput());
    WinUtils::CopyTextToClipboard(buffer);
    if (m_Page == OG::clHTML && !buffer.IsEmpty()) {
        WinUtils::CopyHtmlToClipboard(buffer);
    }
    return 0;
}

// "Thumb per line" edit text changed event
LRESULT CResultsPanel::OnEditChanged(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    onCodeTypeChanged();
    return 0;
 }

LRESULT CResultsPanel::OnBnClickedMediaInfo(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
#ifdef IU_ENABLE_MEDIAINFO
    if (WizardDlg->getLastVideoFile().IsEmpty()) {
        return 0;
    }
    CMediaInfoDlg dlg;
    dlg.ShowInfo(m_hWnd, WizardDlg->getLastVideoFile());
#endif
    return 0;
}


int CResultsPanel::GetCodeType() const {
    return codeTypeComboBox.GetCurSel();
}

void CResultsPanel::SetCodeType(int Index) {
    codeTypeComboBox.SetCurSel(Index);
    onCodeTypeChanged();
}

void CResultsPanel::Clear()
{
    SetDlgItemText(IDC_CODEEDIT, _T(" "));
}

void  CResultsPanel::EnableMediaInfo(bool Enable)
{
    Toolbar.HideButton(IDC_MEDIAFILEINFO,!Enable);
    isMediaInfoEnabled_ = Enable;
}

LRESULT CResultsPanel::OnOptionsDropDown(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    auto* pnmtb = reinterpret_cast<NMTOOLBAR *>(pnmh);
    CMenu sub;
    MENUITEMINFO mi {};
    mi.cbSize = sizeof(mi);
    mi.fMask = MIIM_TYPE|MIIM_ID;
    mi.fType = MFT_STRING;
    sub.CreatePopupMenu();
    RECT rc;

    ::GetWindowRect(GetDlgItem(IDC_RESULTSTOOLBAR),&rc);
    int count = 0;
    mi.fType = MFT_STRING;
    mi.wID = IDC_SHORTENURLITEM;
    CString menuItemTitle;
    if (onShortenUrlChanged_) {
        menuItemTitle.Format(TR("Shorten URL using %s"), IuCoreUtils::Utf8ToWstring(settings->urlShorteningServer.serverName()).c_str());
    } else {
        menuItemTitle = TR("Shorten URL");
    }
    mi.dwTypeData = const_cast<LPWSTR>(menuItemTitle.GetString());
    mi.cch = menuItemTitle.GetLength();
    sub.InsertMenuItem(count++, true, &mi);    

    mi.fType = MFT_STRING;
    mi.wID = IDC_USEDIRECTLINKS;
    CString useDirectLinks = TR("Use direct links");
    mi.dwTypeData = const_cast<LPWSTR>(useDirectLinks.GetString());
    mi.cch = useDirectLinks.GetLength();
    sub.InsertMenuItem(count++, true, &mi);

    mi.fType = MFT_STRING;
    mi.wID = IDC_GROUPBYFILENAME;
    CString groupByFilename = TR("Group by filename");
    mi.dwTypeData = const_cast<LPWSTR>(groupByFilename.GetString());
    mi.cch = groupByFilename.GetLength();
    sub.InsertMenuItem(count++, true, &mi);

    mi.wID = IDC_USETEMPLATE;
    CString useTemplate = TR("Use template");
    mi.dwTypeData = const_cast<LPWSTR>(useTemplate.GetString());
    mi.cch = useTemplate.GetLength();
    sub.InsertMenuItem(count++, true, &mi);

    int insertedServersCount = 0;
    int curPosition = count;

    for(size_t i=0; i<m_Servers.size(); i++)
    {
        const auto& server = m_Servers[i];
        CUploadEngineData* ue = server.uploadEngineData();
        if (!ue) {
            continue;
        }
        CString folderTitle = Utf8ToWCstring(server.folderTitle());
        if (server.folderTitle().empty() || server.folderUrl().empty()) {
            continue;
        }
        std::string titleU8 = str(IuStringUtils::FormatNoExcept(_("Copy URL of %1%->%2%")) % CUploadEngineListBase::getServerDisplayName(ue) % server.folderTitle());
        CString title = U2WC(titleU8);
        mi.wID = IDC_COPYFOLDERURL + i;
        mi.dwTypeData = const_cast<LPWSTR>(title.GetString());
        mi.cch = title.GetLength();
        sub.InsertMenuItem(count++, true, &mi);
        insertedServersCount++;
    }

    if(insertedServersCount)
    {
        mi.wID = IDC_FILESERVER_LAST_ID + 1;
        mi.fType = MFT_SEPARATOR;
        sub.InsertMenuItem(curPosition, true, &mi);
        count++;
    }

    sub.CheckMenuItem(IDC_USEDIRECTLINKS, MF_BYCOMMAND    | (settings->UseDirectLinks? MF_CHECKED    : MF_UNCHECKED)    );
    sub.CheckMenuItem(IDC_USETEMPLATE, MF_BYCOMMAND    | (settings->UseTxtTemplate? MF_CHECKED    : MF_UNCHECKED)    );
    sub.CheckMenuItem(IDC_SHORTENURLITEM, MF_BYCOMMAND    | (shortenUrl_? MF_CHECKED    : MF_UNCHECKED)    );
    sub.CheckMenuItem(IDC_GROUPBYFILENAME, MF_BYCOMMAND    | (groupByFileName_? MF_CHECKED    : MF_UNCHECKED)    );

    ::SendMessage(Toolbar.m_hWnd,TB_GETRECT, pnmtb->iItem, reinterpret_cast<LPARAM>(&rc));
    Toolbar.ClientToScreen(&rc);
    TPMPARAMS excludeArea;
    ZeroMemory(&excludeArea, sizeof(excludeArea));
    excludeArea.cbSize = sizeof(excludeArea);
    excludeArea.rcExclude = rc;
    sub.TrackPopupMenuEx( TPM_LEFTALIGN | TPM_LEFTBUTTON | TPM_VERTICAL,
    rc.left, rc.bottom, m_hWnd, /*&rc, */&excludeArea);
    bHandled = true;
    return TBDDRET_DEFAULT;
}

LRESULT CResultsPanel::OnUseTemplateClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    settings->UseTxtTemplate = !settings->UseTxtTemplate;
    UpdateOutput(true);
    return 0;
}

LRESULT CResultsPanel::OnUseDirectLinksClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    settings->UseDirectLinks = !settings->UseDirectLinks;
    UpdateOutput(true);
    return 0;
}

LRESULT CResultsPanel::OnCopyFolderUrlClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    int index = wID - IDC_COPYFOLDERURL;

    CUploadEngineData *ue = m_Servers[index].uploadEngineData();
    if(!ue) return 0;
    CString folderUrl = Utf8ToWCstring( m_Servers[index].folderUrl());
    WinUtils::CopyTextToClipboard(folderUrl);
    return 0;
}

void CResultsPanel::AddServer(const ServerProfile& server)
{
    for (const auto& cur : m_Servers) {
        if (cur.serverName() == server.serverName() && cur.profileName() == server.profileName()
                && cur.folderId() == server.folderId()) {
            return;
        }
    }
    m_Servers.push_back(server);
    //return 0;
}

void CResultsPanel::setEngineList(CMyEngineList* EngineList)
{
    m_EngineList = EngineList;
}

void CResultsPanel::InitUpload()
{
    m_Servers.clear();
    shortenUrl_ = false;
}

LRESULT CResultsPanel::OnShortenUrlClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    shortenUrl_ = !shortenUrl_;
    if (onShortenUrlChanged_) {
        onShortenUrlChanged_(shortenUrl_);
    } else {
        UpdateOutput(true);
    }
    return 0;
}


LRESULT CResultsPanel::OnPreviewButtonClicked(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    WtlGuiSettings& Settings = *ServiceLocator::instance()->settings<WtlGuiSettings>();
    std::string url ;
    if ( m_Page == 2 && this->UrlList.size() ) {
        //use
        url = this->UrlList[0].getImageUrl();
        if ((!Settings.UseDirectLinks || url.empty()) &&  !this->UrlList[0].getDownloadUrl().empty()) {
            url = this->UrlList[0].getDownloadUrl();
        }
    } else {
        std::string outputTempFileName = AppRuntimeInfo::instance()->tempDirectory()  + "preview.html";
        std::string code = GenerateOutput();
        /*if ( m_Page == 0) {
            code = Utf8ToWCstring(IuTextUtils::BbCodeToHtml(WCstringToUtf8(code)));
        } */
        std::string res = "<html><head><meta http-equiv=\"content-type\" content=\"text/html; charset=UTF-8\" />"
            "<style>img {border:none;}</style>"
            "</head><body>";
        res += "<span id=\"resultcode\">" + code + "</span>";
        if (m_Page == 0) {
            std::string script;
            std::string scriptFileName = W2U(IuCommonFunctions::GetDataFolder()) + "Utils/xbbcode.js";
            if (!IuCoreUtils::ReadUtf8TextFile(scriptFileName, script))
            {
                LOG(ERROR) << "Cannot read file " << scriptFileName;
            }
            res += "<script type=\"text/javascript\">" + script +
                "\r\n"
                "var el = document.getElementById('resultcode');\r\n"
                "var result = XBBCODE.process({   \r\n"
                "   text: el.innerHTML,   \r\n"
                "   removeMisalignedTags: false,  \r\n"
                "   addInLineBreaks: true   \r\n"
                "});  \r\n"
                "el.innerHTML = result.html; \r\n"
                "</script>";
        }
       res+= "</body></html>";

        if (!IuTextUtils::FileSaveContents(outputTempFileName, res) ) {
            LOG(ERROR) << "Could not save temporary file " << outputTempFileName;
        }
        url = "file:///" + outputTempFileName;
    }

    if (url.empty() ) {
        return 0;
    }

    CRect r(0,0,600,400);
    if ( !webViewWindow_ ) {
        webViewWindow_ = std::make_unique<CWebViewWindow>();
        webViewWindow_->Create(0,r,TR("Preview Window"),WS_POPUP|WS_OVERLAPPEDWINDOW,WS_EX_TOPMOST    );
        webViewWindow_->CenterWindow(WizardDlg->m_hWnd);
        webViewWindow_->ShowWindow(SW_SHOW);
    }

    webViewWindow_->NavigateTo(U2W(url));
    webViewWindow_->ShowWindow(SW_SHOW);
//    webViewWindow_->ActivateWindow();
    return 0;
}

std::mutex& CResultsPanel::outputMutex() {
    return urlListMutex_;
}

void CResultsPanel::setRectNeeded(const RECT& rc) {
    rectNeeded = rc;
}

void CResultsPanel::setShortenUrls(bool shorten) {
    shortenUrl_ = shorten;
}

void CResultsPanel::setOnShortenUrlChanged(ShortenUrlChangedCallback callback) {
    onShortenUrlChanged_ = std::move(callback);
}

void CResultsPanel::setGroupByFilename(bool enable) {
    groupByFileName_ = enable;
}

Uptooda::Core::OutputGenerator::AbstractOutputGenerator* CResultsPanel::createOrGetGenerator(Uptooda::Core::OutputGenerator::GeneratorID gid,  Uptooda::Core::OutputGenerator::CodeLang lang,
        Uptooda::Core::OutputGenerator::CodeType type) {
    using namespace Uptooda::Core::OutputGenerator;
    auto it = outputGenerators_.find(gid);
    AbstractOutputGenerator* res{};
    if (it == outputGenerators_.end()) {
        OutputGeneratorFactory factory;
        auto tmp = factory.createOutputGenerator(gid, type, templateList_.get());
        CString templateFileName = IuCommonFunctions::GetDataFolder() + _T("template.txt");
        tmp->loadTemplate(W2U(templateFileName));
        res = tmp.get();
        outputGenerators_[gid] = std::move(tmp);
    } else {
        res = it->second.get();
    }
    if (!res) {
        return {};
    }
    res->setType(type);
    return res;
}

LRESULT CResultsPanel::OnGroupByFilenameClicked(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    groupByFileName_ = !groupByFileName_;
    settings->GroupByFilename = groupByFileName_;
    UpdateOutput(true);
    return 0;
}
