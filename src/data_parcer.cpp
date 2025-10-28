#include <root_cert.h>

#include <boost/asio/connect.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/core/buffers_to_string.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <iostream>

int main() {
    namespace net = boost::beast::net;
    namespace http = boost::beast::http;

    const auto host = "testnet.binance.vision";
    const std::string port = "443";
    const std::string target = "/api/v3/depth?symbol=BNBBTC&limit=5000";

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

    std::cout << res << std::endl;
}