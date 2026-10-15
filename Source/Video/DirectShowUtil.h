#ifndef IU_CORE_VIDEO_DIRECTSHOWUTIL_H
#define IU_CORE_VIDEO_DIRECTSHOWUTIL_H

#pragma once
#include <string>
#include "atlheaders.h"
#define __AFX_H__ // little hack for avoiding __POSITION type redefinition
#include <objbase.h>
#include <streams.h>
#undef __AFX_H__
#include <qedit.h>
#include <comdef.h>
#include <comip.h>
#include <atlbase.h>

typedef struct tagVIDEOINFOHEADER2 {
    RECT rcSource;
    RECT rcTarget;
    DWORD dwBitRate;
    DWORD dwBitErrorRate;
    REFERENCE_TIME AvgTimePerFrame;
    DWORD dwInterlaceFlags;
    DWORD dwCopyProtectFlags;
    DWORD dwPictAspectRatioX;
    DWORD dwPictAspectRatioY;

    union {
        DWORD dwControlFlags;
        DWORD dwReserved1;
    };

    DWORD dwReserved2;
    BITMAPINFOHEADER bmiHeader;
} VIDEOINFOHEADER2;

namespace DirectShowUtil {
    HRESULT GetPin(IBaseFilter* pFilter, PIN_DIRECTION dirrequired,  int iNum, IPin** ppPin);
    CComPtr<IPin>  GetInPin ( IBaseFilter* pFilter, int Num );
    CComPtr<IPin>  GetOutPin( IBaseFilter* pFilter, int Num );
    GUID GuidFromString(const CString& guidStr);
    CComPtr<IBaseFilter> FindFilterByClassID(IGraphBuilder* graphBuilder, GUID classID);
    HRESULT SaveGraphFile(IGraphBuilder *pGraph, WCHAR *wszPath);
    std::string GetMessageForHresult(HRESULT hr);
}

#endif