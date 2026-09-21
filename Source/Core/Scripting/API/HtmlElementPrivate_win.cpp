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

#include "HtmlElementPrivate_win.h"
#include "Func/WinUtils.h"

namespace ScriptAPI {

CComQIPtr<IHTMLElement> AccessibleToHTMLElement(IAccessible* pAccessible) {
    ATLASSERT(pAccessible != nullptr);

    // Query for IServiceProvider interface.
    CComQIPtr<IServiceProvider> spServProvider = pAccessible;
    if (spServProvider != nullptr) {
        // Ask the service for a IHTMLElement object.
        CComQIPtr<IHTMLElement> spHtmlElement;
        /*HRESULT hRes = */
        spServProvider->QueryService(IID_IHTMLElement, IID_IHTMLElement,
                                     (void**)&spHtmlElement);

        return spHtmlElement;
    }

    return {};
}

void HtmlElementPrivate::setValue(const std::string& value) {
    CComQIPtr<IHTMLInputElement> input = disp_
                                             ? CComQIPtr<IHTMLInputElement>(disp_)
                                             : CComQIPtr<IHTMLInputElement>(elem_);
    if (!input) {
        LOG(WARNING) << "setValue: element is not an input.";
        return;
    }

    input->put_value(CComBSTR(IuCoreUtils::Utf8ToWstring(value).c_str()));
}

std::string HtmlElementPrivate::getValue() {
    CComQIPtr<IHTMLInputElement> input = disp_
                                             ? CComQIPtr<IHTMLInputElement>(disp_)
                                             : CComQIPtr<IHTMLInputElement>(elem_);
    if (!input) {
        LOG(WARNING) << "getValue: element is not an input.";
        return {};
    }

    CComBSTR res;
    if (SUCCEEDED(input->get_value(&res)) && res) {
        return IuCoreUtils::WstringToUtf8(static_cast<OLECHAR*>(res));
    }
    return {};
}

CComQIPtr<IAccessible> HTMLElementToAccessible(IHTMLElement* pHtmlElement) {
    ATLASSERT(pHtmlElement != nullptr);

    // Query for IServiceProvider interface.
    CComQIPtr<IServiceProvider> spServProvider = pHtmlElement;
    if (spServProvider != nullptr) {
        // Ask the service for a IAccessible object.
        CComQIPtr<IAccessible> spAccessible;
        /*HRESULT hRes = */
        spServProvider->QueryService(IID_IAccessible, IID_IAccessible,
                                     (void**)&spAccessible);

        return spAccessible;
    }

    return {};
}

}
