#pragma once

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include "OrderBook.h"

class MarketDataConnector {
public:
    using eventID = size_t;
    std::pair<OrderBook, eventID> getOrderBook(std::string_view token);

    using threadID = size_t;
    threadID subscribeForUpdate(std::string_view token, std::shared_ptr<OrderBookUpdateChanel> lockfreeQueue, std::function<void(const std::exception&)> resqueCallback);

    void unsubscribeForUpdate(threadID threadToKill);

private:
    threadID generateThreadID();

    std::mutex mapMutex;
    std::map<threadID, std::pair<std::shared_ptr<std::thread>, std::atomic<bool>>> updateThreads;
};
