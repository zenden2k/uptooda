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

#include "HtmlDocumentPrivate_win.h"

namespace ScriptAPI {;

HtmlElement HtmlDocumentPrivate::rootElement() {
    if ( rootElement_.isNull() ) {
        CComPtr<IHTMLElementCollection> collection;
        CComBSTR tagBstr = _T("html");
        doc3_->getElementsByTagName(tagBstr, &collection);
        long count  = 0;
        if ( !SUCCEEDED(collection->get_length(&count)) || !count) {
            return {};
        }

        IDispatchPtr  disp = 0;
        collection->item(CComVariant(0), CComVariant(0), &disp);
        rootElement_ = new HtmlElementPrivate(disp, this);
        return rootElement_;
    }
    return rootElement_;
}

HtmlElement HtmlDocumentPrivate::getElementById(const std::string& id) {
    CComPtr<IHTMLElement> elem;
    CComBSTR idBstr = IuCoreUtils::Utf8ToWstring(id).c_str();
    if ( SUCCEEDED( doc3_->getElementById(idBstr, &elem))  && elem  ) {
        return new HtmlElementPrivate(elem, this);
    }
    return {};
}

Sqrat::Array HtmlDocumentPrivate::getElementsByTagName(const std::string& tag) {
    CComPtr<IHTMLElementCollection> collection;
    CComBSTR tagBstr = IuCoreUtils::Utf8ToWstring(tag).c_str();
    doc3_->getElementsByTagName(tagBstr, &collection);
    long count  = 0;
    if ( !SUCCEEDED(collection->get_length(&count))) {
        return {};
    }
    Sqrat::Array res(GetCurrentThreadVM(), count);
    for ( int i = 0; i < count; i ++ ) {
        IDispatchPtr  disp = 0;
        collection->item(CComVariant(i), CComVariant(0), &disp);
        res.SetValue(i, HtmlElement(new HtmlElementPrivate(disp, this)));
    }
    return res;
}

Sqrat::Array HtmlDocumentPrivate::getElementsByName(const std::string& name) {
    CComPtr<IHTMLElementCollection> collection;
    CComBSTR tagBstr = IuCoreUtils::Utf8ToWstring(name).c_str();
    doc3_->getElementsByName(tagBstr, &collection);
    long count  = 0;
    if ( !SUCCEEDED(collection->get_length(&count))) {
        return {};
    }
    Sqrat::Array res(GetCurrentThreadVM(), count);
    for ( int i = 0; i < count; i ++ ) {
        IDispatchPtr  disp = 0;
        collection->item(CComVariant(i), CComVariant(0), &disp);
        res.SetValue(i, HtmlElement(new HtmlElementPrivate(disp, this)));
    }
    return res;
}

}
