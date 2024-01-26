#include "http_server.hpp"
#include "globals.hpp"
#include <iostream>
#include <fstream>
#include <csignal>
#include <thread>

#define MAX_THREADS 4
#define SERVER_PORT 8080

void handle_root(http_session& session, const http::request<http::dynamic_body>& req) {
    session.send_response("Welcome to the root!", "text/html");
}

void handle_about(http_session& session, const http::request<http::dynamic_body>& req) {
    session.send_response("About page", "text/html");
}

void handle_favorite_icon(http_session& session, const http::request<http::dynamic_body>& req) {
    session.stream_file("favicon.ico", "image/x-icon");
}

std::unique_ptr<http_server> server;

void signal_handler(int signal) {
  if (signal == SIGINT) {
    std::cout << "Received SIGINT, shutting down" << std::endl;
    server->stop();
  }
}

int main() {
    try {
        std::signal(SIGINT, signal_handler);
        std::cout << "Starting server on port " << SERVER_PORT << std::endl;
        
        net::io_context ioc{1};
        server = std::make_unique<http_server>(ioc, tcp::endpoint(tcp::v4(), SERVER_PORT));

        http_session::addRoute("/", handle_root);
        http_session::addRoute("/about", handle_about);
        http_session::addRoute("/favicon.ico", handle_favorite_icon);

        server->run();

        std::vector<std::thread> threads(MAX_THREADS);
        for (auto& t : threads) {
            t = std::thread([&ioc] { ioc.run(); });
        }
        for (auto& t : threads) {
            t.join();
        }
    } catch (std::exception const& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
