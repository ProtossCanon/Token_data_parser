#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>

#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include "OrderBook.h"
#include <rapidjson/document.h>
#include <rapidjson/prettywriter.h>

#include "MarketDataConnector.h"
#include <root_cert.h>

namespace {

rapidjson::Document getOrderBookFromAPI(const std::string_view target) {
    namespace net = boost::beast::net;
    namespace http = boost::beast::http;

    const auto host = "testnet.binance.vision";
    constexpr std::string_view port = "443";

    net::io_context ioc;
    net::ssl::context ctx(net::ssl::context::tlsv12_client);
    load_root_certificates(ctx);
    ctx.set_verify_mode(net::ssl::verify_peer);

    net::ip::tcp::resolver resolver(ioc);
    net::ssl::stream<boost::beast::tcp_stream> stream(ioc, ctx);

    {
        if (!SSL_set_tlsext_host_name(stream.native_handle(), host)) {
            throw boost::beast::system_error(
                static_cast<int>(::ERR_get_error()),
                net::error::get_ssl_category());
        }
    }

    stream.set_verify_callback(net::ssl::host_name_verification(host));
    auto const results = resolver.resolve(host, port);

    boost::beast::get_lowest_layer(stream).connect(results);

    stream.handshake(net::ssl::stream_base::client);

    http::request<http::string_body> req{http::verb::get, target, 11};

    req.set(http::field::host, host);
    req.target(target);
    req.set(http::field::user_agent, BOOST_BEAST_VERSION_STRING);

    http::write(stream, req);
    boost::beast::flat_buffer buffer;
    http::response<http::dynamic_body> res;
    http::read(stream, buffer, res);

    std::string const responseBody = boost::beast::buffers_to_string(res.body().data());
    rapidjson::Document document;
    if (document.Parse(responseBody.c_str()).HasParseError()) {
        std::cerr << "Order book parsing failed\n";
        exit(1);
    }
    return document;
}

std::pair<Asks, Bids> parseAsksAndBids(const rapidjson::Document &document) {
    auto asksArray = document["asks"].GetArray();
    Asks asks;
    for (auto it = asksArray.begin(); it != asksArray.end(); ++it) {
        std::string price = it->GetArray()[0].GetString();
        std::string amount = it->GetArray()[1].GetString();
        asks[Price(price)] = Quantity(amount);
    }
    auto bidsArray = document["bids"].GetArray();
    Bids bids;
    for (auto it = bidsArray.begin(); it != bidsArray.end(); ++it) {
        std::string price = it->GetArray()[0].GetString();
        std::string amount = it->GetArray()[1].GetString();
        bids[Price(price)] = Quantity(amount);
    }
    return {asks, bids};
}

} // namespace

std::pair<OrderBook, MarketDataConnector::eventID> MarketDataConnector::getOrderBook(std::string_view token) {
    auto orderBookJson = getOrderBookFromAPI(token);
    OrderBook orderBook;
    {
        auto [asks, bids] = parseAsksAndBids(orderBookJson);
        orderBook.asks = asks;
        orderBook.bids = bids;
    }

    MarketDataConnector::eventID ID = orderBookJson["lastUpdateId"].GetUint64();

    return {orderBook, ID};
}

MarketDataConnector::threadID MarketDataConnector::subscribeForUpdate(
    std::string_view token, std::shared_ptr<OrderBookUpdateChanel> lockfreeQueue,
    std::function<void(const std::exception&)> resqueCallback) {


    std::lock_guard<std::mutex> lock(mapMutex);
    MarketDataConnector::threadID ID = generateThreadID();

    auto updateThread = std::make_shared<std::thread>(
    [lockfreeQueue, &flag = updateThreads[ID].second, token = std::string(token), resqueCallback]() mutable {
    try
    {
        std::string host = "stream.binance.com";
        constexpr std::string_view port = "443";

        namespace websocket = boost::beast::websocket;
        namespace net = boost::beast::net;
        namespace http = boost::beast::http;

        net::io_context ioc;
        net::ssl::context ctx(net::ssl::context::tlsv12_client);
        load_root_certificates(ctx);
        ctx.set_verify_mode(net::ssl::verify_peer);

        net::ip::tcp::resolver resolver(ioc);
        websocket::stream<net::ssl::stream<net::ip::tcp::socket>> ws{ioc, ctx};

        auto const results = resolver.resolve(host, port);

        auto ep = net::connect(boost::beast::get_lowest_layer(ws), results);

        if(! SSL_set_tlsext_host_name(ws.next_layer().native_handle(), host.c_str()))
        {
            throw boost::beast::system_error(
                static_cast<int>(::ERR_get_error()),
                net::error::get_ssl_category());
        }
        ws.next_layer().set_verify_callback(net::ssl::host_name_verification(host));
        host += ':' + std::to_string(int(ep.port()));
        ws.next_layer().handshake(net::ssl::stream_base::client);
        ws.set_option(websocket::stream_base::decorator(
            [](websocket::request_type& req)
            {
                req.set(http::field::user_agent,
                    std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-client-coro");
            }));
        ws.handshake(host, std::string("/ws/") + token + std::string("@depth20"));

        boost::beast::flat_buffer buffer;

        while (!flag) {
            ws.read(buffer);
            std::string const responseBody = boost::beast::buffers_to_string(buffer.data());
            rapidjson::Document document;
            if (document.Parse(responseBody.c_str()).HasParseError()) {
                std::cerr << "Order book update parsing failed\n";
                exit(1);
            }
            OrderBookUpdate update;
            auto [asks, bids] = parseAsksAndBids(document);
            update.asks = asks;
            update.bids = bids;
            update.eventID = document["lastUpdateId"].GetUint64();
            lockfreeQueue->push(update);
            buffer.clear();
        }
        ws.close(websocket::close_code::normal);
        std::cout << boost::beast::make_printable(buffer.data()) << std::endl;
    }
    catch(std::exception const& e)
    {
        resqueCallback(e);
        std::cerr << "Error: " << e.what() << std::endl;
    }
});
    updateThreads[ID].first = std::move(updateThread);
    return ID;
}

// mapMutex should be locked before call this function
MarketDataConnector::threadID MarketDataConnector::generateThreadID() {
    static std::random_device randomDevice;
    static std::mt19937_64 generator(randomDevice());
    
    std::uniform_int_distribution<size_t> distribution(0, SIZE_MAX);
    size_t ID = distribution(generator);
    while (updateThreads.find(ID) != updateThreads.end());
    return ID;
}
