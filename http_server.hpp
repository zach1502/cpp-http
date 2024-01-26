// http_server.hpp
#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <boost/asio.hpp>
#include <memory>

namespace net = boost::asio;  // from <boost/asio.hpp>
using tcp = net::ip::tcp;     // from <boost/asio/ip/tcp.hpp>

class http_server {
public:
    http_server(net::io_context& ioc, tcp::endpoint endpoint);
    void run();
    void stop();

private:
    net::io_context& ioc_;
    tcp::acceptor acceptor_;

    void do_accept();
};

#endif  // HTTP_SERVER_HPP
