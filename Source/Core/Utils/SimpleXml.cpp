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

#include "SimpleXml.h"

#include <boost/format.hpp>
#include <tinyxml2.h>

#include "CoreUtils.h"

using namespace tinyxml2;

class SimpleXml_impl
{
    public:
        SimpleXml_impl()
            : docHandle(nullptr)
        {
        }

        virtual ~SimpleXml_impl()= default;

        XMLDocument doc;
        XMLHandle docHandle;
};

class SimpleXmlNode_impl {
    public:
        SimpleXmlNode_impl() {
            m_el = nullptr;
        }

        XMLElement* m_el;

    protected:

    private:
};

SimpleXmlNode::SimpleXmlNode() {
    impl_ = new SimpleXmlNode_impl();
    impl_->m_el = 0;
}

SimpleXmlNode::SimpleXmlNode(const SimpleXmlNode& node) {
    impl_ = new SimpleXmlNode_impl();
    *this = node;
}

SimpleXmlNode::~SimpleXmlNode() {
    delete impl_;
}

SimpleXmlNode::SimpleXmlNode(XMLElement* el) {
    impl_ = new SimpleXmlNode_impl();
    impl_->m_el = el;
}

SimpleXmlNode SimpleXmlNode::operator[](const std::string& name) {
    XMLElement* rootElem = 0;
    if (impl_->m_el)
    {
        XMLNode* rootNode = impl_->m_el->FirstChildElement(name.c_str());
        if (rootNode)
            rootElem = rootNode->ToElement();
    }
    return rootElem;
}

SimpleXmlNode SimpleXmlNode::GetChild(const std::string& name, bool create) {
    XMLElement* rootElem = 0;
    if (impl_->m_el)
    {
        XMLNode* rootNode = impl_->m_el->FirstChildElement(name.c_str());
        if (rootNode)
            rootElem = rootNode->ToElement();
        else if (create)
        {
            return CreateChild(name);
        }
    }
    return rootElem;
}

SimpleXmlNode SimpleXmlNode::GetChildByIndex(int index) {
    XMLElement* res {};
    XMLNode* rootElem = nullptr;
    if (impl_->m_el) {
        int count = 0;
        XMLNode* child = impl_->m_el->FirstChildElement();
        while (child) {
            if (index == count) {
                rootElem = child;
                break;
            }
            child = child->NextSiblingElement();
            count++;
        }
    }
    if (rootElem) {
        res = rootElem->ToElement();
    }
    return res;
}

int SimpleXmlNode::GetChildCount() {
    int count = 0;
    XMLNode* child = impl_->m_el->FirstChildElement();
    while (child) {
        child = child->NextSiblingElement();
        count++;
    }
    return count;
}

bool SimpleXmlNode::IsNull() const {
    return (impl_->m_el == nullptr);
}

bool SimpleXmlNode::GetChilds(const std::string& name, std::vector<SimpleXmlNode>& out) const {
    if (!impl_->m_el)
        return false;
    int count = 0;
    XMLNode* child = impl_->m_el->FirstChildElement(name.c_str());
    while (child) {
        count++;
        XMLElement* el = child->ToElement();
        if (el)
            out.push_back(el);
        child = child->NextSiblingElement(name.c_str());
    }
    return (count != 0);
}

void SimpleXmlNode::DeleteChilds() {
    if (!impl_->m_el)
        return;
    impl_->m_el->DeleteChildren();
}

std::string SimpleXmlNode::Attribute(const std::string& name) const {
    const char* v {};
    std::string result;

    if (impl_->m_el && impl_->m_el->QueryStringAttribute(name.c_str(), &v) == XML_SUCCESS) {
        result = v;
    }

    return result;
}

int SimpleXmlNode::AttributeInt(const std::string& name) const {
    return std::atoi(Attribute(name).c_str());
}

bool SimpleXmlNode::AttributeBool(const std::string& name) const {
    return std::atoi(Attribute(name).c_str()) != 0;
}

int64_t SimpleXmlNode::AttributeInt64(const std::string& name) const {
    return impl_->m_el->Int64Attribute(name.c_str());
}

void SimpleXmlNode::SetAttribute(const std::string& name, const std::string& value) {
    if (!impl_->m_el)
        return;
    impl_->m_el->SetAttribute(name.c_str(), value.c_str());
}

void SimpleXmlNode::SetAttribute(const std::string& name, int value) {
    if (!impl_->m_el)
        return;
    impl_->m_el->SetAttribute(name.c_str(), value);
}

void SimpleXmlNode::SetAttribute(const std::string& name, int64_t value) {
    if (!impl_->m_el)
        return;
    impl_->m_el->SetAttribute(name.c_str(), value);
}

void SimpleXmlNode::SetAttributeInt(const std::string& name, int value) {
    if (!impl_->m_el)
        return;
    impl_->m_el->SetAttribute(name.c_str(), value);
}

void SimpleXmlNode::SetAttributeString(const std::string& name, const std::string& value) {
    SetAttribute(name,value);
}

void SimpleXmlNode::SetAttributeBool(const std::string& name, bool value) {
    if (!impl_->m_el)
        return;
    impl_->m_el->SetAttribute(name.c_str(), value ? 1 : 0);
}

std::string SimpleXmlNode::Name() const {
    std::string result;
    if (impl_->m_el)
        result = impl_->m_el->Value();
    else
        return "null";
    return result;
}

std::string SimpleXmlNode::Text() const {
    std::string result;
    if (impl_->m_el)
    {
        const char* str =  impl_->m_el->GetText();
        if (str)
            result = str;
    }
    return result;
}

SimpleXmlNode& SimpleXmlNode::each(std::function<bool(int, SimpleXmlNode&)> callback) {
    int i = 0;
    XMLNode* child = impl_->m_el->FirstChildElement();
    while (child) {
        XMLElement* el = child->ToElement();
        SimpleXmlNode node(el);
        bool res = callback(i, node);
        if (res) {
            break;
        }
        child = child->NextSiblingElement();
        i++;
    }
    return *this;
}

int SimpleXmlNode::GetLineNumber() const {
    if (!impl_->m_el) { 
        return -1;
    }
    return impl_->m_el->GetLineNum();
}

SimpleXmlNode SimpleXmlNode::CreateChild(const std::string& name) {
    if (!impl_->m_el)
        return 0;
    return impl_->m_el->InsertEndChild(impl_->m_el->GetDocument()->NewElement(name.c_str()))->ToElement();
}

SimpleXml::SimpleXml() {
    impl_ = new SimpleXml_impl();
}

SimpleXmlNode& SimpleXmlNode::operator = (const SimpleXmlNode& node) {
    impl_->m_el = node.impl_->m_el;
    return *this;
}

SimpleXml::~SimpleXml() {
    delete impl_;
}

bool SimpleXml::LoadFromFile(const std::string& fileName) {
    std::string text;
    if (!IuCoreUtils::ReadUtf8TextFile(fileName, text))
        return false;

    return LoadFromString(text);
}

bool SimpleXml::LoadFromString(const std::string& string) {
    bool res = impl_->doc.Parse(string.c_str()) == XML_SUCCESS;
    impl_->docHandle = XMLHandle(&impl_->doc);
    return res;
}

SimpleXmlNode SimpleXml::getRoot(const std::string& name, bool create) {
    if (XMLElement* root = impl_->docHandle.FirstChildElement().ToElement()) {
        return root;
    }
    else if (!create)
        return 0;
    else
    {
        impl_->docHandle = XMLHandle(&impl_->doc);
    }
    /* We do not need to delete this objects later because tinyxml does free memory for us */
    XMLDeclaration* decl = impl_->doc.NewDeclaration();
    impl_->doc.LinkEndChild(decl);
    XMLElement* el = impl_->doc.NewElement(name.c_str());
    impl_->doc.LinkEndChild(el);
    return el;
}

std::string SimpleXml::ToString() {
    XMLPrinter printer;
    //printer.SetIndent( "    " );
    impl_->doc.Accept( &printer );
    return printer.CStr();
}

bool SimpleXml::SaveToFile(const std::string& fileName) const {
    FILE* f = IuCoreUtils::FopenUtf8(fileName.c_str(), "wb");
    if (!f) {
        LOG(ERROR) << boost::format("Could not save xml to file '%s'.") % fileName << std::endl << "Reason: " << strerror(errno);
        return false;
    }
    bool res = impl_->doc.SaveFile(f) == XML_SUCCESS;
    fclose(f);
    return res;
}

void SimpleXmlNode::SetText(const std::string& value) {
    if (!impl_->m_el)
        return;
    impl_->m_el->DeleteChildren();
    impl_->m_el->SetText(value.c_str());
}

bool SimpleXmlNode::GetAttribute(const std::string& name, std::string& value) const {
    const char* v = nullptr;
    if (impl_->m_el)
        if (impl_->m_el->QueryStringAttribute(name.c_str(), &v) != XML_SUCCESS)
            return false;
    if (v) {
        value = v;
        return true;
    }
    return false;
}

bool SimpleXmlNode::GetAttributes(std::vector<std::string>& out) const {
    if (!impl_->m_el)
        return false;
    const XMLAttribute* attr = impl_->m_el->FirstAttribute();
    while (attr != 0)
    {
        out.push_back(attr->Name());
        attr = attr->Next();
    }
    return true;
}

int SimpleXmlNode::GetAttributeCount() {
    if (!impl_->m_el)
        return 0;
    int count = 0;
    const XMLAttribute* attr = impl_->m_el->FirstAttribute();
    while (attr != 0)
    {
        count++;
        attr = attr->Next();
    }
    return count;
}

bool SimpleXmlNode::GetAttributeBool(const std::string& name, bool& value) const {
    if (impl_->m_el)
        if (impl_->m_el->QueryBoolAttribute(name.c_str(), &value) != XML_SUCCESS)
            return false;
    return true;
}

bool SimpleXmlNode::GetAttributeInt(const std::string& name, int& value) const {
    if (impl_->m_el)
        if (impl_->m_el->QueryIntAttribute(name.c_str(), &value) != XML_SUCCESS)
            return false;
    return true;
}
