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

#include "DirectshowFrameGrabber2.h"

#include "atlheaders.h"
#include "Core/Utils/CoreUtils.h"
#include <windows.h>
#define __AFX_H__ // little hack for avoiding __POSITION type redefinition
#include <objbase.h>
#include <streams.h>
#undef __AFX_H__
#include <qedit.h>
#include <comdef.h>
#include <comip.h>
#include <strmif.h>

#include "AbstractVideoFrame.h"
#include "Core/Logging.h"
#include "Core/3rdpart/dxerr.h"
#include "DirectShowUtil.h"
#include "Core/i18n/Translator.h"
#include "GrabFilter.h"
#include "NoDirectVobSub.h"
#include "Core/COMInitializer.h"

class DirectshowVideoFrame2 : public AbstractVideoFrame {
public :
    DirectshowVideoFrame2(unsigned char* data, size_t dataSize, int64_t time, int width, int height) {

        time_ = time;

        BITMAPFILEHEADER bfh;
        memset(&bfh, 0, sizeof(bfh));
        bfh.bfType = ('M' << 8) | 'B';
        bfh.bfSize = sizeof(bfh) + dataSize + sizeof(BITMAPINFOHEADER);
        bfh.bfOffBits = sizeof(BITMAPINFOHEADER) + sizeof(BITMAPFILEHEADER);

        BITMAPINFOHEADER bih;
        memset(&bih, 0, sizeof(bih));
        bih.biSize = sizeof(bih);
        bih.biWidth = width;
        bih.biHeight = height;
        bih.biPlanes = 1;
        bih.biBitCount = 24;

        // Add padding to image
        size_t newStripeSize = 4 * ((width * 3 + 3) / 4);
        size_t oldStripeSize = width * 3;
        size_t newDataSize = newStripeSize * height;
        int dataOffset = sizeof(bfh) + sizeof(bih);
        dataSize_ = newDataSize + dataOffset;
        data_ = new unsigned char[newDataSize + dataOffset];
        memcpy(data_, &bfh, sizeof(bfh));
        memcpy(data_ + sizeof(bfh), &bih, sizeof(bih));

        // Copy pixels line by line
        for (int i = 0; i < height; i++) {
            memcpy(data_ + dataOffset + newStripeSize * i, data + oldStripeSize * i, oldStripeSize);
        }

        width_ = width;
        height_ = height;
    }

    ~DirectshowVideoFrame2() override {
        delete[] data_;
    }

    bool saveToFile(const std::string& fileName) const override {
        std::unique_ptr<AbstractImage> image(AbstractImage::createImage());
        if (!image) {
            return false;
        }
        if (!image->loadFromRawData(AbstractImage::dfBitmapRgb, width_, height_, data_, dataSize_, nullptr)) {
            return false;
        }
        return image->saveToFile(fileName);
    }

    std::unique_ptr<AbstractImage> toImage() const override {
        std::unique_ptr<AbstractImage> image(AbstractImage::createImage());
        if (!image) {
            return nullptr;
        }
        if (!image->loadFromRawData(AbstractImage::dfBitmapRgb, width_, height_, data_, dataSize_, nullptr)) {
            image.reset();
        }
        return image;
    }

protected:
    unsigned char* data_;
    size_t dataSize_;
    DISALLOW_COPY_AND_ASSIGN(DirectshowVideoFrame2);
};

class DirectshowFrameGrabber2Private : public CGrabCallback {
public:
    class Initializer {
    public:
        Initializer() {}

        ~Initializer() {
            DbgTerminate();
        }
    };

    DirectshowFrameGrabber2Private() {
        static Initializer initializer;
        memset(&vih, 0, sizeof(vih));
    }

    DirectshowVideoFrame2* currentFrame() const {
        return currentFrame_.get();
    }

    void setCurrentFrame(DirectshowVideoFrame2* frame) {
        currentFrame_.reset(frame);
    }

    std::unique_ptr<CGrab> grab;
    CComPtr<IBaseFilter> pSource;
    CComPtr<IGraphBuilder> pGraph;
    CComPtr<IVideoWindow> pVideoWindow;
    CComQIPtr<IMediaControl, &IID_IMediaControl> pControl;
    CComQIPtr<IMediaEvent, &IID_IMediaEvent> pEvent;
    CComQIPtr<IMediaSeeking, &IID_IMediaSeeking> pSeeking;

    CComQIPtr<IVideoWindow, &IID_IVideoWindow> pWindow;
    CComQIPtr<IBaseFilter, &IID_IBaseFilter> pGrabberBase;
    CComQIPtr<IFileSourceFilter, &IID_IFileSourceFilter> pLoad;
    CComPtr<IPin> pSourcePin;
    CComPtr<IPin> pGrabPin;
    CComQIPtr<IObjectWithSite> pObjectWithSite;
    NoDirectVobSub graphBuilderCallback;
    VIDEOINFOHEADER vih;

    void OnDumpSample(REFERENCE_TIME SampleTime, BYTE* pBuffer, long BufferSize, BITMAPINFOHEADER* bih) override {
        LONGLONG time = SampleTime / 10000000;

        auto* frame = new DirectshowVideoFrame2(pBuffer, BufferSize, time, bih->biWidth, bih->biHeight);
        setCurrentFrame(frame);
        //Grab = false;
    }

protected:
    std::unique_ptr<DirectshowVideoFrame2> currentFrame_;
};


DirectshowFrameGrabber2::DirectshowFrameGrabber2() :
    duration_(0),
    d_ptr(std::make_unique<DirectshowFrameGrabber2Private>()) {}

bool DirectshowFrameGrabber2::open(const std::string& fileName) {
    thread_local COMInitializer comInitializer(COM_MULTI_THREADED);
    CString fileNameW = U2W(fileName);

    HRESULT hr;
    d_ptr->grab = std::make_unique<CGrab>();
    CGrabFilter* grabFilter = d_ptr->grab->filter();
    d_ptr->pGrabberBase = grabFilter;
    grabFilter->SetCallback(d_ptr.get());

    // Create the file reader
    d_ptr->pSource.CoCreateInstance(CLSID_AsyncReader);
    if (!d_ptr->pSource) {
        LOG(ERROR) << "Couldn't create CLSID_AsyncReader instance.";
        return false;
    }

    // Create the graph
    //
    d_ptr->pGraph.CoCreateInstance(CLSID_FilterGraph);
    if (!d_ptr->pGraph) {
        LOG(ERROR) << "Couldn't create CLSID_FilterGraph instance.";
        return false;
    }
    d_ptr->pObjectWithSite = d_ptr->pGraph;
    d_ptr->pObjectWithSite->SetSite(&d_ptr->graphBuilderCallback);

    // Put them in the graph
    //
    hr = d_ptr->pGraph->AddFilter(d_ptr->pSource, L"Source");

    if (FAILED(hr)) {
        LOG(ERROR) << "Couldn't load codec" << DirectShowUtil::GetMessageForHresult(hr);
        return false;
    }

    // add to graph -- nb need to Query properly for the
    // right interface before giving that to the graph object
    CComQIPtr<IBaseFilter, &IID_IBaseFilter> pDumpFilter(grabFilter);
    if (SUCCEEDED(hr)) {
        hr = d_ptr->pGraph->AddFilter(pDumpFilter, L"CustomRenderer");
    }

    // Load the source
    d_ptr->pLoad = (d_ptr->pSource);
    hr = d_ptr->pLoad->Load(fileNameW, nullptr);

    if (FAILED(hr)) {
        LOG(ERROR) << "Failed to load source file '" << fileName << "'" << DirectShowUtil::GetMessageForHresult(hr);
        return false;
    }

    // Get the output pin and the input pin
    d_ptr->pSourcePin = DirectShowUtil::GetOutPin(d_ptr->pSource, 0);;
    d_ptr->pGrabPin = DirectShowUtil::GetInPin(d_ptr->pGrabberBase, 0);

    // ... and connect them
    hr = d_ptr->pGraph->Connect(d_ptr->pSourcePin, d_ptr->pGrabPin);

    if (FAILED(hr)) {

        LOG(ERROR) << _(
                "Cannot connect filters (format probably is not supported).\nMake sure you have the required codecs installed on your system.")
            << std::endl << DirectShowUtil::GetMessageForHresult(hr);

        return false;
    }

    // Get the seeking interface, so we can seek to a location
    d_ptr->pSeeking = d_ptr->pGraph;

    // Query the graph for the IVideoWindow interface and use it to
    // disable AutoShow.  This will prevent the ActiveMovie window from
    // being displayed while we grab bitmaps from the running movie.
    d_ptr->pWindow = d_ptr->pGraph;
    if (d_ptr->pWindow) {
        hr = d_ptr->pWindow->put_AutoShow(OAFALSE);
    }

    // Find a limited number of frames
    LONGLONG duration;
    d_ptr->pSeeking->GetDuration(&duration);
    if (duration == 0) {
        LOG(ERROR) << "Cannot determine stream's length.";
        return false;
    }

    duration_ = duration;

    d_ptr->pControl = d_ptr->pGraph;
    d_ptr->pEvent = d_ptr->pGraph;
    return true;
}

bool DirectshowFrameGrabber2::seek(int64_t time) {
    HRESULT hr;
    long EvCode = 0;
    int i = 0;
    d_ptr->setCurrentFrame(nullptr);

    // set position
    REFERENCE_TIME Start = time;
    hr = d_ptr->pSeeking->SetPositions(&Start, AM_SEEKING_AbsolutePositioning | AM_SEEKING_SeekToKeyFrame, 0,
                                       AM_SEEKING_NoPositioning);
    if (FAILED(hr)) {
        LOG(WARNING) << "pSeeking->SetPositions failed." << DirectShowUtil::GetMessageForHresult(hr);
    }
    d_ptr->grab->filter()->SetGrab(true);
    hr = d_ptr->pControl->Run();
    if (FAILED(hr)) {
        LOG(WARNING) << "pControl->Run() failed." << DirectShowUtil::GetMessageForHresult(hr);
    }
    //Sleep(1000);
    hr = d_ptr->pEvent->WaitForCompletion(INFINITE, &EvCode);
    if (FAILED(hr)) {
        LOG(WARNING) << "pEvent->WaitForCompletion() failed." << DirectShowUtil::GetMessageForHresult(hr);
    }
    d_ptr->pControl->Stop();
    if (FAILED(hr)) {
        LOG(WARNING) << "pControl->Stop() failed." << DirectShowUtil::GetMessageForHresult(hr);
    }

    return true;
}

AbstractVideoFrame* DirectshowFrameGrabber2::grabCurrentFrame() {
    return d_ptr->currentFrame();
}

int64_t DirectshowFrameGrabber2::duration() {
    return duration_;
}

void DirectshowFrameGrabber2::abort() {}

DirectshowFrameGrabber2::~DirectshowFrameGrabber2() {
    if (d_ptr->pControl) {
        d_ptr->pControl->Stop();
    }
}
