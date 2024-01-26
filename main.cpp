#include "http_server.hpp"
#include "globals.hpp"

#include <filesystem>
#include <iostream>
#include <fstream>
#include <csignal>
#include <thread>
#include <mutex>
namespace fs = std::filesystem;

#define MAX_THREADS 4
#define SERVER_PORT 8080

void handle_root(http_session& session, const http::request<http::dynamic_body>& req) {
    session.stream_file("index.html", "text/html");
}

std::unique_ptr<http_server> server;

void signal_handler(int signal) {
  if (signal == SIGINT) {
    cout_mutex.lock();
    std::cout << "Received SIGINT, shutting down" << std::endl;
    cout_mutex.unlock();
    server->stop();
  }
}

std::string determine_content_type(const std::string& extension) {
    if (extension == ".html") return "text/html";
    else if (extension == ".js") return "text/javascript";
    else if (extension == ".css") return "text/css";
    else if (extension == ".jpg" || extension == ".jpeg") return "image/jpeg";
    else if (extension == ".png") return "image/png";
    else if (extension == ".ico") return "image/x-icon";
    else return "application/octet-stream"; // Default binary type
}

int main() {
    try {
        std::signal(SIGINT, signal_handler);
        std::cout << "Starting server on port " << SERVER_PORT << std::endl;
        
        net::io_context ioc{1};
        server = std::make_unique<http_server>(ioc, tcp::endpoint(tcp::v4(), SERVER_PORT));

        // default route
        http_session::addRoute("/", handle_root);

        const std::string directory_path = "./";
        for (const auto& entry : fs::directory_iterator(directory_path)) {
            if (fs::is_regular_file(entry.status())) {
                std::string file_path = entry.path().string();
                std::string route = file_path.substr(directory_path.length()); // Remove directory path

                // Determine the MIME type based on the file extension
                std::string content_type = determine_content_type(entry.path().extension().string());
                
                // this slash is needed for the route to work
                route = "/" + route;

                std::cout << "Adding route " << route << " for file " << file_path << " with content type " << content_type << std::endl;

                // Add the route
                http_session::addRoute(route, [file_path, content_type](http_session& session, const http::request<http::dynamic_body>& req) {
                    session.stream_file(file_path, content_type);
                });
            }
        }

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
