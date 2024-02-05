#include "http_session.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

#include "globals.hpp"

http_session::http_session(tcp::socket socket): socket_(std::move(socket)) {}

void http_session::start() { do_read(); }

void http_session::send_response(const std::string& message,
                                 const std::string& content_type) {
  auto res = std::make_shared<http::response<http::string_body>>(
      http::status::ok, req_.version());
  res->set(http::field::server, BOOST_BEAST_VERSION_STRING);
  res->set(http::field::content_type, content_type);
  res->set(http::field::cache_control, "public, max-age=2592000");
  res->body() = message;
  res->prepare_payload();

  getGlobalLogger().log(std::to_string(res->result_int()) + " " +
                                   res->reason().to_string() + " " +
                                   std::to_string(res->body().size()) +
                                   " outgoing bytes");

  auto self = shared_from_this();
  http::async_write(
      socket_, *res, [self, res](beast::error_code ec, std::size_t) {
        if (ec) {;
          getGlobalLogger().log("\x1b[31m" + std::string("Error: " + ec.message()) + "\x1b[0m");
          return;
        }

        self->on_write(ec, res->need_eof());
      });
}

void http_session::send_bad_request(const std::string& message) {
  auto res = std::make_shared<http::response<http::string_body>>(
      http::status::bad_request, req_.version());
  res->set(http::field::server, BOOST_BEAST_VERSION_STRING);
  res->set(http::field::content_type, "text/plain");
  res->set(http::field::cache_control, "public, max-age=2592000");
  res->body() = message;
  res->prepare_payload();

  auto self = shared_from_this();
  http::async_write(
      socket_, *res, [self, res](beast::error_code ec, std::size_t) {
        if (ec) {
          getGlobalLogger().log("\x1b[31m" + std::string("Error: " + ec.message()) + "\x1b[0m");
          return;
        }
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
  if (ec) {
    getGlobalLogger().log("\x1b[31m" + std::string("Error: " + ec.message()) + "\x1b[0m");
    return;
  }

  // Log the request type ( with color), path, and bytes transfered

  getGlobalLogger().log("\x1b[32m" + std::string(req_.method_string()) + " " +
                                   req_.target().to_string() + "\x1b[0m" + " " +
                                   std::to_string(bytes_transferred) +
                                   " incoming bytes");

  if (!Router::getInstance().routeRequest(*this, req_)) {
      // If route not found
      handle_fallback();
  }
}

void http_session::handle_fallback() {
  // Send the response
  std::string notFoundMessage =
      "<html><body><h1>404 Not Found</h1><p>The requested resource was not "
      "found on this server.</p></body></html>";
  send_response(notFoundMessage, "text/html");
}

void http_session::on_write(beast::error_code ec, bool close) {
  if (ec) {
    getGlobalLogger().log("\x1b[31m" + std::string("Error: " + ec.message()) + "\x1b[0m");
    if (ec == net::error::broken_pipe || ec == net::error::connection_reset) {
      // Gracefully close the socket
      socket_.close();
    }
    return;
  }

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

void http_session::stream_file(const std::string& file_path,
                               const std::string& content_type) {
  int transfer_id = next_transfer_id++;
  FileTransfer& transfer = file_transfers[transfer_id];

  transfer.file_stream.open(file_path, std::ios::binary);
  if (!transfer.file_stream.is_open()) {
    getGlobalLogger().log("File not found: " + file_path);
    send_bad_request("File not found");
    file_transfers.erase(transfer_id);
    return;
  }

  // Prepare the response header
  auto response = std::make_shared<http::response<http::empty_body>>(
      http::status::ok, req_.version());
  response->set(http::field::server, BOOST_BEAST_VERSION_STRING);
  response->set(http::field::content_type, content_type);
  response->set(http::field::cache_control, "public, max-age=2592000");
  response->keep_alive(req_.keep_alive());
  response->chunked(true);

  // Serialize and send the header
  auto sr =
      std::make_shared<http::response_serializer<http::empty_body>>(*response);
  auto self = shared_from_this();  // Keep the session alive
  http::async_write_header(
      socket_, *sr,
      [self, transfer_id, response, sr](beast::error_code ec, std::size_t) {
        if (!ec) {
          self->do_file_read(transfer_id);
        } else {
          getGlobalLogger().log("Error writing header: " + ec.message());
          self->file_transfers.erase(transfer_id);
        }
      });
}

void http_session::do_file_read(int transfer_id) {
  auto self = shared_from_this();
  FileTransfer& transfer = file_transfers[transfer_id];
  if (!transfer.file_stream.good()) {
    getGlobalLogger().log("Error: File stream is not good");
    file_transfers.erase(transfer_id);
    return;
  }

  transfer.file_stream.read(transfer.buffer.data(), transfer.buffer.size());
  auto bytes_read = transfer.file_stream.gcount();

  if (bytes_read <= 0) {
    // Send the last chunk
    boost::asio::async_write(
        socket_, http::make_chunk_last(),
        [self, transfer_id](beast::error_code ec, std::size_t) {
          if (ec) {
            getGlobalLogger().log("\x1b[31m" + std::string("Error sending last chunk: " + ec.message()) + "\x1b[0m");
          }
          self->file_transfers.erase(transfer_id);
        });
    return;
  }

  // Send a chunk
  boost::asio::const_buffer chunk_body(transfer.buffer.data(), bytes_read);
  boost::asio::async_write(
      socket_, http::make_chunk(chunk_body),
      [self, transfer_id](beast::error_code ec, std::size_t) {
        if (!ec) {
          if (!self->file_transfers[transfer_id].file_stream.eof()) {
            self->do_file_read(transfer_id);
          } else {
            // Send the last chunk
            boost::asio::async_write(
                self->socket_, http::make_chunk_last(),
                [self, transfer_id](beast::error_code ec, std::size_t) {
                  if (ec) {
                    getGlobalLogger().log("\x1b[31m" + std::string("Error sending last chunk: " + ec.message()) + "\x1b[0m");
                  }
                  self->file_transfers.erase(transfer_id);
                });
          }
        } else {
          getGlobalLogger().log("\x1b[31m" + std::string("Error sending chunk: " + ec.message()) + "\x1b[0m");
          self->file_transfers.erase(transfer_id);
        }
      });
}