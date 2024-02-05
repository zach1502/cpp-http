#ifndef ROUTER_HPP
#define ROUTER_HPP

#include "http_session.hpp"
#include <boost/beast/http.hpp>
#include <functional>
#include <map>
#include <string>

class Router {
public:
    using RequestHandler = std::function<void(http_session&, const boost::beast::http::request<boost::beast::http::dynamic_body>&)>;

private:
    std::map<std::string, RequestHandler> routeHandlers;
    Router() {} // Private constructor

public:
    Router(const Router&) = delete; // Delete copy constructor
    Router& operator=(const Router&) = delete; // Delete copy assignment operator

    static Router& getInstance() {
        static Router instance; // Guaranteed to be destroyed and instantiated on first use
        return instance;
    }

    void addRoute(const std::string& route, RequestHandler handler);
    bool routeRequest(http_session& session, const boost::beast::http::request<boost::beast::http::dynamic_body>& req);
};

#endif // ROUTER_HPP
