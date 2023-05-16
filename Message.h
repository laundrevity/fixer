#pragma once

#include <string>
#include <map>

class Message {
public:
    virtual std::string to_string() const = 0;

protected:
    std::map<uint32_t, std::string> fields_;
};