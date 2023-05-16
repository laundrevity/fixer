#include "LogonMessage.h"
#include "FixUtil.cpp"

LogonMessage::LogonMessage(const std::string &access_key, const std::string &access_secret,
                           const std::string &target_comp_id, const std::string &raw_data) {
    // Add header fields
    fields_[8] = "FIX.4.4";
    fields_[49] = "client";
    fields_[56] = target_comp_id;
    fields_[34] = "1";
    fields_[52] = current_utc_time();

    // Add body fields
    fields_[35] = "A";
    fields_[98] = "0";
    fields_[108] = "30";
    fields_[141] = "Y";
    fields_[96] = raw_data;
    fields_[553] = access_key;

    // Calculate and add the password
    std::string password = generate_password(raw_data, access_secret);
    fields_[554] = password;
}

std::string LogonMessage::to_string() const {
    std::stringstream header, body;
    header << "8=" << fields_.at(8) << '\x01' << "9=";  // BeginString and placeholder for BodyLength

    for (const auto &kv : fields_) {
        if (kv.first == 8 || kv.first == 10) { // Skip BeginString and Checksum
            continue;
        }
        body << kv.first << "=" << kv.second << '\x01';
    }

    // Calculate and insert BodyLength into the header
    std::string body_string = body.str();
    int body_length = body_string.size();
    header << std::setw(3) << std::setfill('0') << std::to_string(body_length) << '\x01';

    // Calculate and add the checksum at the end
    std::string logon_message_str = header.str() + body_string;
    int checksum = calculate_checksum(logon_message_str);
    std::stringstream checksum_field;
    checksum_field << "10=" << std::setw(3) << std::setfill('0') << checksum << '\x01';
    logon_message_str += checksum_field.str();

    return logon_message_str;
}
