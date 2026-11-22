#pragma once

#include "atlheaders.h"

#include <richedit.h>
#include "resource.h"

class ServerData;

class ServerInformationWindow : public CDialogImpl<ServerInformationWindow>,
                                public CDialogResize<ServerInformationWindow> {
public:
    enum { IDD = IDD_SERVERINFORMATION };

    explicit ServerInformationWindow(const ServerData& server);
    void showModal(HWND parent);

    BEGIN_MSG_MAP(ServerInformationWindow)
    MESSAGE_HANDLER(WM_INITDIALOG, onInitDialog)
    MESSAGE_HANDLER(WM_SYSCOLORCHANGE, onSystemColorChange)
    NOTIFY_HANDLER(IDC_SERVERINFORMATIONTEXT, EN_LINK, onLink)
    COMMAND_ID_HANDLER(IDCANCEL, onClose)
    CHAIN_MSG_MAP(CDialogResize<ServerInformationWindow>)
    END_MSG_MAP()

    BEGIN_DLGRESIZE_MAP(ServerInformationWindow)
    DLGRESIZE_CONTROL(IDC_SERVERINFORMATIONTEXT, DLSZ_SIZE_X | DLSZ_SIZE_Y)
    DLGRESIZE_CONTROL(IDCANCEL, DLSZ_MOVE_X | DLSZ_MOVE_Y)
    END_DLGRESIZE_MAP()

private:
    void populateText();
    LRESULT onInitDialog(UINT, WPARAM, LPARAM, BOOL&);
    LRESULT onSystemColorChange(UINT, WPARAM, LPARAM, BOOL& handled);
    LRESULT onLink(int, LPNMHDR notification, BOOL& handled);
    LRESULT onClose(WORD, WORD, HWND, BOOL&);

    const ServerData& server_;
    CWindow textControl_;
};
