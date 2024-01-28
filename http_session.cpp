#include "http_session.hpp"

#include <filesystem>
#include <fstream>
#include <iostream>

#include "globals.hpp"

std::map<std::string,
         std::function<void(http_session&,
                            const http::request<http::dynamic_body>&)>>
    http_session::routeHandlers;

std::ifstream file_stream;
http_session::http_session(tcp::socket socket) : socket_(std::move(socket)) {}

void http_session::start() { do_read(); }

void http_session::addRoute(const std::string& route, RequestHandler handler) {
  routeHandlers[route] = handler;
}

void http_session::send_response(const std::string& message, const std::string &content_type) {
  auto res = std::make_shared<http::response<http::string_body>>(
      http::status::ok, req_.version());
  res->set(http::field::server, BOOST_BEAST_VERSION_STRING);
  res->set(http::field::content_type, content_type);
  res->body() = message;
  res->prepare_payload();

  // log outgoing response
  // cout_mutex.lock();
  std::cout << "\x1b[34m" << res->result_int() << " " << res->reason()
            << "\x1b[0m"
            << " " << res->body().size() << " outgoing bytes" << std::endl;
  // cout_mutex.unlock();

  auto self = shared_from_this();
  http::async_write(socket_, *res,
                    [self, res](beast::error_code ec, std::size_t) {
                      if (ec) {
                        std::cerr << "\x1b[31m" << "Error: " << ec.message() << "\x1b[0m" << std::endl;
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
  res->body() = message;
  res->prepare_payload();

  auto self = shared_from_this();
  http::async_write(socket_, *res,
                    [self, res](beast::error_code ec, std::size_t) {
                      if (ec) {
                        std::cerr << "\x1b[31m" << "Error: " << ec.message() << "\x1b[0m" << std::endl;
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
    std::cerr << "\x1b[31m" << "Error: " << ec.message() << "\x1b[0m" << std::endl;
    return;
  }

  // Log the request type ( with color), path, and bytes transfered
  std::cout << "\x1b[32m" << req_.method_string() << " " << req_.target()
            << "\x1b[0m"
            << " " << bytes_transferred << " incoming bytes" << std::endl;

  auto it = routeHandlers.find(req_.target().to_string());
  if (it != routeHandlers.end()) {
    it->second(*this, req_);
  } else {
    // Handle unknown route
    handle_fallback();
  }
}

void http_session::handle_fallback() {
  // Send the response
  std::string notFoundMessage = "<html><body><h1>404 Not Found</h1><p>The requested resource was not found on this server.</p></body></html>";
  send_response(notFoundMessage, "text/html");
}

void http_session::on_write(beast::error_code ec, bool close) {
  if (ec) {
    std::cerr << "\x1b[31m" << "Error: " << ec.message() << "\x1b[0m" << std::endl;
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

void http_session::stream_file(const std::string& file_path, const std::string &content_type) {
  int transfer_id = next_transfer_id++;
  FileTransfer& transfer = file_transfers[transfer_id];

  transfer.file_stream.open(file_path, std::ios::binary);
  if (!transfer.file_stream.is_open()) {
    std::cout << "File not found: " << file_path << std::endl;
    send_bad_request("File not found");
    file_transfers.erase(transfer_id);
    return;
  }

  // Prepare the response header
  http::response<http::empty_body> res{http::status::ok, 11};
  res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
  res.set(http::field::content_type, content_type);
  res.chunked(true);

  // Serialize and send the header
  http::response_serializer<http::empty_body> sr{res};
  http::write_header(socket_, sr);

  try {
    // Send the first chunk
    do_file_read(transfer_id);
  } catch (std::exception& e) {
    std::cerr << "Error happened during file streaming: " << e.what() << std::endl;
    file_transfers.erase(transfer_id);
    return;
  }
}

void http_session::do_file_read(int transfer_id){
    FileTransfer& transfer = file_transfers[transfer_id];
    if (!transfer.file_stream.good()) {
        std::cerr << "Error: File stream is not good\n";
        file_transfers.erase(transfer_id);
        return;
    }

    transfer.file_stream.read(transfer.buffer.data(), transfer.buffer.size());
    auto bytes_read = transfer.file_stream.gcount();

    if (bytes_read <= 0) {
        // Send the last chunk
        boost::asio::write(socket_, http::make_chunk_last());
        transfer.file_stream.close();
        file_transfers.erase(transfer_id);
        return;
    }

    // Send a chunk
    boost::asio::const_buffer chunk_body(transfer.buffer.data(), bytes_read);
    boost::asio::write(socket_, http::make_chunk(chunk_body));

    // log outgoing response
    std::cout << "\x1b[34m" << "200 OK" << "\x1b[0m"
        << " " << bytes_read << " outgoing bytes" << std::endl;

    if (!transfer.file_stream.eof()) {
        do_file_read(transfer_id);
    } else {
        // Send the last chunk
        std::cout << "Sending last chunk\n";
        boost::asio::write(socket_, http::make_chunk_last());
        transfer.file_stream.close();
        file_transfers.erase(transfer_id);
    }
}