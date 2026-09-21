#include "FolderAddDlg.h"

#include <io.h>

#include "Func/WinUtils.h"
#include "Core/ServiceLocator.h"
#include "WizardDlg.h"
#include "MainDlg.h"
#include "Func/IuCommonFunctions.h"
#include "Core/Settings/WtlGuiSettings.h"

CFolderAdd::CFolderAdd(CWizardDlg *WizardDlg): 
    count(0), 
    m_bImagesOnly(false),
    m_pWizardDlg(WizardDlg), findfile(nullptr)
    , dlg(CStatusDlg::create()) {
    
    m_bSubDirs = true;
    memset(&wfd, 0, sizeof(wfd));
}

void CFolderAdd::Do(CStringList &Paths, bool ImagesOnly, bool SubDirs)
{
    count = 0;
    m_bImagesOnly = ImagesOnly;
    RECT rc = { 0, 0, 0, 0 };
    m_bSubDirs = SubDirs;
    if (!dlg->m_hWnd) {
        dlg->Create(m_pWizardDlg->m_hWnd, rc);
    }
    dlg->SetWindowTitle(CString(ImagesOnly ? TR("Searching for picture files...") : TR("Collecting files...")));
    m_Paths.RemoveAll();
    m_Paths.Copy(Paths);
    findfile = 0;
    ZeroMemory(&wfd, sizeof(wfd));

    // Release thread handle to avoid leak
    Release();
    
    Start(THREAD_PRIORITY_BELOW_NORMAL);
}

int CFolderAdd::ProcessDir(CString currentDir, bool bRecursive /* = true  */)
{
    dlg->SetInfo(CString(TR("Processing folder:")), currentDir);
    CString strWildcard = currentDir + "\\*";

    _tfinddata_t s_Dir;
    intptr_t hDir;

    if ((hDir = _tfindfirst(strWildcard, &s_Dir)) == -1L)

        return 1;

    do {
        if (dlg->NeedStop()) { _findclose(hDir); return 0; }

        if (s_Dir.name[0] != '.' && (s_Dir.attrib & _A_SUBDIR) && bRecursive == true) {
            ProcessDir(currentDir + '\\' + s_Dir.name, bRecursive);
        } else if (s_Dir.name[0] != '.') {
            if (!m_bImagesOnly || IuCommonFunctions::IsImage(s_Dir.name)) {
                CWizardDlg::AddImageStruct ais;
                ais.show = !m_pWizardDlg->getQuickUploadMarker();
                CString name = CString(currentDir) + CString(_T("\\")) + CString(s_Dir.name);
                ais.RealFileName = name;
                //CString name = CurPath;
                /*if (SendMessage(m_pWizardDlg->m_hWnd, WM_MY_ADDIMAGE, (WPARAM)&ais, 0))

                    count++;*/
                ServiceLocator::instance()->taskRunner()->runInGuiThread([&] {
                    if (m_pWizardDlg->AddImage(ais.RealFileName, ais.VirtualFileName, ais.show)) {
                        count++;
                    }
                }, false);

            }
        }
    } while (_tfindnext(hDir, &s_Dir) == 0);

    _findclose(hDir);

    return 0;
}

DWORD CFolderAdd::Run()
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    EnableWindow(m_pWizardDlg->m_hWnd, false);
    ServiceLocator::instance()->taskRunner()->runInGuiThread([&] {
        m_pWizardDlg->beginAddFiles();
    });
    for (size_t i = 0; i<m_Paths.GetCount(); i++) {
        CString CurPath = m_Paths[i];
        if (WinUtils::IsDirectory(CurPath))
            ProcessDir(CurPath, m_bSubDirs);
        else
            if (!m_bImagesOnly || IuCommonFunctions::IsImage(CurPath)) {
                CWizardDlg::AddImageStruct ais;
                ais.show = !m_pWizardDlg->getQuickUploadMarker();
                //CString name = CurPath;
                ais.RealFileName = CurPath;
                if (SendMessage(m_pWizardDlg->m_hWnd, WM_MY_ADDIMAGE, (WPARAM)&ais, 0))

                    count++;
            }
        if (dlg->NeedStop()) break;
    }

    ServiceLocator::instance()->taskRunner()->runInGuiThread([&] {
        m_pWizardDlg->SetActiveWindow();
    });
    
    
    dlg->Hide();

    EnableWindow(m_pWizardDlg->m_hWnd, true);

    if (!count)
        GuiTools::LocalizedMessageBox(m_pWizardDlg->m_hWnd, m_bImagesOnly ? TR("No pictures were found.") : TR("No files were found."), APP_NAME, MB_ICONINFORMATION);
    else {
        if (m_pWizardDlg->getQuickUploadMarker()) {
            if (settings->CheckFileTypesBeforeUpload && !m_pWizardDlg->checkFileFormats(m_pWizardDlg->getSessionImageServer(), m_pWizardDlg->getSessionFileServer(), settings->DefaultImageUploadParams)) {
                m_pWizardDlg->ShowPage(CWizardDlg::wpUploadSettingsPage);
            } else {
                m_pWizardDlg->ShowPage(CWizardDlg::wpUploadPage);
            }

        } else {
            m_pWizardDlg->ShowPage(CWizardDlg::wpMainPage);
            //m_pWizardDlg->getPage<CMainDlg>(CWizardDlg::wpMainPage)->ThumbsView.LoadThumbnails();
        }
    }
    ServiceLocator::instance()->taskRunner()->runInGuiThread([&] {
        m_pWizardDlg->endAddFiles();
    });
    dlg->DestroyWindow();
    dlg->m_hWnd = NULL;

    return 0;
}
