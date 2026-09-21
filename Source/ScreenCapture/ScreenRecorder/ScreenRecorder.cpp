#include "ScreenRecorder.h"

ScreenRecorder::ScreenRecorder(std::string outFile, CRect rect):
                                    outFilePath_(std::move(outFile)),
                                    captureRect_(rect){
    
}

void ScreenRecorder::changeStatus(Status status) {
    status_ = status;
    onStatusChange_(status);
}

bool ScreenRecorder::isRunning() const {
    return isRunning_;
}

void ScreenRecorder::setOffset(int x, int y) {
    captureRect_ = CRect(x, y, x + captureRect_.Width(), y + captureRect_.Height());
}

ScreenRecorder::Status ScreenRecorder::status() const {
    return status_;
}

std::string ScreenRecorder::outFileName() const {
    return status_ == Status::Finished ? outFilePath_ : std::string();
}
