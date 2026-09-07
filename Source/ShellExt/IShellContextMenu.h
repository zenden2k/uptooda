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
// IShellContextMenu.h : Declaration of the CIShellContextMenu

#pragma once
#include "resource.h"       // main symbols
#include "atlheaders.h"
#include "Generated/ExplorerIntegration.h"
#include "IconBitmapUtils.h"
#include <shobjidl.h>
#include <map>
#include <unordered_map>
#include <3rdpart/Registry.h>

#if defined(_WIN32_WCE) && !defined(_CE_DCOM) && !defined(_CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA)
#error "Single-threaded COM objects are not properly supported on Windows CE platform, such as the Windows Mobile platforms that do not include full DCOM support. Define _CE_ALLOW_SINGLE_THREADED_OBJECTS_IN_MTA to force ATL to support creating single-thread COM object's and allow use of it's single-threaded COM object implementations. The threading model in your rgs file was set to 'Free' as that is the only threading model supported in non DCOM Windows CE platforms."
#endif

extern HINSTANCE hDllInstance;

// CIShellContextMenu

struct Shell_ContextMenuItem
{
    int cmd;
    CString text;
    CString commandArgument;
    WORD icon;
    int id;
};
#define MENUITEM_UPLOADFILES 0
#define MENUITEM_UPLOADONLYIMAGES 1
#define MENUITEM_IMPORTVIDEO 2
#define MENUITEM_MEDIAINFO 3
#define MENUITEM_SERVER_PROFILE 4
#define MENUITEM_SERVER_PROFILE_GROUP 5

CString GetDllFolder();

class StringsReader
{
    CRegistry reg;
    std::map<CString, CString> map_;
public:
    StringsReader() {
        reg.SetRootKey(HKEY_LOCAL_MACHINE);
        reg.SetKey(_T("Software\\Uptooda\\Strings"), false);
    }

    CString getString(CString name, CString defaultValue) {
        auto it = map_.find(name);
        if (it != map_.end()) {
            return it->second;
        }
        CString res =  reg.ReadString(name, defaultValue);
        map_[name] = res;
        return res;
    }
};

class ATL_NO_VTABLE CIShellContextMenu :
    public CComObjectRootEx<CComSingleThreadModel>,
    public CComCoClass<CIShellContextMenu, &CLSID_IShellContextMenu>,
    public IContextMenu3, public IShellExtInit,
    public IDispatchImpl<IIShellContextMenu, &IID_IIShellContextMenu, &LIBID_ExplorerIntegrationLib, /*wMajor =*/ 1, /*wMinor =*/ 0>
    {
        public:
        struct IconCacheItem {
            HICON icon;
            int w;
            int h;
        };
        CAtlArray<CString> m_FileList;
        std::map<int, Shell_ContextMenuItem> m_nCommands;
        std::map<UINT, HBITMAP> cachedBitmaps_;
        std::map<HICON, HBITMAP> cachedIconBitmaps_;
        std::map<CString, IconCacheItem> cachedServerIcons_;
        StringsReader stringsReader_;
        CIShellContextMenu();
        ~CIShellContextMenu();

        DECLARE_REGISTRY_RESOURCEID(IDR_ISHELLCONTEXTMENU)

        BEGIN_COM_MAP(CIShellContextMenu)
            COM_INTERFACE_ENTRY(IIShellContextMenu)
            COM_INTERFACE_ENTRY(IDispatch)
            COM_INTERFACE_ENTRY(IContextMenu)
            COM_INTERFACE_ENTRY(IContextMenu2)
            COM_INTERFACE_ENTRY(IContextMenu3)
            COM_INTERFACE_ENTRY(IShellExtInit)
        END_COM_MAP()

        DECLARE_PROTECT_FINAL_CONSTRUCT()

        HRESULT FinalConstruct()
        {
            return S_OK;
        }

        void FinalRelease()
        {
        }
        bool MyInsertMenu(HMENU hMenu, int pos, UINT id, int nInternalCommand, const LPCTSTR szTitle, int firstCmd, CString commandArgument = CString(), bool UseBitmaps = true, HBITMAP bm=0,WORD resid=0,HICON ico = 0);
        bool MyInsertMenuSeparator(HMENU hMenu, int pos, UINT id);

        bool m_bMediaInfoInstalled;
        IconBitmapUtils m_IconBitmapUtils;
    HBITMAP IconToBitmap(HINSTANCE hInst, UINT uIcon);
    public:
        STDMETHOD(HandleMenuMsg)(UINT uMsg, WPARAM wParam, LPARAM lParam);
        STDMETHOD(HandleMenuMsg2)(UINT uMsg, WPARAM wParam, LPARAM lParam, LRESULT *pResult);


        STDMETHOD(QueryContextMenu)(HMENU hmenu, UINT indexMenu,UINT idCmdFirst, UINT idCmdLast, UINT uFlags);
        STDMETHOD(InvokeCommand)(LPCMINVOKECOMMANDINFO lpici);
        STDMETHOD(GetCommandString)(UINT_PTR idCmd, UINT uType, UINT *pwReserved, LPSTR pszName, UINT cchMax);
          STDMETHOD(Initialize)(LPCITEMIDLIST pidlFolder, LPDATAOBJECT dataObject, HKEY hkeyProgID);

protected:
     HICON GetCachedServerIcon(const CString& fileName, int w, int h);
     HBITMAP GetCachedIconToBitmapPARGB32(UINT uIcon);
     HBITMAP GetCachedHIconToBitmapPARGB32(HICON hIcon);
};

OBJECT_ENTRY_AUTO(__uuidof(IShellContextMenu), CIShellContextMenu)
