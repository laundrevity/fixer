#include "NewOrderSingle.h"
#include "FixUtil.cpp"

NewOrderSingle::NewOrderSingle(const std::string& clOrdID, char side, double orderQty, double price, const std::string& symbol,
                               const std::string& execInst, char ordType, char timeInForce, double stopPx,
                               double maxShow, int qtyType, double pegOffsetValue, int pegPriceType,
                               const std::string& deribitLabel, char deribitAdvOrderType, bool deribitMMProtection,
                               int deribitConditionTriggerMethod)
                               : NewOrderSingle(create_fields_map(
                                       clOrdID, side, orderQty, price, symbol, execInst, ordType, timeInForce, stopPx,
                                       maxShow, qtyType, pegOffsetValue, pegPriceType, deribitLabel, deribitAdvOrderType, deribitMMProtection, deribitConditionTriggerMethod)) {}

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

std::map<uint32_t, std::string>
NewOrderSingle::create_fields_map(const std::string &clOrdID, char side, double orderQty, double price,
                                  const std::string &symbol, const std::string &execInst, char ordType,
                                  char timeInForce, double stopPx, double maxShow, int qtyType, double pegOffsetValue,
                                  int pegPriceType, const std::string &deribitLabel, char deribitAdvOrderType,
                                  bool deribitMMProtection, int deribitConditionTriggerMethod) {
    std::map<uint32_t, std::string> fields_map;

    // Add header fields
    fields_map[8] = "FIX.4.4";
    fields_map[49] = "client";
    fields_map[56] = std::getenv("DERIBITSERVER");
    fields_map[34] = "0";                                   // this will get updated by buster before being sent
    fields_map[52] = current_utc_time();

    // Add body fields
    fields_map[35] = "D";
    fields_map[11] = clOrdID;
    fields_map[54] = std::to_string(side);
    fields_map[38] = std::to_string(orderQty);
    fields_map[44] = std::to_string(price);
    fields_map[55] = symbol;

    if (!execInst.empty()) fields_map[18] = execInst;
    fields_map[40] = std::to_string(ordType);
    fields_map[59] = std::to_string(timeInForce);

    if (stopPx > 0) fields_map[99] = std::to_string(stopPx);
    if (maxShow > 0) fields_map[210] = std::to_string(maxShow);
    fields_map[854] = std::to_string(qtyType);

    if (pegOffsetValue != 0.0) fields_map[211] = std::to_string(pegOffsetValue);
    if (pegPriceType != 0) fields_map[1094] = std::to_string(pegPriceType);
    if (!deribitLabel.empty()) fields_map[100010] = deribitLabel;
    if (deribitAdvOrderType != '\0') fields_map[100012] = std::to_string(deribitAdvOrderType);
    fields_map[9008] = deribitMMProtection ? "Y" : "N";
    fields_map[5127] = std::to_string(deribitConditionTriggerMethod);

    return fields_map;
}
