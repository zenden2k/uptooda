// Based on code from
// https://github.com/microsoft/Windows-classic-samples/tree/main/Samples/Win7Samples/multimedia/mediafoundation/VideoThumbnail
// Copyright (c) Microsoft Corporation. All rights reserved.

#include "MediaFoundationFrameGrabber.h"

// Direct2D
#include <D2d1.h>
#include <D2d1helper.h>

// Misc
#include <strsafe.h>
#include <assert.h>
#include <propvarutil.h>

#include <comdef.h>

#include "Core/Utils/CoreUtils.h"
#include "Core/CommonDefs.h"

#pragma warning(disable:4127)  // Disable warning C4127: conditional expression is constant

RECT CorrectAspectRatio(const RECT& src, const MFRatio& srcPAR);

constexpr LONGLONG SEEK_TOLERANCE = 10000000;
constexpr LONGLONG MAX_FRAMES_TO_SKIP = 10;

class MFVideoFrame : public AbstractVideoFrame {
public:
    MFVideoFrame(unsigned char* data, unsigned int dataSize, int64_t time, int width, int height) {
        time_ = time;
        data_ = data;
        dataSize_ = dataSize;
        width_ = width;
        height_ = height;
        img_ = std::unique_ptr<AbstractImage>(AbstractImage::createImage());
        if (!img_) {
            return;
        }
        if (!img_->loadFromRawData(AbstractImage::dfRGB32bpp, width_, height_, data_, dataSize_,
                reinterpret_cast<void*>(static_cast<INT_PTR>(width_ * 4)))) {
        }
    }

    ~MFVideoFrame() override {
    }

    bool saveToFile(const std::string& fileName) const override {
        return img_->saveToFile(fileName);
    }

    std::unique_ptr<AbstractImage> toImage() const override {
        return std::move(img_);
    }

protected:
    unsigned char* data_;
    unsigned int dataSize_;
    mutable std::unique_ptr<AbstractImage> img_;
    DISALLOW_COPY_AND_ASSIGN(MFVideoFrame);
};

MediaFoundationFrameGrabber::MediaFoundationFrameGrabber()
    : initializer_(COM_MULTI_THREADED)
{
    //if (initializer_.isCOMInitialized()) {
        
    //}
    ZeroMemory(&format_, sizeof(format_));
}

bool MediaFoundationFrameGrabber::open(const std::string& fileName)
{
    HRESULT hr = S_OK;

    CComPtr<IMFAttributes> pAttributes;

    SafeRelease(&sourceReader_);

    // Configure the source reader to perform video processing.
    //
    // This includes:
    //   - YUV to RGB-32
    //   - Software deinterlace

    hr = MFCreateAttributes(&pAttributes, 1);
    if (FAILED(hr)) {
        _com_error comError(hr);
        LOG(WARNING) << "MFCreateAttributes failed." << comError.ErrorMessage();
        return false;
    }

    hr = pAttributes->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
    
    // Create the source reader from the URL.
    hr = MFCreateSourceReaderFromURL(IuCoreUtils::Utf8ToWstring(fileName).c_str(), pAttributes, &sourceReader_);
    if (FAILED(hr)) {
        _com_error comError(hr);
        LOG(WARNING) << "MFCreateSourceReaderFromURL failed." << std::endl << comError.ErrorMessage();
        return false;
    }

    // Attempt to find a video stream.
    hr = selectVideoStream();

    if (FAILED(hr)) {
        _com_error comError(hr);
        LOG(WARNING) << "SelectVideoStream failed." << std::endl << comError.ErrorMessage();
        return false;
    }

    LONGLONG hnsDuration = 0;
    LONGLONG hnsRangeStart = 0;
    LONGLONG hnsRangeEnd = 0;
    LONGLONG hnsIncrement = 0;
    hr = getDuration(&hnsDuration);

    if (SUCCEEDED(hr)) {
        duration_ = hnsDuration;
    } else {
        _com_error comError(hr);
        LOG(WARNING) << "getDuration failed." << std::endl << comError.ErrorMessage();
    }

    return true;
}

bool MediaFoundationFrameGrabber::seek(int64_t time) {
    HRESULT hr = S_OK;
    BOOL bCanSeek = 0;

    hr = canSeek(&bCanSeek);

    if (FAILED(hr)) {
        return false;
    }

    LONGLONG hnsTimeStamp = 0;
    DWORD cSkipped = 0; // Number of skipped frames

    if (bCanSeek && (time > 0)) {
        PROPVARIANT var;
        PropVariantInit(&var);

        var.vt = VT_I8;
        var.hVal.QuadPart = time;

        hr = sourceReader_->SetCurrentPosition(GUID_NULL, var);

        if (FAILED(hr)) {
            return false;
        }
        return true;
    }
    return false;
}

AbstractVideoFrame* MediaFoundationFrameGrabber::grabCurrentFrame() {
    HRESULT hr = S_OK;
    DWORD dwFlags = 0;
    BYTE* pBitmapData = NULL; // Bitmap data
    DWORD cbBitmapData = 0; // Size of data, in bytes
    LONGLONG hnsTimeStamp = 0;
    BOOL bCanSeek = FALSE; // Can the source seek?
    DWORD cSkipped = 0; // Number of skipped frames

    CComPtr<IMFMediaBuffer> pBuffer;
    IMFSample* pSample = NULL;

    // NOTE: Seeking might be inaccurate, depending on the container
    //       format and how the file was indexed. Therefore, the first
    //       frame that we get might be earlier than the desired time.
    //       If so, we skip up to MAX_FRAMES_TO_SKIP frames.

    while (1) {
        IMFSample* pSampleTmp = NULL;

        hr = sourceReader_->ReadSample(
            (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM,
            0,
            NULL,
            &dwFlags,
            NULL,
            &pSampleTmp);

        if (FAILED(hr)) {
            goto done;
        }

        if (dwFlags & MF_SOURCE_READERF_ENDOFSTREAM) {
            break;
        }

        if (dwFlags & MF_SOURCE_READERF_CURRENTMEDIATYPECHANGED) {
            // Type change. Get the new format.
            hr = getVideoFormat(&format_);

            if (FAILED(hr)) {
                goto done;
            }
        }

        if (pSampleTmp == NULL) {
            continue;
        }

        // We got a sample. Hold onto it.

        SafeRelease(&pSample);

        pSample = pSampleTmp;
        pSample->AddRef();

        if (SUCCEEDED(pSample->GetSampleTime(&hnsTimeStamp))) {
            // Keep going until we get a frame that is within tolerance of the
            // desired seek position, or until we skip MAX_FRAMES_TO_SKIP frames.

            // During this process, we might reach the end of the file, so we
            // always cache the last sample that we got (pSample).

            if ((cSkipped < MAX_FRAMES_TO_SKIP) && (hnsTimeStamp + SEEK_TOLERANCE < pos_)) {
                SafeRelease(&pSampleTmp);

                ++cSkipped;
                continue;
            }
        }

        SafeRelease(&pSampleTmp);

        pos_ = hnsTimeStamp;
        break;
    }

    if (pSample) {
        UINT32 pitch = 4 * format_.imageWidthPels;

        // Get the bitmap data from the sample, and use it to create a
        // Direct2D bitmap object. Then use the Direct2D bitmap to
        // initialize the sprite.

        hr = pSample->ConvertToContiguousBuffer(&pBuffer);

        if (FAILED(hr)) {
            goto done;
        }

        hr = pBuffer->Lock(&pBitmapData, NULL, &cbBitmapData);

        if (FAILED(hr)) {
            goto done;
        }

        assert(cbBitmapData == (pitch * format_.imageHeightPels));
        if (cbBitmapData == pitch * format_.imageHeightPels) {
            LONGLONG time = pos_ / 10000000;
            currentFrame_ = std::make_unique<MFVideoFrame>(pBitmapData, cbBitmapData, time, format_.imageWidthPels, format_.imageHeightPels);
        }    
    } else {
        hr = MF_E_END_OF_STREAM;
    }

done:

    if (pBitmapData) {
        pBuffer->Unlock();
    }
    SafeRelease(&pSample);
    return currentFrame_.get();
}

int64_t MediaFoundationFrameGrabber::duration() {
    return duration_;
}

//-------------------------------------------------------------------
// GetDuration: Finds the duration of the current video file.
//-------------------------------------------------------------------

HRESULT MediaFoundationFrameGrabber::getDuration(LONGLONG *phnsDuration)
{
    PROPVARIANT var;
    PropVariantInit(&var);

    HRESULT hr = S_OK;

    if (sourceReader_ == NULL)
    {
        return MF_E_NOT_INITIALIZED;
    }
    
    hr = sourceReader_->GetPresentationAttribute( 
        (DWORD)MF_SOURCE_READER_MEDIASOURCE, 
        MF_PD_DURATION, 
        &var 
        );

    if (SUCCEEDED(hr))
    {
        assert(var.vt == VT_UI8);
        *phnsDuration = var.hVal.QuadPart;
    }

    PropVariantClear(&var);

    return hr; 
}



//-------------------------------------------------------------------
// CanSeek: Queries whether the current video file is seekable.
//-------------------------------------------------------------------

HRESULT MediaFoundationFrameGrabber::canSeek(BOOL *pbCanSeek)
{
    HRESULT hr = S_OK;

    ULONG flags = 0;

    PROPVARIANT var;
    PropVariantInit(&var);

    if (sourceReader_ == NULL)
    {
        return MF_E_NOT_INITIALIZED;
    }

    *pbCanSeek = FALSE;

    hr = sourceReader_->GetPresentationAttribute(
        (DWORD)MF_SOURCE_READER_MEDIASOURCE,
        MF_SOURCE_READER_MEDIASOURCE_CHARACTERISTICS,
        &var
        );

    if (SUCCEEDED(hr))
    {
        hr = PropVariantToUInt32(var, &flags);
    }

    if (SUCCEEDED(hr))
    {
        // If the source has slow seeking, we will treat it as
        // not supporting seeking. 

        if ((flags & MFMEDIASOURCE_CAN_SEEK) && 
            !(flags & MFMEDIASOURCE_HAS_SLOW_SEEK))
        {
            *pbCanSeek = TRUE;
        }
    }

    return hr;
}

//-------------------------------------------------------------------
// SelectVideoStream
//
// Finds the first video stream and sets the format to RGB32.
//-------------------------------------------------------------------

HRESULT MediaFoundationFrameGrabber::selectVideoStream()
{
    HRESULT hr = S_OK;

    CComPtr<IMFMediaType> pType;

    // Configure the source reader to give us progressive RGB32 frames.
    // The source reader will load the decoder if needed.

    hr = MFCreateMediaType(&pType);

    if (SUCCEEDED(hr))
    {
        hr = pType->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    }

    if (SUCCEEDED(hr))
    {
        hr = pType->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    }


    if (SUCCEEDED(hr))
    {
        hr = sourceReader_->SetCurrentMediaType(
            (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 
            NULL,
            pType
            );
    }

    // Ensure the stream is selected.
    if (SUCCEEDED(hr))
    {
        hr = sourceReader_->SetStreamSelection(
            (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 
            TRUE
            );
    }

    if (SUCCEEDED(hr))
    {
        hr = getVideoFormat(&format_);
    }

    return hr;
}


//-------------------------------------------------------------------
// GetVideoFormat
// 
// Gets format information for the video stream.
//
// iStream: Stream index.
// pFormat: Receives the format information.
//-------------------------------------------------------------------

HRESULT MediaFoundationFrameGrabber::getVideoFormat(FormatInfo *pFormat)
{
    HRESULT hr = S_OK;
    UINT32  width = 0, height = 0;
    LONG lStride = 0;
    MFRatio par = { 0 , 0 };

    FormatInfo& format = *pFormat;

    GUID subtype = { 0 };

    IMFMediaType *pType = NULL;

    // Get the media type from the stream.
    hr = sourceReader_->GetCurrentMediaType(
        (DWORD)MF_SOURCE_READER_FIRST_VIDEO_STREAM, 
        &pType
        );
    
    if (FAILED(hr)) { goto done; }

    // Make sure it is a video format.
    hr = pType->GetGUID(MF_MT_SUBTYPE, &subtype);
    if (subtype != MFVideoFormat_RGB32)
    {
        hr = E_UNEXPECTED;
        goto done;
    }

    // Get the width and height
    hr = MFGetAttributeSize(pType, MF_MT_FRAME_SIZE, &width, &height);

    if (FAILED(hr)) { goto done; }

    // Get the stride to find out if the bitmap is top-down or bottom-up.
    lStride = (LONG)MFGetAttributeUINT32(pType, MF_MT_DEFAULT_STRIDE, 1);

    format.bTopDown = (lStride > 0); 

    // Get the pixel aspect ratio. (This value might not be set.)
    hr = MFGetAttributeRatio(pType, MF_MT_PIXEL_ASPECT_RATIO, (UINT32*)&par.Numerator, (UINT32*)&par.Denominator);
    if (SUCCEEDED(hr) && (par.Denominator != par.Numerator))
    {
        RECT rcSrc = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };

        format.rcPicture = CorrectAspectRatio(rcSrc, par);
    }
    else
    {
        // Either the PAR is not set (assume 1:1), or the PAR is set to 1:1.
        SetRect(&format.rcPicture, 0, 0, width, height);
    }

    format.imageWidthPels = width;
    format.imageHeightPels = height;

done:
    SafeRelease(&pType);

    return hr;
}

//-----------------------------------------------------------------------------
// CorrectAspectRatio
//
// Converts a rectangle from the source's pixel aspect ratio (PAR) to 1:1 PAR.
// Returns the corrected rectangle.
//
// For example, a 720 x 486 rect with a PAR of 9:10, when converted to 1x1 PAR,  
// is stretched to 720 x 540. 
//-----------------------------------------------------------------------------

RECT CorrectAspectRatio(const RECT& src, const MFRatio& srcPAR)
{
    // Start with a rectangle the same size as src, but offset to the origin (0,0).
    RECT rc = {0, 0, src.right - src.left, src.bottom - src.top};

    if ((srcPAR.Numerator != 1) || (srcPAR.Denominator != 1))
    {
        // Correct for the source's PAR.

        if (srcPAR.Numerator > srcPAR.Denominator)
        {
            // The source has "wide" pixels, so stretch the width.
            rc.right = MulDiv(rc.right, srcPAR.Numerator, srcPAR.Denominator);
        }
        else if (srcPAR.Numerator < srcPAR.Denominator)
        {
            // The source has "tall" pixels, so stretch the height.
            rc.bottom = MulDiv(rc.bottom, srcPAR.Denominator, srcPAR.Numerator);
        }
        // else: PAR is 1:1, which is a no-op.
    }
    return rc;
}

MediaFoundationInitializer::MediaFoundationInitializer() {
    // Initialize Media Foundation.
    HRESULT hr = MFStartup(MF_VERSION);
    if (FAILED(hr)) {
        _com_error comError(hr);
        LOG(ERROR) << "MFStartup failed." << std::endl << comError.ErrorMessage();
    }
}

MediaFoundationInitializer::~MediaFoundationInitializer() {
    MFShutdown();
}
