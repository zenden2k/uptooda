#include "DirectShowUtil.h"

#include "Core/3rdpart/dxerr.h"
#include "Core/Utils/CoreUtils.h"

namespace DirectShowUtil {

HRESULT GetPin(IBaseFilter* pFilter, PIN_DIRECTION dirrequired, int iNum, IPin** ppPin) {
    IEnumPins* pEnum;
    *ppPin = nullptr;

    HRESULT hr = pFilter->EnumPins(&pEnum);
    if (FAILED(hr))
        return hr;

    ULONG ulFound;
    IPin* pPin;
    hr = E_FAIL;

    while (S_OK == pEnum->Next(1, &pPin, &ulFound)) {
        auto pindir = static_cast<PIN_DIRECTION>(3);

        pPin->QueryDirection(&pindir);
        if (pindir == dirrequired) {
            if (iNum == 0) {
                *ppPin = pPin; // Return the pin's interface
                hr = S_OK; // Found requested pin, so clear error
                break;
            }
            iNum--;
        }

        pPin->Release();
    }

    pEnum->Release();

    return hr;
}

CComPtr<IPin> GetInPin(IBaseFilter* pFilter, int nPin) {
    CComPtr<IPin> pComPin;
    GetPin(pFilter, PINDIR_INPUT, nPin, &pComPin);
    return pComPin;
}

CComPtr<IPin> GetOutPin(IBaseFilter* pFilter, int nPin) {
    CComPtr<IPin> pComPin;
    GetPin(pFilter, PINDIR_OUTPUT, nPin, &pComPin);
    return pComPin;
}

GUID GuidFromString(const CString& guidStr) {
    GUID res = GUID_NULL;
    CLSIDFromString(guidStr, &res);
    return res;
}

CComPtr<IBaseFilter> FindFilterByClassID(IGraphBuilder* graphBuilder, GUID classID) {
    CComPtr<IBaseFilter> filterFound;

    CComPtr<IEnumFilters> ienumFilt;
    HRESULT hr = graphBuilder->EnumFilters(&ienumFilt);
    if (SUCCEEDED(hr) && ienumFilt) {
        ULONG iFetched;
        IBaseFilter* filter;

        do {
            hr = ienumFilt->Next(1, &filter, &iFetched);
            if (SUCCEEDED(hr) && iFetched == 1) {
                GUID filterGuid;
                HRESULT hr2 = filter->GetClassID(&filterGuid);

                if (filterGuid == classID) {
                    filterFound.Attach(filter);
                    return filterFound;
                }
                filter->Release();
                filter = nullptr;
            }
        }
        while (iFetched == 1 && SUCCEEDED(hr));
    }

    return filterFound;
}

HRESULT SaveGraphFile(IGraphBuilder* pGraph, WCHAR* wszPath) {
    const WCHAR wszStreamName[] = L"ActiveMovieGraph";
    HRESULT hr;

    IStorage* pStorage = nullptr;
    hr = StgCreateDocfile(wszPath, STGM_CREATE | STGM_TRANSACTED | STGM_READWRITE | STGM_SHARE_EXCLUSIVE, 0, &pStorage);
    if (FAILED(hr)) {
        return hr;
    }

    IStream* pStream;
    hr = pStorage->CreateStream(
        wszStreamName,
        STGM_WRITE | STGM_CREATE | STGM_SHARE_EXCLUSIVE,
        0, 0, &pStream);
    if (FAILED(hr)) {
        pStorage->Release();
        return hr;
    }

    IPersistStream* pPersist = nullptr;
    pGraph->QueryInterface(IID_IPersistStream, reinterpret_cast<void**>(&pPersist));
    hr = pPersist->Save(pStream, TRUE);
    pStream->Release();
    pPersist->Release();
    if (SUCCEEDED(hr)) {
        hr = pStorage->Commit(STGC_DEFAULT);
    }
    pStorage->Release();
    return hr;
}

std::string GetMessageForHresult(HRESULT hr) {
    CString cs;
    WCHAR descr[1024] = L"";
    DXGetErrorDescriptionW(hr, descr, ARRAY_SIZE(descr));
    cs.Format(_T("\r\nError 0x%08x: %s\r\n%s"), hr, DXGetErrorStringW(hr), descr);
    return W2U(cs);
}

}
