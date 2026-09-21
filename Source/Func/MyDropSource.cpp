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

#include "MyDropSource.h"

CMyDropSource::CMyDropSource()
{
    m_lRefCount = 1;
}

HRESULT __stdcall CMyDropSource::QueryInterface(REFIID iid,void **ppvObject)
{
    // Check to see what interface has been requested.
    if (iid == IID_IDropSource || iid == IID_IUnknown)
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

ULONG __stdcall CMyDropSource::AddRef()
{
    // Increment object reference count.
    return InterlockedIncrement(&m_lRefCount);
}

ULONG __stdcall CMyDropSource::Release()
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

HRESULT __stdcall CMyDropSource::QueryContinueDrag(BOOL fEscapePressed,DWORD grfKeyState)
{
    // If the escape key has been pressed we cancel the operation.
    if (fEscapePressed != FALSE)
        return DRAGDROP_S_CANCEL;

    // If the left button has been released we should drop.
    if ((grfKeyState & MK_LBUTTON) == 0)
        return DRAGDROP_S_DROP;

    return S_OK;
}

HRESULT __stdcall CMyDropSource::GiveFeedback(DWORD dwEffect)
{
    return DRAGDROP_S_USEDEFAULTCURSORS;
}
