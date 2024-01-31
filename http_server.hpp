// http_server.hpp
#ifndef HTTP_SERVER_HPP
#define HTTP_SERVER_HPP

#include <boost/asio.hpp>
#include <memory>

namespace net = boost::asio;  // from <boost/asio.hpp>
using tcp = net::ip::tcp;     // from <boost/asio/ip/tcp.hpp>

class http_server {
 public:
  http_server(std::vector<std::reference_wrapper<net::io_context>>& io_contexts,
              tcp::endpoint endpoint);
  void run();
  void stop();

 private:
  std::vector<std::reference_wrapper<net::io_context>>& io_contexts_;
  std::vector<std::atomic<int>> io_context_loads_;
  tcp::acceptor acceptor_;
  size_t next_io_context_;

  void do_accept();
};

#endif  // HTTP_SERVER_HPP
