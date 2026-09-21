#include "GrabFilter.h"

#include <windows.h>
#include <streams.h>
#include <initguid.h>
#include <strsafe.h>
#include "Core/Logging.h"

CGrab::CGrab(): m_inputPin(nullptr), m_grabFilter(new CGrabFilter(nullptr, this)){
}

CGrab::~CGrab() {
    delete m_inputPin;
}

CGrabFilter* CGrab::filter() const {
    return m_grabFilter;
}
// {32EEE835-6141-4214-A452-B29A0D5C6638}
DEFINE_GUID(CLSID_GrabFilter,
    0x32eee835, 0x6141, 0x4214, 0xa4, 0x52, 0xb2, 0x9a, 0xd, 0x5c, 0x66, 0x38);

CGrabFilter::CGrabFilter(LPUNKNOWN pUnk, CGrab* grab) :
    CBaseFilter(NAME("CGrabFilter"), pUnk, &grab->m_Lock, CLSID_GrabFilter),

    m_doGrab(false),
    m_callback(nullptr),
    m_pPosition(nullptr),
    m_segmentStart(0),
    m_grab(grab)

{
}

CGrabFilter::~CGrabFilter() {
    delete m_pPosition;
}

CBasePin * CGrabFilter::GetPin(int n){
    if (n == 0) {
        CAutoLock lock(&m_grab->m_Lock);
        
        if (!m_grab->m_inputPin) {
            HRESULT hr = S_OK;
            m_grab->m_inputPin = new CGrabInputPin(this, &m_grab->m_Lock, &m_ReceiveLock, &hr);
        }
        return m_grab->m_inputPin;
    } 
    return nullptr;
}

int CGrabFilter::GetPinCount(){
    return 1;
}

HRESULT CGrabFilter::Receive(IMediaSample* pSample) {
    CheckPointer(pSample, E_POINTER);
    if (m_callback && m_doGrab) {
        BYTE* data;
        m_doGrab = false;
        if (SUCCEEDED(pSample->GetPointer(&data))) {
            BITMAPINFOHEADER* bih = nullptr;
            if ((m_mediaType.formattype == FORMAT_VideoInfo) &&
                (m_mediaType.cbFormat >= sizeof(VIDEOINFOHEADER)) &&
                (m_mediaType.pbFormat != NULL))
            {
                auto vih = reinterpret_cast<VIDEOINFOHEADER*>(m_mediaType.pbFormat);
                bih = &vih->bmiHeader;
                BITMAPINFOHEADER *pbmi = HEADER(m_mediaType.Format());
                REFERENCE_TIME tStart = 0, tStop = 0;
                if (FAILED(pSample->GetTime(&tStart, &tStop))) {
                    LOG(ERROR) << "Unable to get sample time";
                }
                m_callback->OnDumpSample(m_segmentStart + tStart, data, pSample->GetActualDataLength(), bih);
            }
            else {
                LOG(ERROR) << "Invalid media type";
            } 
        }
        NotifyEvent(EC_COMPLETE, 0, 0);
    }
    return S_OK;
}

HRESULT CGrabFilter::EndOfStream() {
    NotifyEvent(EC_COMPLETE, 0, 0);
    return S_OK;
}

HRESULT CGrabFilter::SetMediaType(const CMediaType* pmt) {
    m_mediaType = *pmt;
    return S_OK;
}

void CGrabFilter::SetGrab(bool grab) {
    m_doGrab = grab;
}

void CGrabFilter::SetCallback(CGrabCallback* cb) {
    m_callback = cb;
}

HRESULT CGrabFilter::NonDelegatingQueryInterface(const IID& riid, void** ppv) {
    if (riid == IID_IMediaPosition || riid == IID_IMediaSeeking) {
        if (m_pPosition == nullptr){
            HRESULT hr = S_OK;
            try {
                m_pPosition = new CPosPassThru(NAME("Dump Pass Through"),
                    (IUnknown*)GetOwner(),
                    (HRESULT*)&hr, m_grab->m_inputPin);
            } catch (const std::bad_alloc&) {
                return E_OUTOFMEMORY;
            }

            if (FAILED(hr)){
                delete m_pPosition;
                m_pPosition = nullptr;
                return hr;
            }
        }

        return m_pPosition->NonDelegatingQueryInterface(riid, ppv);
    }
    return CBaseFilter::NonDelegatingQueryInterface(riid, ppv);
}

HRESULT CGrabFilter::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate) {
    m_segmentStart = tStart;
    return S_OK;
}

//
//  Definition of CGrabInputPin
//
CGrabInputPin::CGrabInputPin(CGrabFilter *pFilter, CCritSec *pLock, CCritSec *pReceiveLock, HRESULT *phr) :
    CRenderedInputPin(NAME("CGrabInputPin"),
                  pFilter,                   // Filter
                  pLock,                     // Locking
                  phr,                       // Return code
                  L"Input"),                 // Pin name
    m_pReceiveLock(pReceiveLock),
    m_pDumpFilter(pFilter)
{
}

// Check if the pin can support this specific proposed type and format
HRESULT CGrabInputPin::CheckMediaType(const CMediaType *pmt){
    if (*pmt->Type() != MEDIATYPE_Video) {
        return S_FALSE;
    }
    if ((*pmt->FormatType() != FORMAT_VideoInfo) /*&&
        (*pmt->FormatType() != FORMAT_VideoInfo2)*/) {
        return S_FALSE;
    }

    if (/*(*pmt->Subtype() != MEDIASUBTYPE_YUY2) &&
        (*pmt->Subtype() != MEDIASUBTYPE_RGB565) &&*/
        (*pmt->Subtype() != MEDIASUBTYPE_RGB24)) {
        return S_FALSE;
    }
    return S_OK;
}

HRESULT CGrabInputPin::SetMediaType(const CMediaType* pmt) {
    return m_pDumpFilter->SetMediaType(pmt);
}

// ReceiveCanBlock
// We don't hold up source threads on Receive
STDMETHODIMP CGrabInputPin::ReceiveCanBlock() {
    return S_FALSE;
}

STDMETHODIMP CGrabInputPin::Receive(IMediaSample *pSample) {
    CAutoLock lock(m_pReceiveLock);
    return m_pDumpFilter->Receive(pSample);
}

STDMETHODIMP CGrabInputPin::EndOfStream() {
    CAutoLock lock(m_pReceiveLock);
    return m_pDumpFilter->EndOfStream();
}

// Called when we are seeked
STDMETHODIMP CGrabInputPin::NewSegment(REFERENCE_TIME tStart, REFERENCE_TIME tStop, double dRate) {
    return m_pDumpFilter->NewSegment(tStart, tStop, dRate);
} 
