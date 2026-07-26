// Based on code from
// https://github.com/microsoft/Windows-classic-samples/tree/main/Samples/Win7Samples/multimedia/mediafoundation/VideoThumbnail
// Copyright (c) Microsoft Corporation. All rights reserved.

#pragma once

#include <memory>

// Media Foundation
#include <mfapi.h>
#include <mferror.h>
#include <mfidl.h>
#include <mfreadwrite.h>

// Shell
#include <shellapi.h>
#include <shobjidl.h>

#include "atlheaders.h"

#include "AbstractFrameGrabber.h"
#include "Core/COMInitializer.h"

template <class T>
void SafeRelease(T** ppT)
{
    if (*ppT) {
        (*ppT)->Release();
        *ppT = NULL;
    }
}

struct FormatInfo {
    UINT32 imageWidthPels;
    UINT32 imageHeightPels;
    BOOL bTopDown;
    RECT rcPicture; // Corrected for pixel aspect ratio

    FormatInfo()
        : imageWidthPels(0)
        , imageHeightPels(0)
        , bTopDown(FALSE)
    {
        SetRectEmpty(&rcPicture);
    }
};

class MediaFoundationInitializer {
public:
    MediaFoundationInitializer();

    ~MediaFoundationInitializer();
};

class MFVideoFrame;

class MediaFoundationFrameGrabber : public AbstractFrameGrabber {
public:
    MediaFoundationFrameGrabber();
    ~MediaFoundationFrameGrabber() override;
    bool open(const std::string& fileName) override;
    bool seek(int64_t time) override;
    AbstractVideoFrame* grabCurrentFrame() override;
    int64_t duration() override;
private:
    HRESULT getDuration(LONGLONG* phnsDuration);
    HRESULT canSeek(BOOL* pbCanSeek);
    HRESULT selectVideoStream();
    HRESULT getVideoFormat(FormatInfo *pFormat);

    CComPtr<IMFSourceReader> sourceReader_;
    FormatInfo format_;
    COMInitializer initializer_;
    int64_t duration_ = 0;
    LONGLONG pos_ = -1;
    std::unique_ptr<MFVideoFrame> currentFrame_;
};






