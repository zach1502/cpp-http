#include <csignal>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <thread>

#include "globals.hpp"
#include "http_server.hpp"
namespace fs = std::filesystem;

#define SERVER_PORT 8080

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
  if (extension == ".html")
    return "text/html";
  else if (extension == ".js")
    return "text/javascript";
  else if (extension == ".css")
    return "text/css";
  else if (extension == ".jpg" || extension == ".jpeg")
    return "image/jpeg";
  else if (extension == ".png")
    return "image/png";
  else if (extension == ".ico")
    return "image/x-icon";
  else if (extension == ".svg")
    return "image/svg+xml";
  else if (extension == ".json")
    return "application/json";
  else if (extension == ".pdf")
    return "application/pdf";
  else if (extension == ".zip")
    return "application/zip";
  else if (extension == ".gz")
    return "application/gzip";
  else if (extension == ".mp3")
    return "audio/mpeg";
  else if (extension == ".mp4")
    return "video/mp4";

  else
    return "UNSUPPORTED";
}

void add_all_files_in_directory() {
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
      http_session::addRoute(route,
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

    const std::size_t num_contexts = std::thread::hardware_concurrency();
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
    http_session::addRoute("/", handle_root);
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
