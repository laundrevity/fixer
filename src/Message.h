#pragma once

#include <string>
#include <map>

class Message {
public:
    virtual std::string to_string() const = 0;

    void set_sequence_number(int seq_num) {
        fields_[34] = std::to_string(seq_num);
    }

protected:
    std::map<uint32_t, std::string> fields_;
};