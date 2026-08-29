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
#ifndef IU_GUI_DIALOGS_VIDEOGRABBERPAGE_H
#define IU_GUI_DIALOGS_VIDEOGRABBERPAGE_H

#include <random>
#include "atlheaders.h"
#include "resource.h"       // main symbols
#include "Gui/WizardCommon.h"
#include "Gui/Controls/ThumbsView.h"

class AbstractImage;

class VideoGrabber;
class CVideoGrabberPage;
class UploadEngineManager;
class CMainDlg;

struct SENDPARAMS
{
	BYTE* pBuffer;
	CString szTitle;
	BITMAPINFO bi;
	long BufSize;
	CVideoGrabberPage* vg;
};

class CVideoGrabberPage : public CWizardPage, public CDialogImpl<CVideoGrabberPage>, public CWinDataExchange <CVideoGrabberPage>
{
	public:
		CVideoGrabberPage(UploadEngineManager * uploadEngineManager);
		enum { IDD = IDD_VIDEOGRABBER, ID_DEINTERLACE = 18000, ID_VIDEOSETTINGS, ID_OPENFOLDER, ID_VIDEOENGINEFIRST = 18100, ID_VIDEOENNGINELAST = 18131 };

	protected:
		BEGIN_MSG_MAP(CVideoGrabberPage)
			MESSAGE_HANDLER(WM_INITDIALOG, OnInitDialog)
			MESSAGE_HANDLER(WM_DESTROY, OnDestroy)
			COMMAND_HANDLER(IDCANCEL, BN_CLICKED, OnClickedCancel)
			COMMAND_HANDLER(IDC_GRAB, BN_CLICKED, OnBnClickedGrab)
            COMMAND_HANDLER(ID_VIDEOSETTINGS, BN_CLICKED, OnBnClickedGrabberparams)
			COMMAND_HANDLER(IDC_MULTIPLEFILES, BN_CLICKED, OnBnClickedMultiplefiles)
			COMMAND_HANDLER(IDC_SAVEASONE, BN_CLICKED, OnBnClickedMultiplefiles)
			COMMAND_HANDLER(IDC_SELECTVIDEO, BN_CLICKED, OnBnClickedBrowseButton)
			COMMAND_ID_HANDLER(ID_OPENFOLDER, OnOpenFolder)
            COMMAND_ID_HANDLER(ID_DEINTERLACE, OnDeinterlace)
			NOTIFY_HANDLER(IDC_THUMBLIST, LVN_DELETEITEM, OnLvnItemDelete)
			COMMAND_HANDLER(IDC_FILEINFOBUTTON, BN_CLICKED, OnBnClickedFileinfobutton)
            COMMAND_HANDLER(IDC_OPTIONSBUTTON, BN_CLICKED, OnBnClickedOptions)
            NOTIFY_HANDLER(IDC_OPTIONSBUTTON, BCN_DROPDOWN, OnBnDropdownOptions)
            COMMAND_RANGE_HANDLER(ID_VIDEOENGINEFIRST, ID_VIDEOENNGINELAST, OnMenuVideoEngine)
			REFLECT_NOTIFICATIONS()
		END_MSG_MAP()

        BEGIN_DDX_MAP(CVideoGrabberPage)
            DDX_CONTROL_HANDLE(IDC_FILEEDIT, fileEdit_)
            DDX_CONTROL_HANDLE(IDC_UPDOWN, frameCountUpDownCtrl_)
            DDX_CONTROL_HANDLE(IDC_OPTIONSBUTTON, optionsButton_)
        END_DDX_MAP()

		// Handler prototypes:
		//  LRESULT MessageHandler(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
		//  LRESULT CommandHandler(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
		//  LRESULT NotifyHandler(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
		LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
		LRESULT OnClickedCancel(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnDestroy(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);

		LRESULT OnLvnItemDelete(int /*idCtrl*/, LPNMHDR pNMHDR, BOOL& /*bHandled*/);
		LRESULT OnBnClickedGrab(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/);
		LRESULT OnBnClickedGrabberparams(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/);
		LRESULT OnBnClickedMultiplefiles(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/);
		LRESULT OnBnClickedFileinfobutton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/);
		LRESULT OnBnClickedBrowseButton(WORD /*wNotifyCode*/, WORD /*wID*/, HWND /*hWndCtl*/, BOOL & /*bHandled*/);
		LRESULT OnOpenFolder(WORD /*wNotifyCode*/, WORD wID, HWND /*hWndCtl*/, BOOL& /*bHandled*/);
        LRESULT OnBnClickedOptions(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnBnDropdownOptions(int idCtrl, LPNMHDR pnmh, BOOL& bHandled);
        LRESULT OnDeinterlace(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
        LRESULT OnMenuVideoEngine(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

        int GrabBitmaps(const CString& szFile);
        bool SetGrabbingStatusText(LPCTSTR String);
        int ThreadTerminated();
        bool OnAddImage(Gdiplus::Bitmap* bm, CString title);
        void SavingMethodChanged(void);
        int GenPicture(CString& outFileName);
        CThumbsView ThumbsView;
        void CheckEnableNext();
        bool OnNext() override; // Reimplemented function of CWizardPage
        bool OnShow() override; // Reimplemented function of CWizardPage
        void OnFrameGrabbed(const std::string&, int64_t, std::shared_ptr<AbstractImage>);
        void OnFrameGrabbingFinished(bool success);
        void optionsButtonClicked();
		std::unique_ptr<VideoGrabber> videoGrabber_ ;

//		CImgSavingThread SavingThread;
		CString ErrorStr;
		CString snapshotsFolder;
		bool Terminated;
		int grabbedFramesCount;
		int NumOfFrames;
        bool deinterlace_ = false;
        int currentVideoEngine = 0;
		bool SetFileName(LPCTSTR FileName);
		CString fileName_;
		bool CanceledByUser;
		CMainDlg* MainDlg;
        CEdit fileEdit_;
		UploadEngineManager * uploadEngineManager_;
        CToolTipCtrl toolTipCtrl_;
		CUpDownCtrl frameCountUpDownCtrl_;
        CButton optionsButton_;
        std::vector<std::string> videoEngines_;
};

#endif // VIDEOGRABBER_H
