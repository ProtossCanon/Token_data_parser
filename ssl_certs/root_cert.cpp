#include <boost/asio/ssl.hpp>
#include <cstdio>
#include <fstream>
#include <string>

namespace detail {

const std::string pathToSSLCert = "/etc/ssl/certs/ca-certificates.crt";

inline void load_root_certificates(boost::asio::ssl::context& ctx, boost::system::error_code& ec)
{
    std::string cert;

    std::ifstream fin("/etc/ssl/certs/ca-certificates.crt");
    while (fin) {
        std::string str;
        getline(fin, str);
        cert += str + '\n';
    }

    ctx.add_certificate_authority(
        boost::asio::buffer(cert.data(), cert.size()), ec);
    if(ec)
        return;
}

} // namespace

void load_root_certificates(boost::asio::ssl::context& ctx, boost::system::error_code& ec)
{
    detail::load_root_certificates(ctx, ec);
}

void load_root_certificates(boost::asio::ssl::context& ctx)
{
    boost::system::error_code ec;
    detail::load_root_certificates(ctx, ec);
    if(ec)
        throw boost::system::system_error{ec};
}
