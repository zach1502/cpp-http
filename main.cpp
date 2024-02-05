#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>

#include "globals.hpp"
#include "http_server.hpp"
#include "router.hpp"
namespace fs = std::filesystem;

#define SERVER_PORT 8080

std::unique_ptr<http_server> server;

void signal_handler(int signal) {
  if (signal == SIGINT) {
    std::cout << "Received SIGINT, shutting down" << std::endl;
    server->stop();
  }
}

std::unordered_map<std::string, std::string> extension_to_mime = {
    {".html", "text/html"},
    {".css", "text/css"},
    {".js", "text/javascript"},
    {".json", "application/json"},
    {".png", "image/png"},
    {".jpg", "image/jpeg"},
    {".jpeg", "image/jpeg"},
    {".gif", "image/gif"},
    {".svg", "image/svg+xml"},
    {".ico", "image/x-icon"}
};

std::string determine_content_type(const std::string& extension) {
  auto it = extension_to_mime.find(extension);
  if (it != extension_to_mime.end()) {
    return it->second;
  }

  return "UNSUPPORTED";
}

void add_all_files_in_directory() {
  auto& router = Router::getInstance();
  const std::string directory_path = "./";
  for (const auto& entry : fs::recursive_directory_iterator(directory_path)) {
    if (fs::is_regular_file(entry.status())) {
      std::string file_path = entry.path().string();
      std::string route =
          file_path.substr(directory_path.length());  // Remove directory path

      // do not go into files starting with a dot
      if (route[0] == '.') {
        continue;
      }

      // Determine the MIME type based on the file extension
      std::string content_type =
          determine_content_type(entry.path().extension().string());

      if (content_type == "UNSUPPORTED") {
        std::cout << "Skipping file " << file_path
                  << " with unsupported MIME type" << std::endl;
        continue;
      }

      // this slash is needed for the route to work
      route = "/" + route;

      std::cout << "Adding route " << route << " for file " << file_path
                << " with content type " << content_type << std::endl;

      // Add the route
      router.addRoute(route,
                             [file_path, content_type](
                                 http_session& session,
                                 const http::request<http::dynamic_body>& req) {
                               session.stream_file(file_path, content_type);
                             });
    }
  }
}

void handle_root(http_session& session,
                 const http::request<http::dynamic_body>& req) {
  session.stream_file("index.html", "text/html");
}

int main() {
  try {
    std::signal(SIGINT, signal_handler);
    std::cout << "Starting server on port " << SERVER_PORT << std::endl;
    std::cout << "Press Ctrl+C to stop" << std::endl;

    std::cout << "max threads: " << MAX_THREADS << std::endl;
    std::cout << "max listen connections: "
              << net::socket_base::max_listen_connections << std::endl;

    const std::size_t num_contexts = MAX_THREADS;
    std::vector<net::io_context> io_contexts(num_contexts);
    std::vector<net::executor_work_guard<net::io_context::executor_type>> work_guards;

    for (auto& ctx : io_contexts) {
        work_guards.emplace_back(net::make_work_guard(ctx));
    }

    std::vector<std::reference_wrapper<net::io_context>> io_context_refs;
    for (auto& ctx : io_contexts) {
        io_context_refs.push_back(std::ref(ctx));
    }
    server = std::make_unique<http_server>(io_context_refs, tcp::endpoint(tcp::v4(), SERVER_PORT));
    server->run();

    // add routes
    auto& router = Router::getInstance();
    router.addRoute("/", handle_root);
    add_all_files_in_directory();

    std::vector<std::thread> threads;
    for (auto& ctx : io_contexts) {
        threads.emplace_back([&ctx] { ctx.run(); });
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
