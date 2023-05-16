#include "NewOrderSingle.h"
#include "FixUtil.cpp"

NewOrderSingle::NewOrderSingle(const std::string& clOrdID, char side, double orderQty, double price, const std::string& symbol,
                               const std::string& execInst, char ordType, char timeInForce, double stopPx,
                               double maxShow, int qtyType, double pegOffsetValue, int pegPriceType,
                               const std::string& deribitLabel, char deribitAdvOrderType, bool deribitMMProtection,
                               int deribitConditionTriggerMethod) {
    // Add header fields
    fields_[8] = "FIX.4.4";
    fields_[49] = "client";
    fields_[56] = "DERIEXEC"; // Assuming Deribit as exchange
    fields_[34] = "2";
    fields_[52] = current_utc_time();

    // Add body fields
    fields_[35] = "D";
    fields_[11] = clOrdID;
    fields_[54] = std::to_string(side);
    fields_[38] = std::to_string(orderQty);
    fields_[44] = std::to_string(price);
    fields_[55] = symbol;

    if (!execInst.empty()) fields_[18] = execInst;
    fields_[40] = std::to_string(ordType);
    fields_[59] = std::to_string(timeInForce);

    if (stopPx > 0) fields_[99] = std::to_string(stopPx);
    if (maxShow > 0) fields_[210] = std::to_string(maxShow);
    fields_[854] = std::to_string(qtyType);

    if (pegOffsetValue != 0.0) fields_[211] = std::to_string(pegOffsetValue);
    if (pegPriceType != 0) fields_[1094] = std::to_string(pegPriceType);
    if (!deribitLabel.empty()) fields_[100010] = deribitLabel;
    if (deribitAdvOrderType != '\0') fields_[100012] = std::to_string(deribitAdvOrderType);
    fields_[9008] = deribitMMProtection ? "Y" : "N";
    fields_[5127] = std::to_string(deribitConditionTriggerMethod);
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