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
// IShellContextMenu.cpp : Implementation of CIShellContextMenu

#include <atlbase.h>
#include <atlcoll.h>
#include <shlobj.h>
#include "IShellContextMenu.h"
#include "../3rdpart/Registry.h"
#include "Helpers.h"
#include "../Core/BasicConstants.h"

HINSTANCE hDllInstance;

CString GetStartMenuPath()
{
	CString result;
	LPITEMIDLIST pidl;
	TCHAR        szSendtoPath [MAX_PATH];
	LPMALLOC     pMalloc;

	if(SUCCEEDED( SHGetSpecialFolderLocation ( NULL, CSIDL_STARTMENU, &pidl )))
	{
		if(SHGetPathFromIDList(pidl, szSendtoPath))
		{
			result = szSendtoPath;
		}

		if(SUCCEEDED(SHGetMalloc(&pMalloc)))
		{
			pMalloc->Free ( pidl );
			pMalloc->Release();
		}
	}
	return result;
}

CString GetDllFolder()
{
	TCHAR szFileName[1024],szPath[256];
	GetModuleFileName(hDllInstance, szFileName, 1023);
	Helpers::ExtractFilePath(szFileName, szPath, 256);
	return szPath;
}

bool AreOnlyImages(const CAtlArray<CString> & files)
{
	if(files.GetCount()>1000) return false;

	for(int i=0; i<files.GetCount();i++)
	{
		if(!Helpers::IsImage(files[i])) return false;
	}
	return true;
}

bool CIShellContextMenu::MyInsertMenu(HMENU hMenu, int pos, UINT id, int nInternalCommand, const LPCTSTR szTitle, int firstCmd, CString commandArgument, bool UseBitmaps, HBITMAP bm,WORD resid,HICON ico)
{
	MENUITEMINFO MenuItem;

	MenuItem.cbSize = sizeof(MenuItem);
	MenuItem.fType = MFT_STRING;
	if ( ico ) {
		MenuItem.hbmpItem = IsWindowsVistaOrGreater() ? GetCachedHIconToBitmapPARGB32(ico): HBMMENU_CALLBACK;
	} else {
        MenuItem.hbmpItem = IsWindowsVistaOrGreater() ? GetCachedIconToBitmapPARGB32(resid) : HBMMENU_CALLBACK;
	}

	MenuItem.fMask = MIIM_FTYPE | MIIM_ID | (UseBitmaps?MIIM_BITMAP:0)  | MIIM_STRING;

	MenuItem.wID = id;
	MenuItem.dwTypeData = (LPWSTR)szTitle;
	if(!InsertMenuItem(hMenu, pos, TRUE, &MenuItem))
		return false;

	Shell_ContextMenuItem InternalMenuItem;
	InternalMenuItem.cmd =  nInternalCommand;
	InternalMenuItem.text = szTitle;
	InternalMenuItem.commandArgument = commandArgument;
	//if(resid)
	InternalMenuItem.icon = resid;
	InternalMenuItem.id = id;
	m_nCommands[id-firstCmd]= InternalMenuItem;
	return true;
}

bool CIShellContextMenu::MyInsertMenuSeparator(HMENU hMenu, int pos, UINT id)
{
	MENUITEMINFO MenuItem;
	ZeroMemory(&MenuItem, sizeof(MenuItem));
	MenuItem.cbSize = sizeof(MenuItem);
	MenuItem.fType = MFT_SEPARATOR;

	MenuItem.fMask = MIIM_FTYPE | MIIM_ID;

	MenuItem.wID = id;
	if(!InsertMenuItem(hMenu, pos, TRUE, &MenuItem))
		return false;

	return true;
}

STDMETHODIMP CIShellContextMenu::HandleMenuMsg(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	LRESULT res;
	return HandleMenuMsg2(uMsg, wParam, lParam, &res);
}

STDMETHODIMP CIShellContextMenu::HandleMenuMsg2(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult)
{
	LRESULT res;
	if (pResult == NULL)
		pResult = &res;
	*pResult = FALSE;

	switch (uMsg)
	{
		case WM_MEASUREITEM:
		{
			MEASUREITEMSTRUCT* lpmis = reinterpret_cast<MEASUREITEMSTRUCT*>(lParam);
			if (lpmis==NULL)
				break;
			lpmis->itemWidth += 2;
			if (lpmis->itemHeight < 16)
				lpmis->itemHeight = 16;
			*pResult = TRUE;
		}
		break;
	case WM_DRAWITEM:
		{
			HICON hIcon=0;
			DRAWITEMSTRUCT* lpdis = reinterpret_cast<DRAWITEMSTRUCT*>(lParam);
			if ((lpdis==NULL)||(lpdis->CtlType != ODT_MENU))
				return S_OK;		//not for a menu
			int i =0;
			int iconID=0;

			std::map<int, Shell_ContextMenuItem>::iterator it;
			for(it=m_nCommands.begin(); it!=m_nCommands.end();++it)
			{
				if(it->second.id == lpdis->itemID)
				{
					iconID = it->second.icon;
				}
			}

			int w = GetSystemMetrics(SM_CXSMICON);
			int h = GetSystemMetrics(SM_CYSMICON);
			if ( w > 16 ) {
				w = 32;
				h = 32;
			}
			if(iconID) {
				hIcon = (HICON)LoadImage(hDllInstance, MAKEINTRESOURCE(iconID), IMAGE_ICON, w, h, LR_DEFAULTCOLOR);
			}
			else hIcon = NULL;
			if (hIcon == NULL)
				return S_OK;

			DrawIconEx(lpdis->hDC,
				lpdis->rcItem.left - 16,
				lpdis->rcItem.top + (lpdis->rcItem.bottom - lpdis->rcItem.top - h) / 2,
				hIcon, w, h,
				0, NULL, DI_NORMAL);
			*pResult = TRUE;
			DeleteObject(hIcon);
		}
		break;
	default:
		return S_OK;
	}

	return S_OK;
}

// CIShellContextMenu
HRESULT CIShellContextMenu::QueryContextMenu(HMENU hmenu, UINT indexMenu, UINT idCmdFirst, UINT idCmdLast, UINT uFlags)
{
    bool ExplorerCascadedMenu = true;
    // #ifdef _WIN64
    bool ExplorerContextMenu = true;
    //#else
    //bool ExplorerContextMenu= false;
    //#endif
    bool ExplorerVideoContextMenu = true;
    CRegistry Reg;
    Reg.SetRootKey(HKEY_CURRENT_USER);
    if (Reg.SetKey(_T("Software\\Uptooda"), false)) {
        ExplorerCascadedMenu = Reg.ReadBool(_T("ExplorerCascadedMenu"), true);
        //	ExplorerContextMenu = Reg.ReadBool("ExplorerContextMenu");
        ExplorerVideoContextMenu = Reg.ReadBool(_T("ExplorerVideoContextMenu"), true);
        /*CString lang = Reg.ReadString(_T("Language"));
		//MessageBox(0, lang,0,0);
		if(lang != Lang.GetLanguageName())
		{
			Lang.LoadLanguage(lang);
		}*/
    }
#ifdef _WIN64
    else {
        Reg.SetWOW64Flag(KEY_WOW64_32KEY);
        if (Reg.SetKey(_T("Software\\Uptooda"), false)) {
            ExplorerCascadedMenu = Reg.ReadBool(_T("ExplorerCascadedMenu"));
            //	ExplorerContextMenu = Reg.ReadBool("ExplorerContextMenu");
            ExplorerVideoContextMenu = Reg.ReadBool(_T("ExplorerVideoContextMenu"));
            CString lang = Reg.ReadString(_T("Language"));
            //MessageBox(0, lang,0,0);
            /*if (lang != Lang.GetLanguageName())
				{
					Lang.LoadLanguage(lang);
				}*/
        }
    }
#endif

    UINT currentCommandID = idCmdFirst;
    if ((uFlags & 0x000F) != CMF_NORMAL && (uFlags & CMF_VERBSONLY) == 0 && (uFlags & CMF_EXPLORE) == 0)
        return MAKE_HRESULT(SEVERITY_SUCCESS, 0, currentCommandID);

    m_nCommands.clear(); // Clearing internal map of commands

    HMENU PopupMenu;
    bool UseBitmaps = true;
    CString StartMenuFolder = GetStartMenuPath();
    bool isDirectory = false;

    if (m_FileList.GetCount() == 1) {
        DWORD atrr = GetFileAttributes(m_FileList[0]);
        isDirectory = atrr & FILE_ATTRIBUTE_DIRECTORY;

        if (!lstrcmpi(m_FileList[0], StartMenuFolder)) {
            UseBitmaps = false;
        }
    }
	UINT subIndex = indexMenu;
	if(ExplorerCascadedMenu)
		PopupMenu = CreatePopupMenu();
	else PopupMenu = hmenu;

	if ( !ExplorerCascadedMenu ) {
		MyInsertMenuSeparator(PopupMenu, subIndex++, 0);
	}
	if(AreOnlyImages(m_FileList))
		MyInsertMenu(PopupMenu, subIndex++, currentCommandID++, MENUITEM_UPLOADONLYIMAGES, stringsReader_.getString(_T("UploadImages"), _T("Upload images")), idCmdFirst, CString(), UseBitmaps, 0, IDI_ICONUPLOAD);
	else
	{
		MyInsertMenu(PopupMenu, subIndex++, currentCommandID++, MENUITEM_UPLOADFILES, stringsReader_.getString(_T("UploadFiles"), _T("Upload files")), idCmdFirst, CString(), UseBitmaps, 0, IDI_ICONUPLOAD);
		MyInsertMenu(PopupMenu, subIndex++, currentCommandID++, MENUITEM_UPLOADONLYIMAGES, stringsReader_.getString(_T("UploadImagesOnly"), _T("Upload images only")), idCmdFirst, CString(), UseBitmaps, 0, IDI_ICONUPLOAD);
	}
	bool separatorInserted = false;

	CRegistry Reg2;
	Reg2.SetRootKey( HKEY_CURRENT_USER );
	std::vector<CString> keyNames;
	CString keyPath = _T("Software\\Uptooda\\ContextMenuItems");
	Reg2.GetChildKeysNames(keyPath,keyNames);
	int w = GetSystemMetrics(SM_CXSMICON);
	int h = GetSystemMetrics(SM_CYSMICON);
	CString dataFolder = Helpers::FindDataFolder();
	for(int i =0; i < keyNames.size() ; i++ ) {
		if ( Reg2.SetKey(keyPath + _T("\\") + keyNames[i], false) ) {

				if ( ! separatorInserted && ExplorerCascadedMenu ) {
					MyInsertMenuSeparator(PopupMenu, subIndex++, 0);
					separatorInserted = true;
			}
				CString title = Reg2.ReadString(_T("Name"));
				CString iconFileName = Reg2.ReadString(_T("Icon"));
				HICON ico = nullptr;
				if ( !iconFileName.IsEmpty() ) {
                    ico = GetCachedServerIcon(dataFolder + L"\\Favicons\\" + iconFileName, w, h);
				}
				MyInsertMenu(PopupMenu, subIndex++, currentCommandID++, MENUITEM_SERVER_PROFILE,title,idCmdFirst,keyNames[i],UseBitmaps,0, ico ? 0: IDI_ICONUPLOAD, ico ? ico : nullptr);
		}
	}

	if (ExplorerVideoContextMenu && m_FileList.GetCount() == 1 && !isDirectory){
        bool isVideoFile = Helpers::IsVideoFile(m_FileList[0]);
        bool IsAudioFile = Helpers::IsAudioFile(m_FileList[0]);
        if (isVideoFile) {
            MyInsertMenu(PopupMenu, subIndex++, currentCommandID++, MENUITEM_IMPORTVIDEO, stringsReader_.getString(_T("ImportVideoFile"), _T("Import Video File")), idCmdFirst, CString(), UseBitmaps, 0, ExplorerCascadedMenu ? 0 : IDI_ICONMOVIE);
        }
        if (m_bMediaInfoInstalled && (isVideoFile || IsAudioFile)) {
            MyInsertMenu(PopupMenu, subIndex++, currentCommandID++, MENUITEM_MEDIAINFO, stringsReader_.getString(_T("InformationAboutFile"), _T("Information about file")), idCmdFirst, CString(), UseBitmaps, 0, ExplorerCascadedMenu ? 0 : IDI_ICONINFO);
        }
    }
	if ( !ExplorerCascadedMenu ) {
		MyInsertMenuSeparator(PopupMenu, subIndex++, 0);
		separatorInserted = true;
	}
	if(ExplorerCascadedMenu)
	{
		MENUITEMINFO MenuItem;
		MenuItem.cbSize = sizeof(MenuItem);
		//MenuItem.hbmpChecked = bmIULogo;
		//MenuItem.hbmpUnchecked =  bmIULogo;
		MenuItem.fType = MFT_STRING;
		MenuItem.fMask =MIIM_SUBMENU| MIIM_FTYPE | MIIM_ID | (UseBitmaps?MIIM_BITMAP:0) | MIIM_STRING;//MIIM_SUBMENU | MIIM_TYPE | MIIM_DATA|MIIM_ID|MIIM_BITMAP;//|MIIM_CHECKMARKS;
		MenuItem.wID = currentCommandID++;
		MenuItem.dwTypeData = const_cast<LPWSTR>(APP_NAME);
		MenuItem.hSubMenu = PopupMenu;

		// Inserting item in our internal list
		Shell_ContextMenuItem InternalMenuItem;
		InternalMenuItem.cmd =  -1;
		InternalMenuItem.text = MenuItem.dwTypeData;

		InternalMenuItem.icon= IDI_ICONMAIN;
		MenuItem.hbmpItem = IsWindowsVistaOrGreater() ? GetCachedIconToBitmapPARGB32(IDI_ICONMAIN): HBMMENU_CALLBACK;

		InternalMenuItem.id = MenuItem.wID;
		if(InsertMenuItem(hmenu, indexMenu, true, &MenuItem))
			m_nCommands[InternalMenuItem.id -idCmdFirst]= InternalMenuItem;

	}

	if (IsWindowsVistaOrGreater())
	{
		MENUINFO MenuInfo;

		memset(&MenuInfo, 0, sizeof(MenuInfo));

		MenuInfo.cbSize  = sizeof(MenuInfo);
		MenuInfo.fMask   = MIM_STYLE | MIM_APPLYTOSUBMENUS;
		MenuInfo.dwStyle = MNS_CHECKORBMP;

		SetMenuInfo(hmenu, &MenuInfo);
	}
	return MAKE_HRESULT(SEVERITY_SUCCESS, 0, currentCommandID - idCmdFirst);
}

bool IULaunchCopy(CAtlArray<CString> & CmdLine,const CString params=_T(""))
{
	STARTUPINFO si;
	PROCESS_INFORMATION pi;

	ZeroMemory(&si, sizeof(si));
   si.cb = sizeof(si);
   ZeroMemory(&pi, sizeof(pi));

	CString TempCmdLine = CString(_T("\""))+GetDllFolder()+_T("uptooda.exe")+CString(_T("\""));
	if(!params.IsEmpty()) TempCmdLine+=_T(" ")+params+_T(" ");
	for(int i=0;i <CmdLine.GetCount(); i++)
		{
			if(!lstrcmpi(CmdLine[i], _T("-Embedding"))) continue;
			TempCmdLine = TempCmdLine + _T(" \"") + CmdLine[i] + _T("\"");
		}

    // Start the child process.
    if( !CreateProcess(
		NULL,                   // No module name (use command line).
        (LPWSTR)(LPCTSTR)TempCmdLine, // Command line.
        NULL,                   // Process handle not inheritable.
        NULL,                   // Thread handle not inheritable.
        FALSE,                  // Set handle inheritance to FALSE.
        0,                      // No creation flags.
        NULL,                   // Use parent's environment block.
        NULL,                   // Use parent's starting directory.
        &si,                    // Pointer to STARTUPINFO structure.
        &pi )                   // Pointer to PROCESS_INFORMATION structure.
		) {
			CString errorMessage;
			errorMessage.Format(_T("Unable to start process '%s'"),(LPCTSTR)TempCmdLine);
		MessageBox(0, errorMessage, _T("Error"),0);
        return false;
	}

    // Close process and thread handles.
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
	return true;
}

HRESULT CIShellContextMenu::InvokeCommand(LPCMINVOKECOMMANDINFO lpici)
{
	if(HIWORD( lpici->lpVerb ))
		return E_INVALIDARG ;

	if(m_nCommands.empty())
		return E_INVALIDARG;
	Shell_ContextMenuItem item = m_nCommands[ LOWORD( lpici->lpVerb )];
	switch ( item.cmd)
	{
	case MENUITEM_UPLOADFILES:
		{
			IULaunchCopy(m_FileList,_T("/fromcontextmenu /upload"));

			return S_OK;
		}
		break;
	case MENUITEM_UPLOADONLYIMAGES:
		{
			IULaunchCopy(m_FileList,_T("/fromcontextmenu /imagesonly /upload"));

			return S_OK;
		}
		break;
	case MENUITEM_IMPORTVIDEO:
		{
            IULaunchCopy(m_FileList, _T("/importvideo"));

			return S_OK;
		}
		break;
	case  MENUITEM_MEDIAINFO:
		{
			IULaunchCopy(m_FileList,_T("/mediainfo"));

			return S_OK;
		}
		break;
	case MENUITEM_SERVER_PROFILE:
		IULaunchCopy(m_FileList,_T("/upload /quick /serverprofile=") +item.commandArgument );
		return S_OK;
		break;

	default:
		return E_INVALIDARG;
		break;

	}
	return S_OK;
}


HRESULT CIShellContextMenu::GetCommandString(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax)
{
	return S_OK;
}

CIShellContextMenu::CIShellContextMenu()
{
	m_bMediaInfoInstalled = true;
}

CIShellContextMenu::~CIShellContextMenu() {
    for (auto& it: cachedServerIcons_) {
        DestroyIcon(it.second.icon);
    }

    for (auto& it: cachedBitmaps_) {
        DeleteObject(it.second);
    }

    for (auto& it : cachedIconBitmaps_) {
        DeleteObject(it.second);
    }
}

HRESULT CIShellContextMenu::Initialize(LPCITEMIDLIST pidlFolder, LPDATAOBJECT dataObject, HKEY hkeyProgID)
{
	FORMATETC fmt = { CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL };
	STGMEDIUM stg = { TYMED_HGLOBAL };
	HDROP     hDrop;

	// Look for CF_HDROP data in the data object.
	if ( FAILED(dataObject->GetData ( &fmt, &stg )))
	{
		// Nope! Return an "invalid argument" error back to Explorer.
		return E_INVALIDARG;
	}

	// Get a pointer to the actual data.
	hDrop = (HDROP) GlobalLock ( stg.hGlobal );

	// Make sure it worked.
	if ( NULL == hDrop )
	{
		return E_INVALIDARG;
	}

	// Sanity check - make sure there is at least one filename.
	UINT uNumFiles = DragQueryFile ( hDrop, 0xFFFFFFFF, NULL, 0 );

	if ( 0 == uNumFiles )
	{
		GlobalUnlock ( stg.hGlobal );
		ReleaseStgMedium ( &stg );
		return E_INVALIDARG;
	}

	HRESULT hr = S_OK;
	TCHAR FileName[MAX_PATH+1];
	int i =0;

	while( DragQueryFile ( hDrop, i++, FileName, MAX_PATH ))
	{

		m_FileList.Add(FileName);
		// hr = E_INVALIDARG;
	}

	GlobalUnlock ( stg.hGlobal );
	ReleaseStgMedium ( &stg );


	return hr;
}

HICON CIShellContextMenu::GetCachedServerIcon(const CString& fileName, int w, int h) {
    auto it = cachedServerIcons_.find(fileName);
    if (it != cachedServerIcons_.end()) {
        if (it->second.icon && (it->second.w != w || it->second.h !=h)) {
            DestroyIcon(it->second.icon);
            cachedServerIcons_.erase(it);
        } else {
            return it->second.icon;
        }
    }
    auto icon = reinterpret_cast<HICON>(LoadImage(nullptr, fileName, IMAGE_ICON, w, h, LR_LOADFROMFILE));
    IconCacheItem& iconCacheItem = cachedServerIcons_[fileName];
    iconCacheItem.icon = icon;
    iconCacheItem.w = w;
    iconCacheItem.h = h;
    return icon;
}

HBITMAP CIShellContextMenu::GetCachedIconToBitmapPARGB32(UINT uIcon) {
    auto it = cachedBitmaps_.find(uIcon);
    if (it != cachedBitmaps_.end()) {
        return it->second;
    }
    HBITMAP bmp = m_IconBitmapUtils.IconToBitmapPARGB32(hDllInstance, uIcon);
    cachedBitmaps_[uIcon] = bmp;
    return bmp;
}

HBITMAP CIShellContextMenu::GetCachedHIconToBitmapPARGB32(HICON hIcon) {
    auto it = cachedIconBitmaps_.find(hIcon);
    if (it != cachedIconBitmaps_.end()) {
        return it->second;
    }
    HBITMAP bmp = m_IconBitmapUtils.HIconToBitmapPARGB32(hIcon);
    cachedIconBitmaps_[hIcon] = bmp;
    return bmp;

}


