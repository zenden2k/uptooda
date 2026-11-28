#include "AbstractParameter.h"

AbstractParameter::AbstractParameter(std::string name) : name_(std::move(name)) {
}

std::string AbstractParameter::getName() const {
    return name_;
}

void AbstractParameter::setTitle(const std::string& title) {
    title_ = title;
}

std::string AbstractParameter::getTitle() const {
    return title_;
}

std::string AbstractParameter::getDisplayTitle() const {
    return isRequired() ? title_ + " *" : title_;
}

void AbstractParameter::setRequired(bool required) {
    required_ = required;
}

bool AbstractParameter::isRequired() const {
    return required_;
}

std::string AbstractParameter::getDescription() const {
    return { };
}
