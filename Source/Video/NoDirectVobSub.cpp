#include "NoDirectVobSub.h"

NoDirectVobSub::NoDirectVobSub() {
    Refs = 0;
}

STDMETHODIMP NoDirectVobSub::QueryInterface(REFIID riid, __deref_out void** ppv) 
{
    if (riid == IID_IUnknown)                        (*ppv) = static_cast<IUnknown*>(this);
    else if (riid == IID_IAMGraphBuilderCallback)    (*ppv) = static_cast<IAMGraphBuilderCallback*>(this);

    else { (*ppv) = 0; return E_NOINTERFACE; }

    AddRef();
    return S_OK;
}

// Fake out any COM ref counting
STDMETHODIMP_(ULONG) NoDirectVobSub::AddRef()  {
    return 2;
}
STDMETHODIMP_(ULONG) NoDirectVobSub::Release()  {
    return 1;
}

HRESULT STDMETHODCALLTYPE NoDirectVobSub::SelectedFilter( /* [in] */ IMoniker* pMon) 
{
    HRESULT ret = S_OK;

    IBindCtx* bind; ;
    if (SUCCEEDED(CreateBindCtx(0, &bind)))
    {
        LPOLESTR name;
        if (SUCCEEDED(pMon->GetDisplayName(bind, 0, &name)))
        {
            // thanks to http://rsdn.ru/forum/media/4970888.1
            if (wcsstr(name, L"93A22E7A-5091-45EF-BA61-6DA26156A5D0") != 0) ret = E_ABORT;    // DirectVobSub
            if (wcsstr(name, L"9852A670-F845-491B-9BE6-EBD841B8A613") != 0) ret = E_ABORT;    // DirectVobSub autoload
        }
        bind->Release();
    }

    return ret;
}

HRESULT STDMETHODCALLTYPE NoDirectVobSub::CreatedFilter( /* [in] */ IBaseFilter* pFil) {
    return S_OK;
}