#pragma once

#include "Message.h"
#include <string>
#include <map>


class NewOrderSingle : public Message {
public:
    NewOrderSingle(const std::string& clOrdID, char side, double orderQty, double price, const std::string& symbol);

    std::string to_string() const override;
};