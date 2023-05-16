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

    std::string to_string() const override;
};