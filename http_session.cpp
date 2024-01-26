#include "http_session.hpp"
#include "globals.hpp"
#include <fstream>
#include <iostream>

std::map<std::string,
         std::function<void(http_session&,
                            const http::request<http::dynamic_body>&)>>
    http_session::routeHandlers;

http_session::http_session(tcp::socket socket) : socket_(std::move(socket)) {}

void http_session::start() { do_read(); }

void http_session::addRoute(const std::string& route,
                                   RequestHandler handler) {
  routeHandlers[route] = handler;
}

void http_session::send_response(const std::string& message) {
  auto res = std::make_shared<http::response<http::string_body>>(
      http::status::ok, req_.version());
  res->set(http::field::server, BOOST_BEAST_VERSION_STRING);
  res->set(http::field::content_type, "text/plain");
  res->body() = message;
  res->prepare_payload();

  // log outgoing response
  std::cout << "\x1b[34m" << res->result_int() << " " << res->reason()
            << "\x1b[0m"
            << " " << res->body().size() << " outgoing bytes" << std::endl;

  auto self = shared_from_this();
  http::async_write(socket_, *res,
                    [self, res](beast::error_code ec, std::size_t) {
                      self->on_write(ec, res->need_eof());
                    });
}

void http_session::send_bad_request(const std::string& message) {
  auto res = std::make_shared<http::response<http::string_body>>(
      http::status::bad_request, req_.version());
  res->set(http::field::server, BOOST_BEAST_VERSION_STRING);
  res->set(http::field::content_type, "text/plain");
  res->body() = message;
  res->prepare_payload();

  auto self = shared_from_this();
  http::async_write(socket_, *res,
                    [self, res](beast::error_code ec, std::size_t) {
                      self->on_write(ec, res->need_eof());
                    });
}

void http_session::do_read() {
  auto self = shared_from_this();
  http::async_read(socket_, buffer_, req_,
                   [self](beast::error_code ec, std::size_t bytes_transferred) {
                     self->on_read(ec, bytes_transferred);
                   });
}

void http_session::on_read(beast::error_code ec,
                           std::size_t bytes_transferred) {
  if (ec) return;  // Handle the error

  // Log the request type ( with color), path, and bytes transfered
  std::cout << "\x1b[32m" << req_.method_string() << " " << req_.target()
            << "\x1b[0m"
            << " " << bytes_transferred << " incoming bytes" << std::endl;

  auto it = routeHandlers.find(req_.target().to_string());
  if (it != routeHandlers.end()) {
    it->second(*this, req_);
  } else {
    // Handle unknown route
    send_bad_request("Route not found");
  }
}

void http_session::on_write(beast::error_code ec, bool close) {
  if (ec) return;  // Handle the error

  if (close) {
    // Close the socket
    socket_.shutdown(tcp::socket::shutdown_send, ec);
    return;
  }

  // Clear the buffer
  buffer_.consume(buffer_.size());

  // Read another request
  do_read();
}
