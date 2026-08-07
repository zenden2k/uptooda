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

#include "MyDataObject.h"

#include <strsafe.h>

CMyDataObject::CMyDataObject()
{
    // Reference count must ALWAYS start at 1.
    m_lRefCount = 1;
    TotalLength = 0;
    ZeroMemory(&m_FormatEtc, sizeof(m_FormatEtc));
    m_FormatEtc.cfFormat = CF_HDROP;
    m_FormatEtc.dwAspect = DVASPECT_CONTENT;
    m_FormatEtc.lindex = 0;
    m_FormatEtc.ptd = nullptr;
    m_FormatEtc.tymed = TYMED_HGLOBAL;

    ZeroMemory(&m_StgMedium, sizeof(m_StgMedium));
    m_StgMedium.tymed = TYMED_HGLOBAL;
}

CMyDataObject::~CMyDataObject()
{
    ReleaseStgMedium(&m_StgMedium);
}

void CMyDataObject::Reset()
{
    m_FileItems.RemoveAll();
}

void CMyDataObject::AddFile(LPCTSTR FileName)
{
    if(!FileName) return;
    m_FileItems.Add(CString(FileName));
    TotalLength += lstrlen(FileName)+1;
}


bool CMyDataObject::IsFormatSupported(FORMATETC *pFormatEtc)
{
    if (m_FormatEtc.cfFormat == pFormatEtc->cfFormat &&
        m_FormatEtc.dwAspect == pFormatEtc->dwAspect &&
        m_FormatEtc.tymed & pFormatEtc->tymed)
        return true;

    return false;
}

HRESULT __stdcall CMyDataObject::QueryInterface(REFIID iid, void **ppvObject)
{
    // Check to see what interface has been requested.
    if (iid == IID_IDataObject || iid == IID_IUnknown)
    {
        AddRef();
        *ppvObject = this;
        return S_OK;
    }
    else
    {
        *ppvObject = 0;
        return E_NOINTERFACE;
    }
}

ULONG __stdcall CMyDataObject::AddRef()
{
    // Increment object reference count.
    return InterlockedIncrement(&m_lRefCount);
}

ULONG __stdcall CMyDataObject::Release()
{
    // Decrement object reference count.
    LONG lCount = InterlockedDecrement(&m_lRefCount);

    if (lCount == 0)
    {
        delete this;
        return 0;
    }
    else
    {
        return lCount;
    }
}

HRESULT __stdcall CMyDataObject::GetData(FORMATETC *pFormatEtc, STGMEDIUM *pStgMedium)
{
    if (!IsFormatSupported(pFormatEtc))
        return DV_E_FORMATETC;

    // Copy the storage medium data.
    pStgMedium->tymed = m_StgMedium.tymed;
    pStgMedium->pUnkForRelease = nullptr;
    pStgMedium->hGlobal = GlobalAlloc(GMEM_SHARE,sizeof(DROPFILES)+(TotalLength+1)*sizeof(TCHAR));
    if (!pStgMedium->hGlobal) {
        return E_OUTOFMEMORY;
    }
    auto *dropFiles = static_cast<LPDROPFILES>(GlobalLock(pStgMedium->hGlobal));
    if (!dropFiles) {
        GlobalUnlock(pStgMedium->hGlobal);
        return E_OUTOFMEMORY;
    }
    ZeroMemory(dropFiles, sizeof(DROPFILES));
    dropFiles->fWide = TRUE;
    dropFiles->pFiles = sizeof(DROPFILES);

    auto files = reinterpret_cast<LPTSTR>(reinterpret_cast<PBYTE>(dropFiles) + dropFiles->pFiles);

    for(size_t i = 0; i<m_FileItems.GetCount(); i++)
    {
        StringCchCopy(files, m_FileItems[i].GetLength() + 1, m_FileItems[i]);
        files += m_FileItems[i].GetLength() + 1;
    }
    *files = '\0';

    GlobalUnlock(dropFiles);
    return S_OK;
}

HRESULT CMyDataObject::GetDataHere(FORMATETC *pFormatEtc, STGMEDIUM *pMedium)
{
    return DATA_E_FORMATETC;
}

HRESULT __stdcall CMyDataObject::QueryGetData(FORMATETC *pFormatEtc)
{
    return IsFormatSupported(pFormatEtc) ? S_OK : DV_E_FORMATETC;
}

HRESULT CMyDataObject::GetCanonicalFormatEtc(FORMATETC *pFormatEct, FORMATETC *pFormatEtcOut)
{
    // MUST be set to NULL.
    pFormatEtcOut->ptd = nullptr;
     return E_NOTIMPL;
}

HRESULT __stdcall CMyDataObject::SetData(FORMATETC *pFormatEtc, STGMEDIUM *pMedium, BOOL fRelease)
{
    return E_NOTIMPL;
}

HRESULT __stdcall CMyDataObject::EnumFormatEtc(DWORD dwDirection, IEnumFORMATETC **ppEnumFormatEtc)
{
    if (dwDirection == DATADIR_GET)
    {
        // Windows 2000 and newer only.
        return SHCreateStdEnumFmtEtc(1, &m_FormatEtc, ppEnumFormatEtc);
        //return CreateEnumFmtEtc(1,&m_FormatEtc,ppEnumFormatEtc);
    }
    else
    {
        return E_NOTIMPL;
    }
}

HRESULT CMyDataObject::DAdvise(FORMATETC *pFormatEtc, DWORD advf, IAdviseSink *pAdvSink,
    DWORD *pdwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT CMyDataObject::DUnadvise(DWORD dwConnection)
{
    return OLE_E_ADVISENOTSUPPORTED;
}

HRESULT CMyDataObject::EnumDAdvise(IEnumSTATDATA **ppEnumAdvise)
{
    return OLE_E_ADVISENOTSUPPORTED;
}
