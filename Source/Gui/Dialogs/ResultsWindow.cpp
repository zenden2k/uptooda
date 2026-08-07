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
#include "ResultsWindow.h"

#include "atlheaders.h"
#include "Func/Common.h"
#include "Gui/GuiTools.h"
#include "Core/ServiceLocator.h"
#include "Core/Settings/WtlGuiSettings.h"

namespace OG = Uptooda::Core::OutputGenerator;
// CResultsWindow
CResultsWindow::CResultsWindow(CWizardDlg *wizardDlg,std::vector<OG::UploadObject>  & urlList,bool ChildWindow)
{
    m_WizardDlg = wizardDlg;
    ResultsPanel = std::make_unique<CResultsPanel>(wizardDlg, urlList, !ChildWindow);
    m_childWindow = ChildWindow;
    tabPageToCodeLang[0] = OG::clBBCode;
    tabPageToCodeLang[1] = OG::clHTML;
    tabPageToCodeLang[2] = OG::clMarkdown;
    tabPageToCodeLang[3] = OG::clPlain;
    hMyDlgTemplate_ = nullptr;
}


CResultsWindow::~CResultsWindow()
{
    if (hMyDlgTemplate_) {
        GlobalFree(hMyDlgTemplate_);
    }
}

void CResultsWindow::setOnShortenUrlChanged(std::function<void(bool)> fd) {
    ResultsPanel->setOnShortenUrlChanged(std::move(fd));
}

LRESULT CResultsWindow::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    if(m_childWindow)
    {
        ::ShowWindow(GetDlgItem(IDOK),SW_HIDE);
        ::ShowWindow(GetDlgItem(IDCANCEL),SW_HIDE);
        ::SetWindowPos(GetDlgItem(IDC_RESULTSTAB), 0,0,0,0,0,SWP_NOSIZE);

    }
    else
    {
        CenterWindow(GetParent());
        HICON hIcon = GuiTools::LoadBigIcon(IDR_MAINFRAME);
        iconSmall_ = GuiTools::LoadSmallIcon(IDR_MAINFRAME);
        SetIcon(hIcon, TRUE);
        SetIcon(iconSmall_, FALSE);
    }

    ::SetFocus(GetDlgItem(IDOK));
    SetWindowText(TR("Upload results"));

    TRC(IDCANCEL, "Close");
    resultsTabCtrl_.m_hWnd = GetDlgItem(IDC_RESULTSTAB);

    resultsTabCtrl_.InsertItem(0, TR("Forum code (BBCode)"));
    resultsTabCtrl_.InsertItem(1, TR("HTML code"));
    resultsTabCtrl_.InsertItem(2, _T("Markdown"));
    resultsTabCtrl_.InsertItem(3, TR("Links (URL)"));
    resultsTabCtrl_.SetCurSel(settings->CodeLang);

    // Creating panel with results
    WINDOWPLACEMENT wp;
    resultsTabCtrl_.GetWindowPlacement(&wp);
    resultsTabCtrl_.AdjustRect(FALSE, &wp.rcNormalPosition);

    RECT rc =  { wp.rcNormalPosition.left, wp.rcNormalPosition.top, -wp.rcNormalPosition.left+wp.rcNormalPosition.right,  -wp.rcNormalPosition.top+wp.rcNormalPosition.bottom };
    ResultsPanel->setRectNeeded(rc);
    ::MapWindowPoints(nullptr, m_hWnd, reinterpret_cast<LPPOINT>(&rc), 2);
    CMyEngineList* myEngineList = ServiceLocator::instance()->myEngineList();
    ResultsPanel->setEngineList(myEngineList);
    ResultsPanel->Create(m_hWnd,rc);
    using namespace Uptooda::Core::OutputGenerator;
    SetPage(static_cast<CodeLang>(settings->CodeLang));
    ResultsPanel->SetCodeType(settings->CodeType);
    ResultsPanel->GetClientRect(&rc);
    BOOL b;
    OnTabChanged(IDC_RESULTSTAB, 0, b);
    return 0;
}


LRESULT CResultsWindow::OnClickedOK(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(wID);
    return 0;
}

LRESULT CResultsWindow::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    EndDialog(wID);
    return 0;
}

LRESULT CResultsWindow::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    settings->CodeLang = GetPage();
    settings->CodeType = GetCodeType();
    return 0;
}

int CResultsWindow::GetCodeType() const
{
    return ResultsPanel->GetCodeType();
}

void CResultsWindow::UpdateOutput(bool immediately)
{
    ResultsPanel->UpdateOutput(immediately);
}

void CResultsWindow::SetCodeType(int Index)
{
    ResultsPanel->SetCodeType(Index);
}

void CResultsWindow::Clear()
{
    ResultsPanel->Clear();
}

void CResultsWindow::EnableMediaInfo(bool Enable)
{
    ResultsPanel->EnableMediaInfo(Enable);
}

void CResultsWindow::SetPage(OG::CodeLang Index)
{
    auto it = std::find_if(tabPageToCodeLang.begin(), tabPageToCodeLang.end(), [&](const std::pair<int,int>& v) {return v.second == Index; });

    if (it != tabPageToCodeLang.end()) {
        resultsTabCtrl_.SetCurSel(it->first);
        ResultsPanel->SetPage(Index);
    }
}

int CResultsWindow::GetPage()
{
    int curTab = resultsTabCtrl_.GetCurSel();
    auto it = tabPageToCodeLang.find(curTab);
    return it != tabPageToCodeLang.end()? it->second : 0;
}
void CResultsWindow::AddServer(const ServerProfile& server)
{
    ResultsPanel->AddServer(server);
}

void CResultsWindow::InitUpload()
{
    ResultsPanel->InitUpload();
}

std::mutex& CResultsWindow::outputMutex() {
    return ResultsPanel->outputMutex();
}

bool CResultsWindow::copyResultsToClipboard() {
    return ResultsPanel->copyResultsToClipboard();
}

LRESULT CResultsWindow::OnTabChanged(int idCtrl, LPNMHDR pnmh, BOOL& bHandled)
{
    ResultsPanel->SetPage(static_cast<CResultsPanel::TabPage>(GetPage()));
    return 0;
}

LRESULT CResultsWindow::OnDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    dpiChanged(LOWORD(wParam), HIWORD(wParam));
    return 0;
}

LRESULT CResultsWindow::OnMyDpiChanged(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    dpiChanged(LOWORD(wParam), HIWORD(wParam));
    return 0;
}

void CResultsWindow::dpiChanged(int dpiX, int dpiY) {
    ResultsPanel->SendMessage(WM_MY_DPICHANGED, MAKEWPARAM(dpiX, dpiY));
}

DLGTEMPLATE* CResultsWindow::GetTemplate()
{
    HINSTANCE hInst =  GetModuleHandle(0);
    HRSRC res = FindResource( hInst, MAKEINTRESOURCE(IDD_RESULTSWINDOW),RT_DIALOG);
    auto* dit = static_cast<DLGTEMPLATE*>(LockResource(LoadResource(hInst, res)));

    size_t sizeDlg = ::SizeofResource(hInst, res);
    hMyDlgTemplate_ = ::GlobalAlloc(GPTR, sizeDlg);
    auto *pMyDlgTemplate = static_cast<ATL::_DialogSplitHelper::DLGTEMPLATEEX*>(::GlobalLock(hMyDlgTemplate_));
    ::memcpy(pMyDlgTemplate, dit, sizeDlg);

    if(m_childWindow)
    {
        //pMyDlgTemplate->style = pMyDlgTemplate->style & ~ WS_POPUP;
        pMyDlgTemplate->style = pMyDlgTemplate->style |  WS_CHILD;
    }
    else
    {
        pMyDlgTemplate->style -= WS_CHILD;
        pMyDlgTemplate->style -= DS_CONTROL;
        pMyDlgTemplate->exStyle|=WS_EX_APPWINDOW;
        pMyDlgTemplate->style = pMyDlgTemplate->style |  WS_POPUP | WS_CAPTION;
    }
    if (ServiceLocator::instance()->translator()->isRTL()) {
        pMyDlgTemplate->exStyle |= WS_EX_RTLREADING | WS_EX_LAYOUTRTL;
    }
    return reinterpret_cast<DLGTEMPLATE*>(pMyDlgTemplate);
}

void CResultsWindow::FinishUpload()
{
    BOOL b;
    ResultsPanel->OnCbnSelchangeCodetype(0,0,0, b);
}

void CResultsWindow::setShortenUrls(bool shorten) {
    ResultsPanel->setShortenUrls(shorten);
}
