#include "ParameterListAdapter.h"

#include "Core/CommonDefs.h"
#include "Func/WinUtils.h"
#include "3rdpart/PropertyList.h"
#include "Core/Upload/Parameters/TextParameter.h"
#include "Core/Upload/Parameters/ChoiceParameter.h"
#include "Core/Upload/Parameters/BooleanParameter.h"
#include "Core/Upload/Parameters/FileNameParameter.h"
#include "Core/Upload/UploadEngine.h"
#include "Gui/Components/MyFileDialog.h"
#include "Core/i18n/Translator.h"
#include "Gui/Components/NewStyleFolderDialog.h"

class TextParameterController : public IParameterController {
public:
    HPROPERTY show(AbstractParameter* parameter, CString title, const std::string& value, LPARAM lParam) override {
        auto par = dynamic_cast<TextParameter*>(parameter);
        if (!par) {
            return nullptr;
        }
        return PropCreateSimple(title, U2W(parameter->getValueAsString()), lParam);
    }

    bool save(AbstractParameter* parameter, const CComVariant& value) override {
        parameter->setValue(WCstringToUtf8(value.bstrVal));
        return true;
    }
};

class BooleanParameterController : public IParameterController {
public:
    HPROPERTY show(AbstractParameter* parameter, CString title, const std::string& value, LPARAM lParam) override {
        auto par = dynamic_cast<BooleanParameter*>(parameter);
        if (!par) {
            return nullptr;
        }

        return PropCreateCheckButton(title, par->getValue(), lParam);
    }

    bool save(AbstractParameter* parameter, const CComVariant& value) override {
        auto par = dynamic_cast<BooleanParameter*>(parameter);
        if (!par) {
            return false;
        }
        par->setValue(value.boolVal);
        return true;
    }
};

class ChoiceParameterController : public IParameterController {
public:
    HPROPERTY show(AbstractParameter* parameter, CString title, const std::string& value, LPARAM lParam) override {
        auto par = dynamic_cast<ChoiceParameter*>(parameter);
        if (!par) {
            return nullptr;
        }

        const auto& items = par->getItems();
        std::vector<CString> s; // Maybe only this vector of CString is enough?
        std::vector<LPCTSTR> choices;
        for (const auto& it : items) {
            s.emplace_back(IuCoreUtils::Utf8ToWstring(it.second).c_str());
            choices.push_back(s.back().GetString());
        }
        choices.push_back(nullptr);
        return PropCreateList(U2W(par->getTitle()), choices.data(), par->selectedIndex(), lParam);
    }

    bool save(AbstractParameter* parameter, const CComVariant& value) override {
        auto choiceParameter = dynamic_cast<ChoiceParameter*>(parameter);
        if (choiceParameter) {
            choiceParameter->setSelectedIndex(value.intVal);
            return true;
        }
        return false;
    }
};

class FileNameParameterController : public IParameterController {
public:
    HPROPERTY show(AbstractParameter* parameter, CString title, const std::string& value, LPARAM lParam) override {
        auto par = dynamic_cast<FileNameParameter*>(parameter);
        if (!par) {
            return nullptr;
        }
        return PropCreateFileName(title, U2W(parameter->getValueAsString()), lParam);
    }

    bool save(AbstractParameter* parameter, const CComVariant& value) override {
        parameter->setValue(WCstringToUtf8(value.bstrVal));
        return true;
    }
};

ParameterListAdapter::ParameterListAdapter(ParameterList* parameterList, CPropertyListCtrl* control):
    parameterList_(parameterList),
    control_(control)
{
    registerParameterController(TextParameter::TYPE, std::make_unique<TextParameterController>());
    registerParameterController(BooleanParameter::TYPE, std::make_unique<BooleanParameterController>());
    registerParameterController(ChoiceParameter::TYPE, std::make_unique<ChoiceParameterController>());
    registerParameterController(FileNameParameter::TYPE, std::make_unique<FileNameParameterController>());
}

void ParameterListAdapter::updateControl(ServerSettingsStruct* serverSettings) {
    std::sort(parameterList_->begin(), parameterList_->end(), [](auto& l, auto& r) {
        int res = lstrcmpi(U2WC(l->getName()), U2WC(r->getName()));
        return res < 0;
    });
    for (const auto& parameter : *parameterList_) {
        HPROPERTY prop = nullptr;
        CString name = U2W(parameter->getName());
        std::string value = serverSettings ? serverSettings->params[parameter->getName()] : std::string();
        auto lParam = reinterpret_cast<LPARAM>(parameter.get());

        auto controller = getParameterController(parameter->getType());

        if (controller) {
            parameter->setValue(value);
            prop = controller->show(parameter.get(), U2W(parameter->getTitle()), value, lParam);
        }

        if (prop) {
            control_->AddItem(prop);
        }
    }
}

void ParameterListAdapter::saveValues(ServerSettingsStruct* serverSettings) {
    for (const auto& parameter : *parameterList_) {
        HPROPERTY pr = control_->FindProperty(reinterpret_cast<LPARAM>(parameter.get()));

        if (pr) {
            CComVariant vValue;
            pr->GetValue(&vValue);

            auto controller = getParameterController(parameter->getType());
            std::string result;
             
            if (controller) {
                if (controller->save(parameter.get(), vValue)) {
                    result = parameter->getValueAsString();
                }
            }

            serverSettings->params[parameter->getName()] = std::move(result);
        }
    }
}

void ParameterListAdapter::browseFileDialog(HPROPERTY prop) {
    auto par = reinterpret_cast<AbstractParameter*>(prop->GetItemData());

    if (!par) {
        return;
    }

    auto* fileNameParameter = dynamic_cast<FileNameParameter*>(par);
    if (!fileNameParameter) {
        return;
    }
    VARIANT vart;
    vart.vt = VT_BSTR;
    if (!fileNameParameter->directory()) {
        // TODO: customizable file name filters
        IMyFileDialog::FileFilterArray filters = {
            { TR("All files"), _T("*.*") }
        };
        CString fileDir = U2W(IuCoreUtils::ExtractFilePath(fileNameParameter->getValueAsString()));

        auto dlg = MyFileDialogFactory::createFileDialog(control_->m_hWnd, fileDir, {}, filters, false);
        if (dlg->DoModal(control_->m_hWnd) != IDOK) {
            return;
        }
        vart.bstrVal = dlg->getFile().AllocSysString();
    } else {
        CString initialDir = U2W(fileNameParameter->getValueAsString());
        CNewStyleFolderDialog dlg(control_->m_hWnd, initialDir, TR("Choose folder"), true);
        if (dlg.DoModal(control_->m_hWnd) != IDOK) {
            return;
        }

        vart.bstrVal = dlg.GetFolderPath().AllocSysString();
    }

    control_->SetItemValue(prop, &vart);
    SysFreeString(vart.bstrVal);
}

void ParameterListAdapter::registerParameterController(const std::string& type, std::unique_ptr<IParameterController> controller) {
    registeredTypes_[type] = std::move(controller);
}

IParameterController* ParameterListAdapter::getParameterController(const std::string& typeStr) const {
    auto it = registeredTypes_.find(typeStr);

    if (it != registeredTypes_.end()) {
        return it->second.get();
    }

    // Fallback to text parameter type
    it = registeredTypes_.find(TextParameter::TYPE);
    if (it != registeredTypes_.end()) {
        return it->second.get();
    }
    return {};
}

