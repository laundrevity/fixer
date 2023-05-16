#pragma once

#include "Message.h"

class LogonMessage : public Message {
public:
    LogonMessage(
            const std::string& access_key,
            const std::string& access_secret,
            const std::string& target_comp_id,
            const std::string& raw_data);

    std::string to_string() const override;
};
