#include "ChoiceParameter.h"

ChoiceParameter::ChoiceParameter(std::string title)
    : AbstractParameter(std::move(title))
{
}

std::string ChoiceParameter::getType() const {
    return TYPE;
}

std::string ChoiceParameter::getValueAsString() const {
    if (selectedIndex_ == -1) {
        return {};
    }
    return items_.at(selectedIndex_).first;
}

void ChoiceParameter::setValue(const std::string& val) {
    size_t i = 0;

    for (const auto& it : items_) {
        if (it.first == val) {
            selectedIndex_ = static_cast<int>(i);
            return;
        }
        ++i;
    }
    selectedIndex_ = -1;
}

void ChoiceParameter::addItem(const std::string& id, const std::string& value) {
    items_.emplace_back(id, value);
}

void ChoiceParameter::clearItems() {
    items_.clear();
    selectedIndex_ = -1;
}

const std::vector<std::pair<std::string, std::string>>& ChoiceParameter::getItems() const{
    return items_;
}

void ChoiceParameter::setSelectedIndex(int val) {
    if (val >= -1 && val < items_.size()) {
        selectedIndex_ = val;
    }
}

int ChoiceParameter::selectedIndex() const {
    return selectedIndex_;
}

std::string ChoiceParameter::getDescription() const {
    std::string res = "Possible values:\n";
    
    for (const auto& it : items_) {
        res += "    \"" + it.first + "\" (" + it.second + ")\n";
    }

    return res;
}
