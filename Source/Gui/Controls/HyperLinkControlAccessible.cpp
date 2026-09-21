#include "HyperLinkControlAccessible.h"

#include "HyperLinkControl.h"
#include "Core/i18n/Translator.h"

CHyperLinkControlAccessible::CHyperLinkControlAccessible() : alive_(TRUE), control_(nullptr){
    
}

/*HRESULT CHyperLinkControlAccessible::QueryInterface(const IID& riid, void** ppvObject) {
    // Always set out parameter to NULL, validating it first.
    if (!ppvObject)
        return E_INVALIDARG;
    *ppvObject = NULL;
    if (riid == IID_IUnknown || riid == IID_IDispatch || riid == IID_IAccessible) {
        // Increment the reference count and return the pointer.
        *ppvObject = (LPVOID)this;
        AddRef();
        return NOERROR;
    }
    return E_NOINTERFACE;
}

ULONG CHyperLinkControlAccessible::AddRef() {
    InterlockedIncrement(&refCount_);
    return refCount_;
}

ULONG CHyperLinkControlAccessible::Release() {
    ULONG ulRefCount = InterlockedDecrement(&refCount_);
    if (0 == refCount_) {
        delete this;
    }
    return ulRefCount;
}*/

STDMETHODIMP CHyperLinkControlAccessible::GetTypeInfoCount(UINT* pctinfo) {
    *pctinfo = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CHyperLinkControlAccessible::GetTypeInfo(UINT iTInfo, LCID lcid, ITypeInfo** ppTInfo) {
    *ppTInfo = NULL;
    return E_NOTIMPL;
}

STDMETHODIMP CHyperLinkControlAccessible::GetIDsOfNames(const IID& riid, LPOLESTR* rgszNames, UINT cNames, LCID lcid, DISPID* rgDispId) {
    return E_NOTIMPL;
}

STDMETHODIMP CHyperLinkControlAccessible::Invoke(DISPID dispIdMember, const IID& riid, LCID lcid, WORD wFlags, DISPPARAMS* pDispParams, VARIANT* pVarResult, EXCEPINFO* pExcepInfo, UINT* puArgErr) {
    return E_NOTIMPL;
}

STDMETHODIMP CHyperLinkControlAccessible::get_accParent(IDispatch** ppdispParent) {
    return pBase_->get_accParent(ppdispParent);
}

STDMETHODIMP CHyperLinkControlAccessible::get_accChildCount(long* pcountChildren) {
    HRESULT hr = CheckAlive();
    if (SUCCEEDED(hr)) {
        *pcountChildren = static_cast<long>(control_->ItemCount());
    }
    return hr;
}

STDMETHODIMP CHyperLinkControlAccessible::get_accChild(VARIANT varChild, IDispatch** ppdispChild) {
    // This IAccessible doesn't use Child IDs, so this just does param
    // validation, and returns S_OK with *ppdispChild as NULL.
    *ppdispChild = NULL;
    HRESULT hr = ValidateChildId(varChild);
    if (SUCCEEDED(hr)) {
        hr = S_OK;
    }
    return hr;
}

STDMETHODIMP CHyperLinkControlAccessible::get_accName(VARIANT varChild, BSTR* pszName) {
    // Let the base IAccessible handle the name of the overall control - 
    // default support in OLEACC will look for a label if the control
    // is in a dialog. Otherwise, return the string for the appropriate
    // child item.
    *pszName = NULL;
    HRESULT hr = ValidateChildId(varChild);
    if (SUCCEEDED(hr)) {
        if (varChild.lVal == CHILDID_SELF) {
            *pszName = SysAllocString(TR("Select action:"));
            if (*pszName == NULL) {
                hr = E_OUTOFMEMORY;
            }
            //hr = pBase_->get_accName(varChild, pszName);
        } else {
            *pszName = SysAllocString(control_->GetItemTitle(varChild.lVal - 1));
            if (*pszName == NULL) {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    return hr;
}

STDMETHODIMP CHyperLinkControlAccessible::get_accValue(VARIANT varChild, BSTR* pszValue) {
    *pszValue = NULL;
    HRESULT hr = ValidateChildId(varChild);
    return hr;
}

STDMETHODIMP CHyperLinkControlAccessible::get_accDescription(VARIANT varChild, BSTR* pszDescription) {
    *pszDescription = NULL;
    HRESULT hr = ValidateChildId(varChild);
    if (SUCCEEDED(hr)) {
        if (varChild.lVal == CHILDID_SELF) {
            hr = pBase_->get_accName(varChild, pszDescription);
        } else {
            *pszDescription = SysAllocString(control_->GetItemDescription(varChild.lVal - 1));
            if (*pszDescription == NULL) {
                hr = E_OUTOFMEMORY;
            }
        }
    }
    return hr;
}

STDMETHODIMP CHyperLinkControlAccessible::get_accRole(VARIANT varChild, VARIANT* pvarRole) {
    HRESULT hr = ValidateChildId(varChild);
    if (SUCCEEDED(hr)) {
        pvarRole->vt = VT_I4;

        if (varChild.lVal == CHILDID_SELF) {
            pvarRole->lVal = ROLE_SYSTEM_PANE;
        } else {
            pvarRole->lVal = ROLE_SYSTEM_LINK;
        }
    }
    return S_OK;

}

STDMETHODIMP CHyperLinkControlAccessible::get_accState(VARIANT varChild, VARIANT* pvarState) {
    HRESULT hr = ValidateChildId(varChild);
    if (SUCCEEDED(hr)) {
        if (varChild.lVal == CHILDID_SELF) {
            pvarState->vt = VT_I4;
            pvarState->lVal = 0;
        } else  // For list items.
        {
            DWORD flags =  STATE_SYSTEM_FOCUSABLE;
            int index = (int)varChild.lVal - 1;
            if (index == control_->SelectedIndex()) {
                flags |= STATE_SYSTEM_FOCUSED;
            }
            pvarState->vt = VT_I4;
            pvarState->lVal = flags;
        }
    }
    return S_OK;
}

STDMETHODIMP CHyperLinkControlAccessible::get_accHelp(VARIANT varChild, BSTR* pszHelp) {
    *pszHelp = NULL;
    return S_OK;
}

STDMETHODIMP CHyperLinkControlAccessible::get_accHelpTopic(BSTR* pszHelpFile, VARIANT varChild, long* pidTopic) {
    *pidTopic = NULL;
    return S_OK;
}

STDMETHODIMP CHyperLinkControlAccessible::get_accKeyboardShortcut(VARIANT varChild, BSTR* pszKeyboardShortcut) {
    *pszKeyboardShortcut = NULL;
    return S_OK;
}

STDMETHODIMP CHyperLinkControlAccessible::get_accFocus(VARIANT* pvarChild) {
    int selectedIndex = control_->SelectedIndex();
    if (selectedIndex == -1) {
        pvarChild->vt = VT_EMPTY;
    } else {
        pvarChild->vt = VT_I4;
        pvarChild->lVal = selectedIndex + 1;
    }
    return S_OK;
}

STDMETHODIMP CHyperLinkControlAccessible::get_accSelection(VARIANT* pvarChildren) {
    pvarChildren->vt = VT_EMPTY;
    return S_OK;
}

STDMETHODIMP CHyperLinkControlAccessible::get_accDefaultAction(VARIANT varChild, BSTR* pszDefaultAction) {
    HRESULT hr = ValidateChildId(varChild);
    if (SUCCEEDED(hr)) {
        if (varChild.lVal == CHILDID_SELF) {
            *pszDefaultAction = SysAllocString(L"None.");
        } else {
            *pszDefaultAction = SysAllocString(L"Click");
        }
    }
    return S_OK;
}

STDMETHODIMP CHyperLinkControlAccessible::accSelect(long flagsSelect, VARIANT varChild) {
    HRESULT hr = ValidateChildId(varChild);
    if (SUCCEEDED(hr)) {
        if (varChild.lVal != CHILDID_SELF && (flagsSelect & SELFLAG_TAKEFOCUS)) {
            int itemIndex = varChild.lVal - 1;
            control_->SelectItem(itemIndex);
        }
    }
    
    return S_OK;
}

/**
 * 	The IAccessible::accLocation method retrieves the specified object's current screen location. 
 * 	All visual objects must support this method. Sound objects do not support this method.
 */
STDMETHODIMP CHyperLinkControlAccessible::accLocation(long* pxLeft, long* pyTop, long* pcxWidth, long* pcyHeight, VARIANT varChild) {
    *pxLeft = *pyTop = *pcxWidth = *pcyHeight = 0;
    HRESULT hr = ValidateChildId(varChild);
    if (SUCCEEDED(hr)) {
        if (varChild.lVal == CHILDID_SELF) {
            hr = pBase_->accLocation(pxLeft, pyTop, pcxWidth, pcyHeight, varChild);
        } else {
            RECT rc = control_->GetItemRect(varChild.lVal - 1);
            MapWindowPoints(control_->m_hWnd,
                NULL, (POINT *)&rc, 2);
            *pxLeft = rc.left;
            *pyTop = rc.top;
            *pcxWidth = rc.right - rc.left;
            *pcyHeight = rc.bottom - rc.top;
        }
    }
    return hr;
}

STDMETHODIMP CHyperLinkControlAccessible::accNavigate(long navDir, VARIANT varStart, VARIANT* pvarEndUpAt) {
    pvarEndUpAt->vt = VT_EMPTY;
    HRESULT hr = ValidateChildId(varStart);
    if (SUCCEEDED(hr)) {
        if (varStart.lVal == CHILDID_SELF) {
            if (navDir == NAVDIR_FIRSTCHILD) {
                if (control_->ItemCount() == 0) {
                    // No child objects to navigate to.
                    hr = S_FALSE;
                } else {
                    pvarEndUpAt->vt = VT_I4;
                    pvarEndUpAt->lVal = 1;
                }
            } else if (navDir == NAVDIR_LASTCHILD) {
                if (control_->ItemCount() == 0) {
                    // No child objects to navigate to.
                    hr = S_FALSE;
                } else {
                    pvarEndUpAt->vt = VT_I4;
                    pvarEndUpAt->lVal = control_->ItemCount();
                }
            } else {
                hr = pBase_->accNavigate(navDir, varStart, pvarEndUpAt);
            }
        } else {
            if (navDir == NAVDIR_DOWN || navDir == NAVDIR_NEXT) {
                /*UINT cItems = control_->ItemCount();
                if (cItems > 1 && (UINT)varStart.lVal < cItems) {
                    pvarEndUpAt->vt = VT_I4;
                    pvarEndUpAt->lVal = varStart.lVal + 1;
                } else {
                    hr = S_FALSE;
                }*/
                return GetNextItem(varStart, pvarEndUpAt);
            } else if (navDir == NAVDIR_PREVIOUS || navDir == NAVDIR_UP) {
                return GetPrevItem(varStart, pvarEndUpAt);
            } else if (navDir == NAVDIR_LEFT || navDir == NAVDIR_RIGHT) {
                // Leave out param as VT_EMPTY from above.
                hr = S_FALSE;
            } else {
                hr = E_INVALIDARG;
            }
        }
    }
    return hr;
}

HRESULT CHyperLinkControlAccessible::GetNextItem(VARIANT varStart, VARIANT* pvarEndUpAt) {
    UINT cItems = control_->ItemCount();
    if (cItems > 1 && (UINT)varStart.lVal < cItems) {
        LONG itemIndex;
        for (itemIndex = varStart.lVal - 1; itemIndex + 1 < cItems; itemIndex++) {
            auto item = control_->getItem(itemIndex);
            if (item->Visible) {
                pvarEndUpAt->vt = VT_I4;
                pvarEndUpAt->lVal = itemIndex + 1;
                return S_OK;
            }
        }
        
    }
    return S_FALSE;
}
HRESULT CHyperLinkControlAccessible::GetPrevItem(VARIANT varStart, VARIANT* pvarEndUpAt) {
    if (varStart.lVal > 1) {
        LONG itemIndex;
        for (itemIndex = varStart.lVal - 1; itemIndex >= 0; itemIndex--) {
            auto item = control_->getItem(itemIndex);
            if (item->Visible) {
                pvarEndUpAt->vt = VT_I4;
                pvarEndUpAt->lVal = itemIndex + 1;
                return S_OK;
            }
        }
    }
    return S_FALSE;
}

STDMETHODIMP CHyperLinkControlAccessible::accHitTest(long xLeft, long yTop, VARIANT* pvarChild) {
    pvarChild->vt = VT_EMPTY;
    HRESULT hr = CheckAlive();
    if (SUCCEEDED(hr)) {
        hr = pBase_->accHitTest(xLeft, yTop, pvarChild);
        if (SUCCEEDED(hr) && pvarChild->vt == VT_I4 &&
            pvarChild->lVal == CHILDID_SELF) {
            POINT pt = { xLeft, yTop };
            MapWindowPoints(NULL, control_->m_hWnd, &pt, 1);
            UINT iItem = control_->ItemFromPoint(pt);
            if (iItem != (UINT)-1) {
                pvarChild->lVal = iItem + 1;
            }
        }
    }
    return hr;
}

STDMETHODIMP CHyperLinkControlAccessible::accDoDefaultAction(VARIANT varChild) {
    if (varChild.vt != VT_I4) {
        return E_INVALIDARG;
    }
    if (varChild.lVal != CHILDID_SELF) {
        int itemIndex = varChild.lVal - 1;
        // It is assumed that the control does its own checking to see which
        // item has the focus when it receives this message.
        control_->NotifyParent(itemIndex);
    }
    return S_OK;
}

STDMETHODIMP CHyperLinkControlAccessible::put_accName(VARIANT varChild, BSTR szName) {
    return E_NOTIMPL;
}

STDMETHODIMP CHyperLinkControlAccessible::put_accValue(VARIANT varChild, BSTR szValue) {
    return E_NOTIMPL;
}

HRESULT CHyperLinkControlAccessible::CheckAlive()
{
    return alive_ ? S_OK : RPC_E_DISCONNECTED;
}

HRESULT CHyperLinkControlAccessible::ValidateChildId(VARIANT& varChildId) {
    HRESULT hr = CheckAlive();
    if (SUCCEEDED(hr)) {
        if (varChildId.vt != VT_I4) {
            hr = E_INVALIDARG;
        } else if (varChildId.lVal != CHILDID_SELF) {
            if ((UINT)(varChildId.lVal - 1) >= control_->ItemCount()) {
                hr = E_INVALIDARG;
            } else {
                auto item = control_->getItem(varChildId.lVal - 1);
                if (!item->Visible) {
                    hr = E_INVALIDARG;
                }
            }
        }
    }
    return hr;
}

HRESULT CHyperLinkControlAccessible::Initialize(CHyperLinkControl* control) {
    control_ = control;
    return CreateStdAccessibleObject(control->m_hWnd, OBJID_CLIENT, IID_PPV_ARGS(&pBase_));
}

HRESULT CHyperLinkControlAccessible::Create(_In_ CHyperLinkControl* control, _COM_Outptr_ IAccessible** out)
{
    ATLENSURE_RETURN_HR(out != nullptr, E_POINTER);
    CComObject<CHyperLinkControlAccessible>* p = nullptr;
    auto hrCreate = ATL::CComObject<CHyperLinkControlAccessible>::CreateInstance(&p);
    ATLENSURE_RETURN_HR(SUCCEEDED(hrCreate), hrCreate);
    auto hrInit = p->Initialize(control);
    if (SUCCEEDED(hrInit)) {
        *out = p;
        p->AddRef();
    }
    return hrInit;
}
