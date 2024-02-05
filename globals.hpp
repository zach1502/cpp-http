#ifndef GLOBALS_HPP
#define GLOBALS_HPP

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <mutex>
#include <unordered_map>
#include "http_session.hpp"
#include "http_server.hpp"

#define isDevMode 1
#define MAX_THREADS std::thread::hardware_concurrency()
#define SERVER_PORT 8080
#define BOOST_BEAST_VERSION_STRING "my_server/1.0"

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

using RequestHandler = std::function<void(
    http_session&, const http::request<http::dynamic_body>&)>;
using RouteHandlers = std::map<std::string, RequestHandler>;

void handle_root(http_session& session, const http::request<http::dynamic_body>& req);

#endif  // GLOBALS_HPP
