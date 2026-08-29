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

#include "VideoGrabberPage.h"

#include <boost/format.hpp>

#include "Core/CommonDefs.h"
#include "Core/Images/GdiPlusImage.h"
#include "Gui/Dialogs/SettingsDlg.h"
#include "Video/VideoGrabber.h"

#include "Func/WinUtils.h"
#ifdef IU_ENABLE_MEDIAINFO
#include "MediaInfoDlg.h"
#endif
#include "Core/AppRuntimeInfo.h"
#include "Core/Images/Utils.h"
#include "Core/Logging.h"
#include "Core/ServiceLocator.h"
#include "Core/Settings/WtlGuiSettings.h"
#include "Core/Utils/StringUtils.h"
#include "Func/ImageGenerator.h"
#include "Func/IuCommonFunctions.h"
#include "Func/MyUtils.h"
#include "Gui/Components/MyFileDialog.h"
#include "Gui/Dialogs/WizardDlg.h"
#include "Gui/GuiTools.h"

CVideoGrabberPage::CVideoGrabberPage(UploadEngineManager* uploadEngineManager)
{
	Terminated = true;
    grabbedFramesCount = 0;
    uploadEngineManager_ = uploadEngineManager;
    ThumbsView.SetDeletePhysicalFiles(true);
    NumOfFrames = 0;
    CanceledByUser = false;
    MainDlg = nullptr;
}

bool CVideoGrabberPage::SetFileName(LPCTSTR FileName)
{
    fileName_ = FileName;
    fileEdit_.SetWindowText(fileName_);
    return false;
}

LRESULT CVideoGrabberPage::OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
    WtlGuiSettings& Settings = *ServiceLocator::instance()->settings<WtlGuiSettings>();
    PageWnd = m_hWnd;
    DoDataExchange(FALSE);

    frameCountUpDownCtrl_.SetRange(1, 100);

    SetDlgItemInt(IDC_NUMOFFRAMESEDIT, Settings.VideoSettings.NumOfFrames);

    TRC(IDC_EXTRACTFRAMES, "Extracting frames from video");
    TRC(IDC_SELECTVIDEO, "Browse...");

    TRC(IDC_PATHTOFILELABEL, "Filename:");
    TRC(IDCANCEL, "Stop");
    TRC(IDC_FRAMELABEL, "Number of frames:");
    TRC(IDC_MULTIPLEFILES, "multiple files");
    TRC(IDC_SAVEASONE, "single file (mosaic)");
    TRC(IDC_SAVEAS, "Save as:");
    TRC(IDC_GRAB, "Grab");
    TRC(IDC_FILEINFOBUTTON, "Information about file");
    TRC(IDC_OPTIONSBUTTON, "Options");

    optionsButton_.SetButtonStyle(BS_SPLITBUTTON);

    videoEngines_ = CommonGuiSettings::VideoEngines;
    if (!CommonGuiSettings::IsFFmpegAvailable()) {
        videoEngines_.erase(std::remove(videoEngines_.begin(), videoEngines_.end(), CommonGuiSettings::VideoEngineFFmpeg), videoEngines_.end());
    }
    // Sort video engines alphabetically
    std::sort(videoEngines_.begin(), videoEngines_.end(), [](auto& l, auto& r) {
        int res = IuStringUtils::StrCaseInsensitiveCompare(l, r);
        return res < 0;
    });

    int i = 0;
    for (const auto& engine : videoEngines_) {
        if (Settings.VideoSettings.Engine == engine) {
            currentVideoEngine = i;
        }
        i++;
    }

    fileEdit_.SetWindowText(fileName_);

    bool check = true;
    SendDlgItemMessage(IDC_MULTIPLEFILES, BM_SETCHECK, check);
    SendDlgItemMessage(IDC_SAVEASONE, BM_SETCHECK, !check);

    ::ShowWindow(GetDlgItem(IDC_STOP), SW_HIDE);
    ::ShowWindow(GetDlgItem(IDC_PROGRESSBAR), SW_HIDE);
    SavingMethodChanged();
    ThumbsView.SubclassWindow(GetDlgItem(IDC_THUMBLIST));
    ThumbsView.Init();

    /*toolTipCtrl_ = GuiTools::CreateToolTipForWindow(GetDlgItem(IDC_VIDEOENGINECOMBO),
        TR("Video engine which is used to extract frames from a video file."));
    */
    return 1;  // Let the system set the focus
}

LRESULT CVideoGrabberPage::OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled)
{
    if (!Terminated) {

        if(videoGrabber_) {
            videoGrabber_->abort();
        }


        CanceledByUser = true;
    }

    return 0;
}

LRESULT CVideoGrabberPage::OnBnClickedGrab(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    WtlGuiSettings* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    CString fileName = GuiTools::GetDlgItemText(m_hWnd, IDC_FILEEDIT);
    if ( fileName.IsEmpty() ) {
        return 0;
    }

    if ( !WinUtils::FileExists(fileName ) ) {
        LOG(ERROR) << str(boost::wformat(TR("File doesn't exist:\n%s")) % fileName.GetString());
        return 0;
    }

    WizardDlg->setLastVideoFile(fileName);
    grabbedFramesCount = 0;
    Terminated = false;

    ::ShowWindow(GetDlgItem(IDC_FRAMELABEL), SW_HIDE);
    ::ShowWindow(GetDlgItem(IDC_NUMOFFRAMESEDIT), SW_HIDE);
    ::ShowWindow(GetDlgItem(IDC_UPDOWN), SW_HIDE);
    optionsButton_.ShowWindow(SW_HIDE);

    ::ShowWindow(GetDlgItem(IDCANCEL), SW_SHOW);

    ::EnableWindow(GetDlgItem(IDC_GRAB), 0);
    ::EnableWindow(GetDlgItem(IDC_FILEEDIT), 0);
    ::EnableWindow(GetDlgItem(IDC_SELECTVIDEO), 0);
    ::ShowWindow(GetDlgItem(IDC_PROGRESSBAR), SW_SHOW);

    SetNextCaption(TR("Next >"));
    EnableNext(false);
    EnablePrev(false);
    EnableExit(false);
    TRC(IDC_STOP, "Stop");

    NumOfFrames = GetDlgItemInt(IDC_NUMOFFRAMESEDIT);
    if (NumOfFrames < 1)
        NumOfFrames = 5;
    SendDlgItemMessage(IDC_PROGRESSBAR, PBM_SETPOS, 0);

    SendDlgItemMessage(IDC_PROGRESSBAR, PBM_SETRANGE, 0, MAKELPARAM(0, NumOfFrames*10));
    CanceledByUser = false;

    snapshotsFolder.Empty();

    GrabBitmaps(fileName);

    return 0;
}

bool CVideoGrabberPage::OnAddImage(Gdiplus::Bitmap *bm, CString title)
{
    using namespace Gdiplus;
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    CString fileNameBuffer;

    SetGrabbingStatusText(CString(TR("Extracting frame ")) + title);

    if (deinterlace_)
    {
        int iwidth = bm->GetWidth();
        int iheight = bm->GetHeight();
        int halfheight = iheight / 2;
        Graphics g(m_hWnd, true);
        Bitmap BackBuffer (iwidth,  halfheight, &g);
        Graphics gr(&BackBuffer);
        gr.SetInterpolationMode(InterpolationModeHighQualityBicubic );
        gr.DrawImage(bm, 0, 0, iwidth, halfheight);
        Graphics gr2(bm);
        gr2.SetInterpolationMode(InterpolationModeHighQualityBicubic );
        gr2.DrawImage(&BackBuffer, 0, 0, iwidth, iheight);
    }

    CString videoFile = GuiTools::GetDlgItemText(m_hWnd, IDC_FILEEDIT);

    if (snapshotsFolder.IsEmpty()) {
        if (!settings->VideoSettings.SnapshotsFolder.empty()) {
            CString path
                = U2W(settings->VideoSettings.SnapshotsFolder + "\\" + settings->VideoSettings.SnapshotFileTemplate);
            CString snapshotsFolderTemplate = Utf8ToWCstring(IuCoreUtils::ExtractFilePath(WCstringToUtf8(path)));
            snapshotsFolder = IuCommonFunctions::GenerateFileName(snapshotsFolderTemplate, 1,
                                                                  CPoint(bm->GetWidth(), bm->GetHeight()),
                                                                  time(nullptr), TR("Frame"), videoFile);
            std::string snapshotsFolderUtf8 = WCstringToUtf8(snapshotsFolder);

            if (!IuCoreUtils::DirectoryExists(snapshotsFolderUtf8)) {
                if (!IuCoreUtils::CreateDir(snapshotsFolderUtf8)) {
                    CString logMessage;
                    CString lastError = WinUtils::GetLastErrorAsString();
                    logMessage.Format(_T("Could not create folder '%s'.\r\n%s"), (LPCTSTR)snapshotsFolder,
                                      (LPCTSTR)lastError);
                    ServiceLocator::instance()->logger()->write(ILogger::logError, _T("Video Grabber"), logMessage, L"",
                                                                videoFile);
                    snapshotsFolder = AppRuntimeInfo::instance()->tempDirectoryW();
                }
            }
        }
        if (snapshotsFolder.IsEmpty()) {
            snapshotsFolder = AppRuntimeInfo::instance()->tempDirectoryW();
        }
    }

    CString wOutDir;
    if (IuCoreUtils::DirectoryExists(WCstringToUtf8(snapshotsFolder))) {
        wOutDir = snapshotsFolder;
    }
    CString snapshotFileTemplate = U2W(IuCoreUtils::ExtractFileNameNoExt(settings->VideoSettings.SnapshotFileTemplate));


    CString outFilename = IuCommonFunctions::GenerateFileName(snapshotFileTemplate, grabbedFramesCount + 1,
                                                              CPoint(bm->GetWidth(), bm->GetHeight()), time(nullptr),
                                                              TR("Frame"), videoFile);
    /*CString fullOutFileName = Settings.VideoSettings.SnapshotsFolder + "\\" + outFilename;
    std::string outDir = IuCoreUtils::ExtractFilePath(WCstringToUtf8(fullOutFileName));
    CString fileNameNoExt = Utf8ToWCstring(IuCoreUtils::ExtractFileNameNoExt(WCstringToUtf8(fullOutFileName)));*/

    try {
        if (ImageUtils::MySaveImage(bm, outFilename, fileNameBuffer, ImageUtils::sifPNG, 100, !wOutDir.IsEmpty() ? static_cast<LPCTSTR>(wOutDir) : NULL)) {
            ThumbsView.AddImage(fileNameBuffer, title, false, bm);
            grabbedFramesCount++;
        }
    }
    catch (const std::exception& ex) {
        LOG(ERROR) << "Failed to save image " << outFilename << std::endl << ex.what();
    }

    return true;
}

bool CVideoGrabberPage::SetGrabbingStatusText(LPCTSTR String)
{
    ErrorStr = String;
    SetDlgItemText(IDC_GRABINFOLABEL, String);
    return false;
}

int CVideoGrabberPage::ThreadTerminated()
{
    Terminated = true;

    ::EnableWindow(GetDlgItem(IDC_GRAB), 1);

    ::ShowWindow(GetDlgItem(IDC_STOP), SW_HIDE);
    ::ShowWindow(GetDlgItem(IDC_PROGRESSBAR), SW_HIDE);
    if (CanceledByUser)
        SetGrabbingStatusText(TR("Extracting video frames was stopped by user."));

    ::ShowWindow(GetDlgItem(IDC_FRAMELABEL), SW_SHOW);
    ::ShowWindow(GetDlgItem(IDC_NUMOFFRAMESEDIT), SW_SHOW);
    ::ShowWindow(GetDlgItem(IDC_UPDOWN), SW_SHOW);
    optionsButton_.ShowWindow(SW_SHOW);
    CheckEnableNext();
    ::EnableWindow(GetDlgItem(IDC_FILEEDIT), 1);
    ::EnableWindow(GetDlgItem(IDC_SELECTVIDEO), 1);

    EnablePrev();
    EnableExit();
    return 0;
}

LRESULT CVideoGrabberPage::OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled) {
    return 0;
}

LRESULT CVideoGrabberPage::OnBnClickedGrabberparams(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    CSettingsDlg dlg(CSettingsDlg::spVideo, uploadEngineManager_);
    dlg.DoModal(m_hWnd);
    return 0;
}

LRESULT CVideoGrabberPage::OnBnClickedMultiplefiles(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    SavingMethodChanged();
    return 0;
}

void CVideoGrabberPage::SavingMethodChanged(void)
{
    BOOL check = SendDlgItemMessage(IDC_MULTIPLEFILES, BM_GETCHECK);
    //::EnableWindow(GetDlgItem(IDC_GRABBERPARAMS), !check);
}

int CVideoGrabberPage::GenPicture(CString& outFileName)
{
	CMainDlg* mainDlg = WizardDlg->getPage<CMainDlg>(CWizardDlg::wpMainPage);
	TCHAR buf[256] = _T("\0");
	int n = ThumbsView.GetItemCount();
	std::vector<ImageGeneratorTask::FileItem> items;
	for (int i = 0; i < n; i++) {
		buf[0] = _T('\0');
		LPCTSTR fileName = ThumbsView.GetFileName(i);
		ThumbsView.GetItemText(i, 0, buf, 256);
		items.emplace_back(fileName, buf);
	}
	CString mediaFile = GuiTools::GetDlgItemText(m_hWnd, IDC_FILEEDIT);
	auto task = std::make_shared<ImageGeneratorTask>(items, ThumbsView.maxwidth, ThumbsView.maxheight, mediaFile);

    boost::signals2::scoped_connection taskFinishedConnection = task->onTaskFinished.connect([task, mainDlg](BackgroundTask*, BackgroundTaskResult taskResult) {
		if (taskResult == BackgroundTaskResult::Success && !task->outFileName().IsEmpty()) {
			mainDlg->AddToFileList(task->outFileName(), L"", true, nullptr, true);
		}
	});
    auto dlg = CStatusDlg::create(task);
	if (dlg->executeTask(m_hWnd, 0) == IDOK) {
		return 1;
	}
	return 0;
}

LRESULT CVideoGrabberPage::OnBnClickedBrowseButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    IMyFileDialog::FileFilterArray filters {
        { TR("Video files"), PrepareVideoDialogFilters()},
        { TR("All files"), _T("*.*") }
    };

    auto dlg = MyFileDialogFactory::createFileDialog(m_hWnd, settings->VideoFolder, TR("Choose video file"), filters, false);
    if (dlg->DoModal(m_hWnd) != IDOK) {
        return 0;
    }

    fileEdit_.SetWindowText(dlg->getFile());
    settings->VideoFolder = dlg->getFolderPath();

    return 0;
}

int CVideoGrabberPage::GrabBitmaps(const CString& szFile )
{
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    std::string videoEngine = settings->VideoSettings.Engine;

    if (videoEngine == WtlGuiSettings::VideoEngineAuto) {
        videoEngine = settings->IsFFmpegAvailable() ? WtlGuiSettings::VideoEngineFFmpeg : WtlGuiSettings::VideoEngineDirectshow2;
    }
    VideoGrabber::VideoEngine engine = VideoGrabber::veAuto;
#ifdef IU_ENABLE_FFMPEG
    if (videoEngine == WtlGuiSettings::VideoEngineFFmpeg) {
        engine = VideoGrabber::veAvcodec;
    } else
#endif
    if (videoEngine == WtlGuiSettings::VideoEngineDirectshow) {
        engine = VideoGrabber::veDirectShow;
    } else if (videoEngine == WtlGuiSettings::VideoEngineDirectshow2) {
        engine = VideoGrabber::veDirectShow2;
    } else if (videoEngine == WtlGuiSettings::VideoEngineMediaFoundation) {
        engine = VideoGrabber::veMediaFoundation;
    }

    settings->VideoSettings.NumOfFrames = NumOfFrames;

    videoGrabber_ = std::make_unique<VideoGrabber>();
    videoGrabber_->setFrameCount(NumOfFrames);
    using namespace std::placeholders;
    videoGrabber_->setOnFrameGrabbed(std::bind(&CVideoGrabberPage::OnFrameGrabbed, this, _1, _2, _3));
    videoGrabber_->setOnFinished(std::bind(&CVideoGrabberPage::OnFrameGrabbingFinished, this, _1));

    videoGrabber_->setVideoEngine(engine);
    videoGrabber_->grab(W2U(szFile));
    return 0;
}

bool CVideoGrabberPage::OnShow()
{
    SetNextCaption(TR("Grab"));
    fileEdit_.SetWindowText(fileName_);
#ifdef IU_ENABLE_MEDIAINFO
    ::ShowWindow(GetDlgItem(IDC_FILEINFOBUTTON), SW_SHOW);
#endif
    SetGrabbingStatusText(_T(""));
    EnableNext(true);
    ShowPrev();
    ShowNext();
    EnablePrev();
    EnableExit();
    ThumbsView.MyDeleteAllItems();
    ::SetFocus(GetDlgItem(IDC_GRAB));
    return true;
}

void CVideoGrabberPage::OnFrameGrabbed(const std::string& timeStr, int64_t, std::shared_ptr<AbstractImage> img )
{
    auto image = std::dynamic_pointer_cast<GdiPlusImage>(img);
    if ( !image ) {
        LOG(ERROR) << "image is null";
        return;
    }
    Gdiplus::Bitmap *bm = image->getBitmap();
    if ( bm ) {
        SendDlgItemMessage(IDC_PROGRESSBAR, PBM_SETPOS, (grabbedFramesCount + 1) * 10 );
        OnAddImage(bm, Utf8ToWCstring(timeStr));
    }
}

void CVideoGrabberPage::OnFrameGrabbingFinished(bool success)
{
    ThreadTerminated();

    if (!CanceledByUser) {
        SetGrabbingStatusText(success ? TR("Extracting video frames was finished."): TR("An error occured while extracting video frames."));
    }
}

void CVideoGrabberPage::optionsButtonClicked() {
    RECT rc {};
    optionsButton_.GetWindowRect(&rc);
    POINT menuOrigin { rc.left, rc.bottom };

    CMenu videoEngineMenu;
    videoEngineMenu.CreatePopupMenu();
    int i = 0;
    for (const auto& engine : videoEngines_) {
        videoEngineMenu.AppendMenu(
            MF_STRING | MFT_RADIOCHECK | (currentVideoEngine == i ? MF_CHECKED : MF_UNCHECKED),
            ID_VIDEOENGINEFIRST + i, U2W(engine)
        );
        i++;
    }

    CMenu popupMenu;
    popupMenu.CreatePopupMenu();
    popupMenu.AppendMenu(MF_STRING, videoEngineMenu, TR("Video backend"));
    popupMenu.AppendMenu(MF_STRING | (deinterlace_ ? MF_CHECKED : MF_UNCHECKED), ID_DEINTERLACE, TR("Deinterlace"));
    popupMenu.AppendMenu(MF_STRING, ID_VIDEOSETTINGS, TR("Additional options..."));
    popupMenu.AppendMenu(MF_SEPARATOR);
    bool enableOpenFolderMenuItem = IuCoreUtils::DirectoryExists(W2U(snapshotsFolder));
    popupMenu.AppendMenu(MF_STRING | (enableOpenFolderMenuItem ? MF_ENABLED : MF_DISABLED), ID_OPENFOLDER, TR("Open containing folder"));

    TPMPARAMS excludeArea;
    ZeroMemory(&excludeArea, sizeof(excludeArea));
    excludeArea.cbSize = sizeof(excludeArea);
    excludeArea.rcExclude = rc;
    popupMenu.TrackPopupMenuEx(TPM_LEFTALIGN | TPM_LEFTBUTTON, menuOrigin.x, menuOrigin.y, m_hWnd, &excludeArea);
}

void CVideoGrabberPage::CheckEnableNext()
{
    EnableNext(ThumbsView.GetItemCount() > 1);
}

bool CVideoGrabberPage::OnNext()
{
    int n = ThumbsView.GetItemCount();
    if (n < 1)
    {
        SendDlgItemMessage(IDC_GRAB, BM_CLICK);
        return false;
    }

    WizardDlg->CreatePage(CWizardDlg::wpMainPage);
    CMainDlg* mainDlg = WizardDlg->getPage<CMainDlg>(CWizardDlg::wpMainPage);

    BOOL check = SendDlgItemMessage(IDC_MULTIPLEFILES, BM_GETCHECK);

    if (check) // If option "Multiple files" turn on
    {
        for (int i = 0; i < n; i++)
        {
            LPCTSTR filename = ThumbsView.GetFileName(i);
            if (!filename)
                continue;
            mainDlg->AddToFileList(filename);
        }
    }
    else
    {
        CString outFileName;
        if (!GenPicture(outFileName)) {
            return false;
        }
    }

    // Reset selection in the thumbs view
    mainDlg->ThumbsView.SelectItem(-1);

    ThumbsView.MyDeleteAllItems();
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    settings->VideoSettings.NumOfFrames = GetDlgItemInt(IDC_NUMOFFRAMESEDIT);

    return true;
}

LRESULT CVideoGrabberPage::OnLvnItemDelete(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& bHandled)
{
    bHandled = FALSE;
    CheckEnableNext();
    return 0;
}

LRESULT CVideoGrabberPage::OnBnClickedFileinfobutton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL& /*bHandled*/)
{
#ifdef IU_ENABLE_MEDIAINFO
    CMediaInfoDlg dlg;
    CString fileName;
    fileEdit_.GetWindowText(fileName);
    dlg.ShowInfo(m_hWnd, fileName);
#endif
    return 0;
}

LRESULT CVideoGrabberPage::OnOpenFolder(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/) {
    WinUtils::ShellOpenFileOrUrl(snapshotsFolder, m_hWnd);
    return 0;
}

LRESULT CVideoGrabberPage::OnBnClickedOptions(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    optionsButtonClicked();
    return 0;
}

LRESULT CVideoGrabberPage::OnBnDropdownOptions(int idCtrl, LPNMHDR pnmh, BOOL& bHandled) {
    optionsButtonClicked();
    return 0;
}

LRESULT CVideoGrabberPage::OnDeinterlace(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    deinterlace_ = !deinterlace_;
    return 0;
}

LRESULT CVideoGrabberPage::OnMenuVideoEngine(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled) {
    int index = wID - ID_VIDEOENGINEFIRST;
    if (index < 0 || index >= videoEngines_.size()) {
        return 0;
    }
    currentVideoEngine = index;
    auto* settings = ServiceLocator::instance()->settings<WtlGuiSettings>();
    settings->VideoSettings.Engine = videoEngines_[index];
    return 0;
}
