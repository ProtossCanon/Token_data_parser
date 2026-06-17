#pragma once

#include <algorithm>
#include <cstddef>
#include <map>
#include <string>
#include "LockfreeQueue.h"

namespace utility {

class FixedPointNumber {
public:
    FixedPointNumber() : pointPos(0), number(0) {}

    FixedPointNumber(std::string_view str) {
        auto it = std::find(str.begin(), str.end(), '.');
        
        if (it == str.end()) {
            pointPos = 0;
        } else {
            pointPos = size_t(std::distance(it, str.end()) - 1);
        }
        
        number = 0;
        for (char c : str) {
            if (c == '.') {
                continue;
            }
            number *= 10;
            number += size_t(c - '0');
        }
    }

    explicit operator std::string() {    
        std::string result = std::to_string(number);

        if (pointPos == 0) {
            return result;
        }

        if (result.length() <= pointPos) {
            result = std::string(pointPos - result.length() + 1, '0') + result;
        }

        size_t insertPos = result.length() - pointPos;
        result.insert(insertPos, ".");

        return result;
    }
    
    auto operator<=>(const FixedPointNumber& other) const {
        size_t maxPointPos = std::max(pointPos, other.pointPos);
        
        size_t scaledThis = number * pow10(maxPointPos - pointPos);
        size_t scaledOther = other.number * pow10(maxPointPos - other.pointPos);
        
        return scaledThis <=> scaledOther;
    }
private:
    static size_t pow10(size_t x) {
        size_t result = 1;
        while (x--) {
            result *= 10;
        }
        return result;
    }

    size_t pointPos;
    size_t number;
};

} // namespace utility

using Price = utility::FixedPointNumber;
using Quantity = utility::FixedPointNumber;

using Bids = std::map<Price, Quantity>;
using Asks = std::map<Price, Quantity>;

struct OrderBook {
    Bids bids;
    Asks asks;
};

struct OrderBookUpdate {
    Bids bids;
    Asks asks;

    size_t eventID;
};

static constexpr size_t OrderBookUpdateChanelSize = 1<<13;
using OrderBookUpdateChanel = utility::LockfreeQueue<OrderBookUpdate, OrderBookUpdateChanelSize>;
