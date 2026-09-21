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

#ifndef IU_CORE_HTMLELEMENTPRIVATE_WIN_H
#define IU_CORE_HTMLELEMENTPRIVATE_WIN_H

#pragma once

#include "WebBrowserPrivate_win.h"
#include "HtmlDocumentPrivate_win.h"
#include "Core/Utils/CoreUtils.h"
#include "Core/Utils/StringUtils.h"
#include "atlheaders.h"
#include <ComDef.h>

#include "COMUtils.h"
#include "mshtml.h"


namespace ScriptAPI {
    /* @cond PRIVATE */
    // From IHTMLElement to IAccessible.
    CComQIPtr<IAccessible> HTMLElementToAccessible(IHTMLElement* pHtmlElement);
    /* @endcond */
class HtmlElementPrivate {
    public:

        HtmlElementPrivate(IHTMLElement* elem, HtmlDocumentPrivate *docPrivate) {
            elem_ = elem;
            selector_ = CComQIPtr<IElementSelector>(elem);
            elem2_ = CComQIPtr<IHTMLElement2>(elem);
            form_ = elem;
            docPrivate_= docPrivate;

        }

        HtmlElementPrivate(IDispatchPtr disp, HtmlDocumentPrivate *docPrivate) {
            disp_  = disp;
            elem_ = CComQIPtr<IHTMLElement,&IID_IHTMLElement>(disp);
            elem2_ = CComQIPtr<IHTMLElement2>(disp);
            selector_ = CComQIPtr<IElementSelector>(disp);
            form_ = disp;
            docPrivate_= docPrivate;
        }

        std::string getAttribute(const std::string& name)
        {
            if ( name == "class" || !IuStringUtils::StrCaseInsensitiveCompare(name, "class")) {
                return getClassName();
            }
            CComVariant res;
            if ( SUCCEEDED( elem_->getAttribute(CComBSTR(IuCoreUtils::Utf8ToWstring(name).c_str()), 0, &res) ) ) {
                return ComVariantToString(res);
            }
            return std::string();
        }

        std::string getClassName() {

            CComBSTR res;
            if ( SUCCEEDED( elem_->get_className(&res) ) && res  ) {
                return ComVariantToString(res);
            }
            return std::string();
        }

        void setClassName(const std::string& name) {
            elem_->put_className(CComBSTR(IuCoreUtils::Utf8ToWstring(name).c_str()));
        }
        void setAttribute(const std::string& name, const std::string& value)
        {
            if ( name == "class" || !IuStringUtils::StrCaseInsensitiveCompare(name, "name")) {
                return setClassName(value);
            }
            elem_->setAttribute(CComBSTR(IuCoreUtils::Utf8ToWstring(name).c_str()), CComVariant(IuCoreUtils::Utf8ToWstring(value).c_str()));
        }

        void removeAttribute(const std::string& name)
        {
            VARIANT_BOOL res;
            elem_->removeAttribute(CComBSTR(IuCoreUtils::Utf8ToWstring(name).c_str()), 0, &res);
        }

        std::string getId()
        {
            CComBSTR res;
            if ( SUCCEEDED ( elem_->get_id(&res) ) && res  ) {
                return IuCoreUtils::WstringToUtf8((OLECHAR*)res);
            }
            return std::string();
        }

        void setId(const std::string& id)
        {
            elem_->put_id((CComBSTR(IuCoreUtils::Utf8ToWstring(id).c_str())));
        }

        std::string getInnerHTML()
        {
            CComBSTR res;
            if ( SUCCEEDED( elem_->get_innerHTML(&res) ) && res  ) {
                return IuCoreUtils::WstringToUtf8((OLECHAR*)res);
            }
            return std::string();
        }

        void setInnerHTML(const std::string& html)
        {
            elem_->put_innerHTML(CComBSTR(IuCoreUtils::Utf8ToWstring(html).c_str()));
        }

        std::string getInnerText()
        {
            CComBSTR res;
            if ( SUCCEEDED( elem_->get_innerText(&res) ) && res  ) {
                return IuCoreUtils::WstringToUtf8((OLECHAR*)res);
            }
            return std::string();
        }

        void setInnerText(const std::string& text)
        {
            elem_->put_innerText(CComBSTR(IuCoreUtils::Utf8ToWstring(text).c_str()));
        }

        std::string getOuterHTML()
        {
            CComBSTR res;
            if ( SUCCEEDED( elem_->get_outerHTML(&res) ) && res   ) {
                return IuCoreUtils::WstringToUtf8((OLECHAR*)res);
            }
            return std::string();
        }

        void setOuterHTML(const std::string& html)
        {
            elem_->put_outerHTML(CComBSTR(IuCoreUtils::Utf8ToWstring(html).c_str()));
        }

        std::string getOuterText()
        {
            CComBSTR res;
            if ( SUCCEEDED( elem_->get_outerText(&res) )  && res  ) {
                return IuCoreUtils::WstringToUtf8((OLECHAR*)res);
            }
            return std::string();
        }

        void setOuterText(const std::string& text)
        {
            elem_->put_outerText(CComBSTR(IuCoreUtils::Utf8ToWstring(text).c_str()));
        }

        std::string getTagName()
        {
            CComBSTR res;
            if ( SUCCEEDED( elem_->get_tagName(&res) ) && res  ) {
                return IuCoreUtils::WstringToUtf8((OLECHAR*)res);
            }
            return std::string();
        }
        bool isNull() {
            return !elem_;
        }

        void setValue(const std::string& value);
        std::string getValue();

        HtmlElement getParentElement()
        {
            IHTMLElement* res;
            if ( SUCCEEDED( elem_->get_parentElement(&res) ) ) {
                return new HtmlElementPrivate(res, docPrivate_);
            }
            return HtmlElement();
        }

        void scrollIntoView()
        {
            elem_->scrollIntoView(CComVariant(TRUE));
        }

        void click()
        {
            elem_->click();
        }

        void insertHTML(const std::string& html, bool atEnd /*= false */)
        {
            elem_->insertAdjacentHTML(CComBSTR(atEnd ? L"afterBegin" : L"beforeEnd"), CComBSTR(IuCoreUtils::Utf8ToWstring(html).c_str()));
        }

        void insertText(const std::string& text, bool atEnd /*= false */)
        {
            elem_->insertAdjacentText(CComBSTR(atEnd ? L"afterBegin" : L"beforeEnd"), CComBSTR(IuCoreUtils::Utf8ToWstring(text).c_str()));
        }

        Sqrat::Array getChildren() {
            IDispatchPtr disp;
            elem_->get_children(&disp);
            CComQIPtr<IHTMLElementCollection> collection(disp);
            long count  = 0;
            if ( !SUCCEEDED(collection->get_length(&count))) {
                return Sqrat::Array();
            }
            Sqrat::Array res(GetCurrentThreadVM(), count);
            for ( int i = 0; i < count; i ++ ) {
                IDispatchPtr  disp = 0;
                collection->item(CComVariant(i), CComVariant(0), &disp);
                res.SetValue(i, HtmlElement(new HtmlElementPrivate(disp, docPrivate_)));
            }
            return res;
        }

        HtmlElement querySelector(const std::string& query)
        {
            if ( !selector_ ) {
                LOG(WARNING) << "This Internet Explorer version does not support querySelector functions.";
                return  HtmlElement();
            }
            CComPtr<IHTMLElement> elem;
            CComBSTR queryBstr = IuCoreUtils::Utf8ToWstring(query).c_str();
            if ( SUCCEEDED( selector_->querySelector(queryBstr, &elem))  ) {
                if ( elem ) {
                    return new HtmlElementPrivate(elem, docPrivate_);
                }
            }
            return HtmlElement();
        }

        Sqrat::Array querySelectorAll(const std::string& query)
        {
            if ( !selector_ ) {
                LOG(WARNING) << "This Internet Explorer version does not support querySelector functions.";
                return  Sqrat::Array();
            }
            CComPtr<IHTMLDOMChildrenCollection> collection;
            CComBSTR queryBstr = IuCoreUtils::Utf8ToWstring(query).c_str();
            if ( !SUCCEEDED(selector_->querySelectorAll(queryBstr, &collection)) ) {
                return Sqrat::Array();
            }
            long count  = 0;
            if ( !SUCCEEDED(collection->get_length(&count))) {
                return Sqrat::Array();
            }
            Sqrat::Array res(GetCurrentThreadVM(), count);
            for ( int i = 0; i < count; i ++ ) {
                IDispatchPtr  disp = 0;
                collection->item(i,/*, CComVariant(0),*/ &disp);
                // Check if object does  provide IHTMLElement interface
                if ( CComQIPtr<IHTMLElement>(disp)) {
                    res.SetValue(i, HtmlElement(new HtmlElementPrivate(disp, docPrivate_)));
                }
            }
            return res;
        }

        Sqrat::Array getFormElements() {
            if ( !form_ ) {
                LOG(ERROR) << "getFormElements: element is not a form";
                return Sqrat::Array();
            }
            long count = 0;
            if ( !SUCCEEDED( form_->get_length(&count) ) )  {
                return Sqrat::Array();
            }
            Sqrat::Array res(GetCurrentThreadVM(), count);
            for ( int i = 0; i < count; i ++ ) {
                IDispatchPtr  disp = 0;
                form_->item(CComVariant(i), CComVariant(0), &disp);
                //form_->item(i,/*, CComVariant(0),*/ &disp);
                // Check if object does  provide IHTMLElement interface

                res.SetValue(i, HtmlElement(new HtmlElementPrivate(disp, docPrivate_)));

            }
            return res;
        }

        bool submitForm() {
            if ( !form_ ) {
                LOG(ERROR) << "submitForm: element is not a form";
                return false;
            }
            return SUCCEEDED( form_->submit());
        }

    protected:
        CComPtr<IHTMLElement> elem_;
        CComPtr<IHTMLElement2> elem2_;
        CComPtr<IAccessible> accessible_;
        IDispatchPtr disp_;
        CComPtr<IElementSelector> selector_;
        CComQIPtr<IHTMLFormElement> form_;
        HtmlDocumentPrivate *docPrivate_;
};

}
#endif
