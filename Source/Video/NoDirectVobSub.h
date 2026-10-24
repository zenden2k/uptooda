#ifndef IU_CORE_VIDEO_NODIRECTVOBSUB_H
#define IU_CORE_VIDEO_NODIRECTVOBSUB_H

#include <windows.h>
#include <streams.h>

// A COM object to be passed to the GraphBuilder callback function, in order to avoid loading subtitles
class NoDirectVobSub : public IAMGraphBuilderCallback
{
    int Refs;

public:
    NoDirectVobSub();
    STDMETHODIMP QueryInterface(REFIID riid, __deref_out void** ppv) override;
    
    // Fake out any COM ref counting
    STDMETHODIMP_(ULONG) AddRef() override;
    STDMETHODIMP_(ULONG) Release() override;

    HRESULT STDMETHODCALLTYPE SelectedFilter( /* [in] */ IMoniker* pMon) override;

    HRESULT STDMETHODCALLTYPE CreatedFilter( /* [in] */ IBaseFilter* pFil) override;
};


#endif