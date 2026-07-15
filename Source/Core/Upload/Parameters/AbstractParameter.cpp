#include "AbstractParameter.h"

AbstractParameter::AbstractParameter(std::string name)
    : name_(std::move(name))
{
}

std::string AbstractParameter::getName() const
{
    return name_;
}

void AbstractParameter::setTitle(const std::string& title) {
    title_ = title;
}

std::string AbstractParameter::getTitle() const {
    return title_;
}

std::string AbstractParameter::getDescription() const {
    return {};
}
