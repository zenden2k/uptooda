#include "HotkeyList.h"

#include "Gui/Dialogs/FloatingWindow.h"
#include "Func/WinUtils.h"
#include "Gui/Dialogs/ScreenRecorderWindow.h"
#include "3rdpart/vkCodes.h"

CString MYHOTKEY::Serialize() const {
    DWORD data = MAKELONG(keyCode, keyModifier);
    CString res;
    res.Format(_T("%u"), data);
    return res;
}

bool MYHOTKEY::DeSerialize(const CString& data) {
    DWORD key = _tstoi(data);
    keyCode = LOWORD(key);
    keyModifier = HIWORD(key);
    return true;
}

CString MYHOTKEY::getKeyName(UINT vk, BOOL fExtended) {
    LONG lScan = MapVirtualKey(vk, 0) << 16;

    // if it's an extended key, add the extended flag
    if (fExtended)
        lScan |= 0x01000000L;

    // CString str;
    TCHAR buf[20];

    ::GetKeyNameTextW(lScan, buf, 20);
    return buf;  // str.c_str();
}

ACCEL MYHOTKEY::toAccel() const {
    ACCEL result;
    result.fVirt = 0;
    result.cmd = 0;
    if (keyModifier & MOD_ALT)
        result.fVirt |= FALT;
    if (keyModifier & MOD_CONTROL)
        result.fVirt |= FCONTROL;
    if (keyModifier & MOD_SHIFT)
        result.fVirt |= FSHIFT;
    result.fVirt |= FVIRTKEY;
    result.key = keyCode;
    return result;
}

CString MYHOTKEY::toString() const {
    CString res;
    HotkeyToString(keyCode, keyModifier, res );
    return res;
}

CHotkeyList::CHotkeyList()
{
#undef TR
#define TR
    m_bChanged = false;
    add(TR("Do nothing"), _T(""), 0);
    add(TR("Tray icon context menu"), _T("contextmenu"), IDM_CONTEXTMENU);
    add(TR("Upload images"),_T("addimages"), IDM_UPLOADIMAGES);
    add(TR("Upload files"),_T("addimages"), IDM_UPLOADFILES);
    add(TR("Upload folder"),_T("addfolder"), IDM_ADDFOLDERS);
    add(TR("Import Video File"),_T("importvideo"), IDM_IMPORTVIDEO);
    add(TR("Screenshot"),_T("screenshotdlg"), IDM_SCREENSHOTDLG);
    add(TR("Capture Rectangular Region"),_T("regionscreenshot"), IDM_REGIONSCREENSHOT, false);
    add(TR("Capture the Entire Screen"),_T("fullscreenshot"), IDM_FULLSCREENSHOT, false);
    add(TR("Capture the Active Window"),_T("windowscreenshot"), IDM_ACTIVEWINDOWSCREENSHOT, false);
    add(TR("Capture Selected Object"),_T("windowhandlescreenshot"), IDM_WINDOWHANDLESCREENSHOT, false);
    add(TR("Capture Selected Window"), _T("topwindowscreenshot"), IDM_TOPWINDOWSCREENSHOT, false);
    add(TR("Capture Last Region"), _T("lastregionscreenshot"), IDM_LASTREGIONSCREENSHOT, false);
    add(TR("Freehand Capture"),_T("freeformscreenshot"), IDM_FREEFORMSCREENSHOT, false);
    add(TR("Screen Recording Window"), _T("screenrecordingdlg"), IDM_SCREENRECORDINGDIALOG, false);
    add(TR("Screen Recording (start)"), _T("screenrecording"), IDM_SCREENRECORDINGSTART, false);
    add(TR("Screen Recording (stop)"), _T("screenrecording_stop"), ScreenRecorderWindow::ID_STOP, false, 0, 0, HOTKEY_GROUP_SCREEN_RECORDER_WINDOW);
    add(TR("Screen Recording (pause)"), _T("screenrecording_pause"), ScreenRecorderWindow::ID_PAUSE, false, 0, 0, HOTKEY_GROUP_SCREEN_RECORDER_WINDOW);
    add(TR("Screen Recording (cancel)"), _T("screenrecording_cancel"), IDCANCEL, false, 0, 0, HOTKEY_GROUP_SCREEN_RECORDER_WINDOW);
    add(TR("Show program window"),_T("showmainwindow"), IDM_SHOWAPPWINDOW);
    add(TR("Open screenshots folder"), _T("open_screenshot_folder"), IDM_OPENSCREENSHOTSFOLDER);
    add(TR("Settings"),_T("settings"), IDM_SETTINGS);
    add(TR("Paste"),_T("paste"), IDM_PASTEFROMCLIPBOARD,true,0x56, MOD_CONTROL); // Ctrl+V keyboard shortcut
    add(TR("Images from web"),_T("downloadimages"), IDM_PASTEFROMWEB);
    add(TR("View Media File Information"),_T("mediainfo"), IDM_MEDIAINFO);
    add(TR("Quick upload image from clipboard"), _T("uploadfromclipboard"), IDM_QUICKUPLOADFROMCLIPBOARD);
    add(TR("Shorten a Link"),_T("shortenurl"), IDM_SHORTENURL);
    add(TR("Shorten a link from the clipboard"),_T("shortenurlclipboard"), IDM_SHORTENURLCLIPBOARD);
    add(TR("Reupload images"),_T("reuploadimages"), IDM_REUPLOADIMAGES);
    add(TR("Exit"), _T("exit"), IDM_EXIT);
}

CHotkeyItem& CHotkeyList::getByFunc(const CString &func)
{
    for(size_t i=0; i<size(); i++)
    {
        if ((*this)[i].func == func) return  (*this)[i];
    }
    return (*this)[0];
}

int CHotkeyList::getFuncIndex(const CString &func) const {
    for(size_t i=0; i<size(); i++)
    {
        if ((*this)[i].func == func) return  i;
    }
    return -1;
}

int CHotkeyList::findByKey(WORD keyCode, WORD keyModifier) {
    for (size_t i=0; i<size(); i++) {
        CHotkeyItem& item = (*this)[i];
        if (item.globalKey.keyCode == keyCode && item.globalKey.keyModifier == keyModifier) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

bool CHotkeyList::operator==(const CHotkeyList& other) const {
    if (this == &other) {
        return true;
    }

    if(size() != other.size()) {
        return false;
    }

    for(size_t i = 0; i < other.size(); i++){
        const auto& item = (*this)[i];
        if(item.localKey != other[i].localKey || item.globalKey != other[i].globalKey) {
            return false;
        }
    }
    return true;
}

bool CHotkeyList::operator!=(const CHotkeyList& other) const {
    return !(*this == other);
}

void CHotkeyList::add(CString name, CString func, DWORD commandId, bool setForegroundWindow, WORD code, WORD modif, int groupId)
{
    CHotkeyItem hi;
    hi.localKey.keyCode = code;
    hi.localKey.keyModifier = modif;

    hi.func = func;
    hi.name = name;
    hi.commandId = commandId;
    hi.setForegroundWindow = setForegroundWindow;
    hi.groupId = groupId;
    push_back(hi);
}

bool CHotkeyList::changed() const
{
    return m_bChanged;
}

CHotkeyList& CHotkeyList::operator=( const CHotkeyList& c)
{
    if (this != &c) {
        clear();
        for (size_t i = 0; i < c.size(); i++) {
            push_back(c[i]);
        }
        m_bChanged = true;
    }
    return *this;
}

CString CHotkeyList::toString() const {
    CString result;
    for (size_t i = 1; i < size(); i++) {
        if (!((*this)[i].IsNull()))
            result += CString((*this)[i].func) + _T("=") + (*this)[i].localKey.Serialize() + _T(",") + (*this)[i].
                globalKey.Serialize() + _T(";");
    }
    return result;
}

bool CHotkeyList::deserialize(const CString &data)
{
    TCHAR hotkey[200];
    int i =0;

    while(WinUtils::ExtractStrFromList(
           data /* Source string */,
            i++, /* Zero based item index */
            hotkey /* Destination buffer */,
            sizeof(hotkey)/sizeof(TCHAR), /* Length in characters of destionation buffer */
            _T(""),
            _T(';')))
    {
        TCHAR funcName[30]=_T("");
        TCHAR localKeyStr[20] = _T(""), globalKeyStr[20] = _T("");

        WinUtils::ExtractStrFromList(hotkey , 0, funcName,sizeof(funcName)/sizeof(TCHAR), _T(""),_T('='));
        //(*this)[i].localKey.DeSerialize(localKeyStr);

        int cur = getFuncIndex(funcName);
        if(cur<0) continue;

        //(*this)[i].func = funcName;

        WinUtils::ExtractStrFromList(hotkey , 1, funcName,sizeof(funcName)/sizeof(TCHAR), _T(""),_T('='));
        (*this)[cur].localKey.DeSerialize(localKeyStr);

        WinUtils::ExtractStrFromList(funcName , 0, localKeyStr,sizeof(localKeyStr)/sizeof(TCHAR), _T(""),_T(','));
        (*this)[cur].localKey.DeSerialize(localKeyStr);

        WinUtils::ExtractStrFromList(funcName ,1, globalKeyStr ,sizeof(globalKeyStr)/sizeof(TCHAR),_T(""),_T(','));
        (*this)[cur].globalKey.DeSerialize(globalKeyStr);

    }

    return true;
}
