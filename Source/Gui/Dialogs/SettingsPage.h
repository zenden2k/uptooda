#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#pragma once

#include <stdexcept>

#include "atlheaders.h"
#include "Gui/GuiTools.h"
#include "Core/i18n/Translator.h"
#include "Func/Library.h"

class CWizardDlg;
class CSettingsDlg;

struct ValidationError {
    CString Message;
    HWND Control;

    ValidationError()
    {
        Control = nullptr;
    }
    ValidationError(CString message, HWND control)
        : Message(message)
        , Control(control)
    {
    }
};

class ValidationException : public std::exception {
public:
    struct ValidationError {
        CString Message;
        HWND Control;

        ValidationError()
        {
            Control = nullptr;
        }

        ValidationError(CString message, HWND control)
            : Message(message)
            , Control(control)
        {
        }
    };

    ValidationException(CString Message, HWND Control = nullptr) : std::exception("Form validation error") {
        try {
            errors_.emplace_back(Message, Control);
        } catch (const std::exception&) {
            
        }
    }

    ValidationException(std::vector<ValidationError> errors) : std::exception("Form validation error") {
        errors_ = std::move(errors);
    }

    ValidationException(const ValidationException& ex) : std::exception(ex), errors_(ex.errors_) {}

    ValidationException& operator=(const ValidationException& ex) {
        this->errors_ = ex.errors_;
        return *this;
    }

    std::vector<ValidationError> errors_;
};

class CSettingsPage
{
    public:
        CSettingsPage();
        virtual ~CSettingsPage() = default;
        CWizardDlg *WizardDlg = nullptr;
        HBITMAP HeadBitmap = nullptr;
        virtual bool onShow();
        virtual bool onHide();
        HWND PageWnd = nullptr;
        void fixBackground() const;

        void addError(CString message, HWND control = NULL);
        virtual bool apply();
        virtual bool validate();
        void clearErrors();
        const std::vector<ValidationError>& errors() const;
    protected:
         std::vector<ValidationError> errors_;
};

template<typename T>
class CSettingsPageTrait {
    public:

    void checkBounds(int controlId, int minValue, int maxValue, int labelId) const {
        auto* derived =  static_cast<const T*>(this);
        int value = derived->GetDlgItemInt(controlId);
        if (value < minValue || value > maxValue) {
            CString fieldName = labelId != -1 ? GuiTools::GetDlgItemText(derived->m_hWnd, labelId) : _T("Unknown field");
            CString message;
            message.Format(TR("Error in the field '%s': value should be between %d and %d."), fieldName.GetString(), minValue, maxValue);
            throw ValidationException(message, derived->GetDlgItem(controlId));
        }
    }
};
#endif // SETTINGSPAGE_H
