#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <iostream>
#include <string>
#include <thread>
#include <memory>
#include <csignal>

#define isDevMode 1
#define MAX_THREADS 4
#define SERVER_PORT 8080
#define BOOST_BEAST_VERSION_STRING "my_server/1.0"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

class http_session : public std::enable_shared_from_this<http_session> {
    tcp::socket socket_;
    beast::flat_buffer buffer_;
    http::request<http::dynamic_body> req_;

public:
    explicit http_session(tcp::socket socket)
        : socket_(std::move(socket)) {}

    void start() {
        do_read();
    }

private:
    void do_read() {
        auto self = shared_from_this();
        http::async_read(socket_, buffer_, req_,
            [self](beast::error_code ec, std::size_t bytes_transferred) {
                self->on_read(ec, bytes_transferred);
            });
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred) {
        if(ec) return; // Handle the error

        // Log the request type ( with color), path, and bytes transfered
        std::cout << "\x1b[32m" << req_.method_string() << " " << req_.target()
            << "\x1b[0m" << " " << bytes_transferred << " bytes" << std::endl;
        switch (req_.method()) {
          case http::verb::get:
            handle_get();
            break;
          case http::verb::post:
            handle_post();
            break;
          case http::verb::put:
            handle_put();
            break;
          case http::verb::delete_:
            handle_delete();
            break;
          default:
            send_bad_request("Unknown HTTP method");
        }
    }

    void on_write(beast::error_code ec, bool close) {
        if(ec) return; // Handle the error

        if(close) {
            // Close the socket
            socket_.shutdown(tcp::socket::shutdown_send, ec);
            return;
        }

        // Clear the buffer
        buffer_.consume(buffer_.size());

        // Read another request
        do_read();
    }

    void handle_get() {
        // Send the response
        send_response("Hello, world! Received GET request");
    }

    void handle_post() {
        // Send the response
        send_response("Hello, world! Received POST request");
    }

    void handle_put() {
        // Send the response
        send_response("Hello, world! Received PUT request");
    }

    void handle_delete() {
        // Send the response
        send_response("Hello, world! Received DELETE request");
    }

    void send_bad_request(const std::string& message) {
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

    void send_response(const std::string& message) {
        auto res = std::make_shared<http::response<http::string_body>>(
            http::status::ok, req_.version());
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
};

// singleton http server
class http_server {
    net::io_context& ioc_;
    tcp::acceptor acceptor_;

public:
    http_server(net::io_context& ioc, tcp::endpoint endpoint)
        : ioc_(ioc), acceptor_(net::make_strand(ioc)) {
        beast::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if(ec) {
            throw std::runtime_error("Failed to open acceptor: " + ec.message());
        }

        #if isDevMode
          acceptor_.set_option(net::socket_base::reuse_address(true), ec);
          if(ec) {
              throw std::runtime_error("Failed to set SO_REUSEADDR: " + ec.message());
          }
        #endif

        // Bind to the server address
        acceptor_.bind(endpoint, ec);
        if(ec) {
            throw std::runtime_error("Failed to bind: " + ec.message());
        }

        // Start listening for connections
        acceptor_.listen(net::socket_base::max_listen_connections, ec);
        if(ec) {
            throw std::runtime_error("Failed to listen: " + ec.message());
        }
    }

    // Start accepting incoming connections
    void run() {
        do_accept();
    }

    void stop() {
        // Close the acceptor
        beast::error_code ec;
        acceptor_.close(ec);
        if(ec) {
            // Log the error
            std::cerr << "Failed to close acceptor: " << ec.message() << std::endl;
        }

        // Stop the io_context
        ioc_.stop();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            [this](beast::error_code ec, tcp::socket socket) {
                if(!ec) {
                    std::make_shared<http_session>(std::move(socket))->start();
                }
                do_accept();
            });
    }
};

std::unique_ptr<http_server> server;

void signal_handler(int signal) {
    if (signal == SIGINT) {
        std::cout << "Received SIGINT, shutting down" << std::endl;
        server->stop();
    }
}

int main() {
    try {
        std::cout << "Starting server on port " << SERVER_PORT << std::endl;
        net::io_context ioc{1};
        server = std::make_unique<http_server>(ioc, tcp::endpoint(tcp::v4(), SERVER_PORT));
        std::signal(SIGINT, signal_handler);
        server->run();

        // Run the I/O service on the requested number of threads
        std::vector<std::thread> v;
        v.reserve(MAX_THREADS);
        for(auto i = v.capacity(); i > 0; --i) {
            v.emplace_back([&ioc] { ioc.run(); });
        }
        for(auto& t : v) {
            t.join();
        }
    }
    catch (std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}


