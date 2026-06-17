#pragma once

#include <string_view>
#include <root_cert.h>
#include "MarketDataConnector.h"
#include "OrderBook.h"

class OrderBookHolder {
public:
    OrderBookHolder(std::string_view token);
private:
    OrderBook orderBook;
};
