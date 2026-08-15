#include "ScreenRecorderUtils.h"

#include "Core/Utils/CoreUtils.h"

#ifdef _WIN32
#include <ComDef.h>

#include "Core/3rdpart/dxerr.h"

bool StringToGUID(const std::string& str, GUID& guid) {
    HRESULT hr = CLSIDFromString(IuCoreUtils::Utf8ToWstring(str).c_str(), &guid);
    return SUCCEEDED(hr);
}

CString GetMessageForHresult(HRESULT hr) {
    //_com_error error(hr);
    CString cs;
    WCHAR descr[1024] = L"";
    DXGetErrorDescriptionW(hr, descr, std::size(descr));
    cs.Format(_T("\r\nError 0x%08x: %s\r\n%s"), hr, DXGetErrorStringW(hr), descr);
    return cs;
}

CComPtr<IDXGIAdapter1> GetAdapterByIndex(int index) {
    CComPtr<IDXGIAdapter1> pAdapter;
    if (index < 0) {
        return pAdapter;
    }

    CComPtr<IDXGIFactory1> pFactory;
    HRESULT hr = CreateDXGIFactory1(__uuidof(IDXGIFactory1), (void**)&pFactory);

    if (FAILED(hr)) {
        return pAdapter;
    }

    for (UINT i = 0; pFactory->EnumAdapters1(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        if (i == index) {
            DXGI_ADAPTER_DESC1 desc;
            hr = pAdapter->GetDesc1(&desc);
            if (SUCCEEDED(hr)) {
                LOG(ERROR) << "Adapter " << i << desc.Description << std::endl;
                /*wprintf(L"VendorId: %04X, DeviceId: %04X\n",
                    desc.VendorId, desc.DeviceId);
                */
            }
            return pAdapter;
        }

        pAdapter.Release();
    }
    return pAdapter;
}

CComPtr<IDXGIAdapter1> GetAdapterForMonitor(HMONITOR hMonitor, UINT* adapterIndex, UINT* outputIndex) {
    outputIndex = 0;
    CComPtr<IDXGIFactory1> pFactory;
    CreateDXGIFactory1(__uuidof(IDXGIFactory1), reinterpret_cast<void**>(&pFactory));

    CComPtr<IDXGIAdapter1> pAdapter;
    for (UINT i = 0; pFactory->EnumAdapters1(i, &pAdapter) != DXGI_ERROR_NOT_FOUND; ++i) {
        CComPtr<IDXGIOutput> pOutput;
        for (UINT j = 0; pAdapter->EnumOutputs(j, &pOutput) != DXGI_ERROR_NOT_FOUND; ++j) {
            DXGI_OUTPUT_DESC outputDesc;
            pOutput->GetDesc(&outputDesc);

            if (outputDesc.Monitor == hMonitor) {
                if (adapterIndex) {
                    *adapterIndex = i;
                }

                if (outputIndex) {
                    *outputIndex = j;
                }

                return pAdapter;
            }
            pOutput.Release();
        }
        pAdapter.Release();
    }

    return nullptr;
}

#endif
