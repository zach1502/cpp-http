#include "http_server.hpp"

#include <iostream>

#include "http_session.hpp"

http_server::http_server(net::io_context& ioc, tcp::endpoint endpoint)
    : ioc_(ioc), acceptor_(net::make_strand(ioc)) {
  beast::error_code ec;

  // Open the acceptor
  acceptor_.open(endpoint.protocol(), ec);
  if (ec) {
    throw std::runtime_error("Failed to open acceptor: " + ec.message());
  }

#if isDevMode
  acceptor_.set_option(net::socket_base::reuse_address(true), ec);
  if (ec) {
    throw std::runtime_error("Failed to set SO_REUSEADDR: " + ec.message());
  }
#endif

  // Bind to the server address
  acceptor_.bind(endpoint, ec);
  if (ec) {
    throw std::runtime_error("Failed to bind: " + ec.message());
  }

  // Start listening for connections
  acceptor_.listen(net::socket_base::max_listen_connections, ec);
  if (ec) {
    throw std::runtime_error("Failed to listen: " + ec.message());
  }
}

// Start accepting incoming connections
void http_server::run() { do_accept(); }

void http_server::stop() {
  // Close the acceptor
  beast::error_code ec;
  acceptor_.close(ec);
  if (ec) {
    // Log the error
    std::cerr << "Failed to close acceptor: " << ec.message() << std::endl;
  }

  // Stop the io_context
  ioc_.stop();
}

void http_server::do_accept() {
  acceptor_.async_accept([this](beast::error_code ec, tcp::socket socket) {
    if (!ec) {
      std::make_shared<http_session>(std::move(socket))->start();
    }
    do_accept();
  });
}
