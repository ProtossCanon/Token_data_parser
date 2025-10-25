#pragma once

#include <boost/asio/ssl.hpp>
#include <cstdio>

// Load the root certificates into an ssl::context

void load_root_certificates(boost::asio::ssl::context& ctx, boost::system::error_code& ec);

void load_root_certificates(boost::asio::ssl::context& ctx);
