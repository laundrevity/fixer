#pragma once

#include "Message.h"
#include <string>
#include <map>


class NewOrderSingle : public Message {
public:
    NewOrderSingle(const std::string& clOrdID, char side, double orderQty, double price, const std::string& symbol,
                   const std::string& execInst = "", char ordType = '2', char timeInForce = '1', double stopPx = 0.0,
                   double maxShow = 0.0, int qtyType = 0, double pegOffsetValue = 0.0, int pegPriceType = 0,
                   const std::string& deribitLabel = "", char deribitAdvOrderType = '\0', bool deribitMMProtection = false,
                   int deribitConditionTriggerMethod = 1);

    explicit NewOrderSingle(const std::map<uint32_t, std::string>& fields_map) {
        fields_ = fields_map;
    }

    [[nodiscard]] std::string to_string() const override;
private:
    std::map<uint32_t, std::string> create_fields_map(const std::string& clOrdID, char side, double orderQty, double price, const std::string& symbol,
                                                      const std::string& execInst = "", char ordType = '2', char timeInForce = '1', double stopPx = 0.0,
                                                      double maxShow = 0.0, int qtyType = 0, double pegOffsetValue = 0.0, int pegPriceType = 0,
                                                      const std::string& deribitLabel = "", char deribitAdvOrderType = '\0', bool deribitMMProtection = false,
                                                      int deribitConditionTriggerMethod = 1);
};