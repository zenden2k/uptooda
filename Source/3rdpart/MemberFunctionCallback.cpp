#include "MemberFunctionCallback.h"
#include <mutex>
#include "Core/Logging.h"

CBTHookCallbackBase* AvailableCallbackSlots[kMaxCallbacks] = {
    new DynamicCBTHookCallback<0x00>(),
    new DynamicCBTHookCallback<0x01>(),
    new DynamicCBTHookCallback<0x02>(),
    new DynamicCBTHookCallback<0x03>(),
    new DynamicCBTHookCallback<0x04>()
    /*new DynamicCBTHookCallback<0x05>(),
    new DynamicCBTHookCallback<0x06>(),
    new DynamicCBTHookCallback<0x07>(),
    new DynamicCBTHookCallback<0x08>(),
    new DynamicCBTHookCallback<0x09>(),
    new DynamicCBTHookCallback<0x0A>(),
    new DynamicCBTHookCallback<0x0B>(),
    new DynamicCBTHookCallback<0x0C>(),
    new DynamicCBTHookCallback<0x0D>(),
    new DynamicCBTHookCallback<0x0E>(),
    new DynamicCBTHookCallback<0x0F>(),*/
};
std::mutex AvailableCallbackSlotsMutex;

struct Dummy {
    ~Dummy() {
        for (auto it : AvailableCallbackSlots) {
            delete it;
        }
    }
};

Dummy dummyObject;

CBTHookMemberFunctionCallback::CBTHookMemberFunctionCallback(const HookCallback& method) {
    std::lock_guard<std::mutex> lock(AvailableCallbackSlotsMutex);
    constexpr int imax = std::size(AvailableCallbackSlots);
    for (m_nAllocIndex = 0; m_nAllocIndex < imax; ++m_nAllocIndex) {
        m_cbCallback = AvailableCallbackSlots[m_nAllocIndex]->Reserve(method);
        if (m_cbCallback != nullptr) {
            return;
        }
    }
    LOG(ERROR) << "Cannot create member function callback";
}

CBTHookMemberFunctionCallback::~CBTHookMemberFunctionCallback() {
    if (IsValid()) {
        std::lock_guard<std::mutex> lock(AvailableCallbackSlotsMutex);
        AvailableCallbackSlots[m_nAllocIndex]->Free();
    }
}
