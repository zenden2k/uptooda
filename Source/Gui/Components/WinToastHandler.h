#pragma once

#include "3rdpart/wintoastlib.h"

class WinToastHandler : public WinToastLib::IWinToastHandler {
public:
    WinToastHandler() = default;
    // Public interfaces
    void toastActivated() const override;
    void toastActivated(int actionIndex) const override;
    void toastDismissed(WinToastDismissalReason state) const override;
    void toastFailed() const override;
};
