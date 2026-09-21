#pragma once

#include <string>
#include <memory>
#include <vector>

class AbstractParameter {
public:
    AbstractParameter(std::string name);
    virtual ~AbstractParameter() = default;

    std::string getName() const;
    void setTitle(const std::string& title);
    std::string getTitle() const;
    virtual std::string getType() const = 0; 

    virtual void setValue(const std::string& val) = 0;
    virtual std::string getValueAsString() const  = 0;

    virtual std::string getDescription() const;

private:
    std::string name_, title_;
};

using ParameterList = std::vector<std::unique_ptr<AbstractParameter>>;
