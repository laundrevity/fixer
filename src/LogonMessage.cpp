#include "LogonMessage.h"
#include "FixUtil.cpp"
#include <iostream>

// Modify the existing constructor to utilize the new constructor
LogonMessage::LogonMessage(const std::string &access_key, const std::string &access_secret,
             const std::string &target_comp_id, const std::string &raw_data)
        : LogonMessage(create_fields_map()) {}


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

std::map<uint32_t, std::string>
LogonMessage::create_fields_map() {
    std::map<uint32_t, std::string> fields_map;

    // Add header fields
    fields_map[8] = "FIX.4.4";
    fields_map[49] = "client";
    fields_map[56] = std::getenv("FIX_TARGET_COMP_ID");
    // TODO: fix this so it can work for reconnects, in which case seqnum will not be 1 for LOGON msg
    fields_map[34] = "1";
    fields_map[52] = current_utc_time();

    // Add body fields
    fields_map[35] = "A";
    fields_map[98] = "0";
    fields_map[108] = "30";
    fields_map[141] = "Y";

    size_t nonce_length = 32;
    std::string nonce = generate_nonce(nonce_length);
    std::string base64_nonce = base64_encode(nonce);

    std::time_t timestamp = std::time(nullptr);
    std::stringstream raw_data_ss;
    raw_data_ss << timestamp << "." << base64_nonce;
    std::string raw_data = raw_data_ss.str();

    fields_map[96] = raw_data;
    fields_map[553] = std::getenv("FIX_ACCESS_KEY");

    fields_map[554] = generate_password(raw_data, std::getenv("FIX_ACCESS_SECRET"));
    return fields_map;
}
