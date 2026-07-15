#pragma once

#include <string>
#include <map>
#include <memory>

#include "3rdpart/PropertyItem.h"
#include "Core/Upload/Parameters/AbstractParameter.h"

class CPropertyListCtrl;
class ServerSettingsStruct;

class IParameterController {
public:
    virtual ~IParameterController() = default;
    virtual HPROPERTY show(AbstractParameter* parameter, CString title, const std::string& value, LPARAM lParam) = 0;
    virtual bool save(AbstractParameter* parameter, const CComVariant& value) = 0;
};

class ParameterListAdapter {
public:
    ParameterListAdapter(ParameterList* parameterList, CPropertyListCtrl* control);
    void updateControl(ServerSettingsStruct* serverSettings);
    void saveValues(ServerSettingsStruct* serverSettings);
    void browseFileDialog(HPROPERTY prop);

    void registerParameterController(const std::string& type, std::unique_ptr<IParameterController> controller);
private:
    ParameterList* parameterList_;
    CPropertyListCtrl* control_;
    [[nodiscard]] IParameterController* getParameterController(const std::string& typeStr) const;
    std::map<std::string, std::unique_ptr<IParameterController>> registeredTypes_;
};
