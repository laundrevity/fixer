#include "NewOrderSingle.h"
#include "FixUtil.cpp"

NewOrderSingle::NewOrderSingle(const std::string& clOrdID, char side, double orderQty, double price, const std::string& symbol) {
    // Add header fields
    fields_[8] = "FIX.4.4";
    fields_[49] = "client";
    fields_[56] = std::getenv("FIX_TARGET_COMP_ID");
    fields_[34] = "2";
    fields_[52] = current_utc_time();

    // Add body fields
    fields_[35] = "D";
    fields_[11] = clOrdID;
    fields_[54] = std::to_string(side);
    fields_[38] = std::to_string(orderQty);
    fields_[44] = std::to_string(price);
    fields_[55] = symbol;
}

std::string NewOrderSingle::to_string() const {
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
    std::string new_order_single_str = header.str() + body_string;
    int checksum = calculate_checksum(new_order_single_str);
    std::stringstream checksum_field;
    checksum_field << "10=" << std::setw(3) << std::setfill('0') << checksum << '\x01';
    new_order_single_str += checksum_field.str();

    return new_order_single_str;
}