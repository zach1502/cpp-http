// http_session.hpp
#ifndef HTTP_SESSION_HPP
#define HTTP_SESSION_HPP

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/write.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <functional>
#include <memory>
#include <string>
#include <map>
#include <fstream>
#include <vector>

namespace beast = boost::beast;  // from <boost/beast.hpp>
namespace http = beast::http;    // from <boost/beast/http.hpp>
namespace net = boost::asio;     // from <boost/asio.hpp>
using tcp = net::ip::tcp;        // from <boost/asio/ip/tcp.hpp>

struct FileTransfer {
    std::ifstream file_stream;
    std::vector<char> buffer;
    std::shared_ptr<http::response<http::buffer_body>> response;

    FileTransfer() : buffer(4096) { /* ... */ }
};

class http_session : public std::enable_shared_from_this<http_session> {
public:
    explicit http_session(tcp::socket socket);
    void start();

    static void addRoute(const std::string& route, std::function<void(http_session&, const http::request<http::dynamic_body>&)> handler);
    void send_response(const std::string& message, const std::string &content_type);
    void send_bad_request(const std::string& message);
    void stream_file(const std::string& file_path, const std::string &content_type);

private:
    tcp::socket socket_;
    beast::flat_buffer buffer_;
    http::request<http::dynamic_body> req_;
    std::ifstream file_stream;
    std::unordered_map<int, FileTransfer> file_transfers;
    int next_transfer_id = 0;

    void do_read();
    void on_read(beast::error_code ec, std::size_t bytes_transferred);
    void on_write(beast::error_code ec, bool close);
    void handle_fallback();

    void do_file_read(int transfer_id);

    static std::map<std::string, std::function<void(http_session&, const http::request<http::dynamic_body>&)>> routeHandlers;
};

#endif  // HTTP_SESSION_HPP
