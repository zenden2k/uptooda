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

#include "HtmlElement.h"

#include "Core/Scripting/Squirrelnc.h"

#ifdef _WIN32
#include "HtmlElementPrivate_win.h"
#else
// Not implemented
#endif

namespace ScriptAPI {

HtmlElement::HtmlElement() {
    //d_.reset(new HtmlElementPrivate());
}

HtmlElement::HtmlElement(HtmlElementPrivate* pr) {
    d_.reset(pr);
}

std::string HtmlElement::getAttribute(const std::string& name) {
    if (!checkNull("getAttribute")) {
        return {};
    }
    return d_->getAttribute(name);
}

void HtmlElement::setAttribute(const std::string& name, const std::string& value) {
    if (!checkNull("setAttribute")) {
        return;
    }
    d_->setAttribute(name, value);
}

void HtmlElement::removeAttribute(const std::string& name) {
    if (!checkNull("removeAttribute")) {
        return;
    }
    d_->removeAttribute(name);
}

std::string HtmlElement::getId() {
    if (!checkNull("getId")) {
        return {};
    }
    return d_->getId();
}

void HtmlElement::setId(const std::string& id) {
    if (!checkNull("setId")) {
        return;
    }
    d_->setId(id);
}

std::string HtmlElement::getInnerHTML() {
    if (!checkNull("getInnerHTML")) {
        return {};
    }
    return d_->getInnerHTML();
}

void HtmlElement::setInnerHTML(const std::string& html) {
    if (!checkNull("setInnerHTML")) {
        return;
    }
    d_->setInnerHTML(html);
}

std::string HtmlElement::getInnerText() {
    if (!checkNull("getInnerText")) {
        return {};
    }
    return d_->getInnerText();
}

void HtmlElement::setInnerText(const std::string& text) {
    if (!checkNull("setInnerText")) {
        return;
    }
    d_->setInnerText(text);
}

std::string HtmlElement::getOuterHTML() {
    if (!checkNull("getOuterHTML")) {
        return {};
    }
    return d_->getOuterHTML();
}

void HtmlElement::setOuterHTML(const std::string& html) {
    if (!checkNull("setOuterHTML")) {
        return;
    }
    d_->setOuterHTML(html);
}

std::string HtmlElement::getOuterText() {
    if (!checkNull("getOuterText")) {
        return {};
    }
    return d_->getOuterText();
}

void HtmlElement::setOuterText(const std::string& text) {
    if (!checkNull("setOuterText")) {
        return;
    }
    d_->setOuterText(text);
}

void HtmlElement::setValue(const std::string& value) {
    if (!checkNull("setValue")) {
        return;
    }
    d_->setValue(value);
}

std::string HtmlElement::getValue() {
    if (!checkNull("getValue")) {
        return {};
    }
    return d_->getValue();
}

std::string HtmlElement::getTagName() {
    if (!checkNull("getTagName")) {
        return {};
    }
    return d_->getTagName();
}


HtmlElement HtmlElement::getParentElement() {
    if (!checkNull("parentElement")) {
        return {};
    }
    return d_->getParentElement();
}

void HtmlElement::scrollIntoView() {
    if (!checkNull("scrollIntoView")) {
        return;
    }
    d_->scrollIntoView();
}

void HtmlElement::click() {
    if (!checkNull("click")) {
        return;
    }
    d_->click();
}

void HtmlElement::insertHTML(const std::string& name, bool atEnd /*= false */) {
    if (!checkNull("insertHTML")) {
        return;
    }
    d_->insertHTML(name, atEnd /*= false */);
}

void HtmlElement::insertText(const std::string& name, bool atEnd /*= false */) {
    if (!checkNull("insertText")) {
        return;
    }
    d_->insertText(name, atEnd /*= false */);
}

HtmlElement HtmlElement::querySelector(const std::string& query) {
    if (!checkNull("querySelector")) {
        return {};
    }
    return d_->querySelector(query);
}

Sqrat::Array HtmlElement::querySelectorAll(const std::string& query) {
    if (!checkNull("querySelectorAll")) {
        return {};
    }
    return d_->querySelectorAll(query);
}

Sqrat::Array HtmlElement::getFormElements() {
    if (!checkNull("getFormElements")) {
        return {};
    }
    return d_->getFormElements();
}

bool HtmlElement::submitForm() {
    if (!checkNull("submitForm")) {
        return false;
    }
    return d_->submitForm();
}

bool HtmlElement::isNull() {
    return !d_ || d_->isNull();
}

Sqrat::Array HtmlElement::getChildren() {
    if (!checkNull("getChildren")) {
        return {};
    }
    return d_->getChildren();
}

bool HtmlElement::checkNull(const char* func) {
    if (isNull()) {
        LOG(WARNING) << func << " : " << "HtmlElement is null";
        return false;
    }
    return true;
}

}
