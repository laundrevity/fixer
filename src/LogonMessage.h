#pragma once

#include "Message.h"
#include <optional>

class LogonMessage : public Message {
public:
    LogonMessage(
            const std::string& access_key,
            const std::string& access_secret,
            const std::string& target_comp_id,
            const std::string& raw_data);

    LogonMessage(const std::map<uint32_t, std::string>& fields_map) {
        fields_ = fields_map;
    }

    std::string to_string() const override;

private:
    std::map<uint32_t, std::string> create_fields_map();
};
