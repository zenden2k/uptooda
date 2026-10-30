#ifndef UPTOODA_HOTKEYLIST_H
#define UPTOODA_HOTKEYLIST_H

#include <vector>

#include "atlheaders.h"

constexpr auto HOTKEY_GROUP_APP = 0;
constexpr auto HOTKEY_GROUP_SCREEN_RECORDER_WINDOW = 1;

struct MYHOTKEY {
    CString Serialize() const;

    bool DeSerialize(const CString& data);

    bool operator!=(const MYHOTKEY& c) const {
        return (keyCode != c.keyCode || keyModifier != c.keyModifier);
    }

    static CString getKeyName(UINT vk, BOOL fExtended);

    ACCEL toAccel() const;

    CString toString() const;

    WORD keyCode = 0;
    WORD keyModifier = 0;
};

class CHotkeyItem {
public:
    MYHOTKEY localKey;
    MYHOTKEY globalKey;
    CString func;
    DWORD commandId;
    CString name;
    bool setForegroundWindow;
    int groupId;

    CString GetDisplayName() const;

    CHotkeyItem() {
        localKey.keyCode = 0;
        localKey.keyModifier = 0;
        globalKey.keyCode = 0;
        globalKey.keyModifier = 0;
        setForegroundWindow = true;
        commandId = 0;
        groupId = 0;
    }

    void Clear() {
        localKey.keyCode = 0;
        localKey.keyModifier = 0;
        globalKey.keyCode = 0;
        globalKey.keyModifier = 0;
        setForegroundWindow = true;
    }

    bool IsNull() const {
        return (localKey.keyCode == 0 && globalKey.keyCode == 0);
    }
};

class CHotkeyList : public std::vector<CHotkeyItem> {
public:
    CHotkeyList();
    CHotkeyList(const CHotkeyList&) = delete;
    bool changed() const;
    CHotkeyItem& getByFunc(const CString& func);
    void add(CString name, CString func, DWORD commandId, bool setForegroundWindow = true, WORD Code = 0,
             WORD modif = 0, int groupId = 0);
    CHotkeyList& operator=(const CHotkeyList&);
    bool operator==(const CHotkeyList&) const;
    bool operator!=(const CHotkeyList&) const;
    CString toString() const;
    bool deserialize(const CString& data);
    int getFuncIndex(const CString& func) const;
    int findByKey(WORD keyCode, WORD keyModifier);

private:
    bool m_bChanged;
};

#endif //UPTOODA_HOTKEYLIST_H
