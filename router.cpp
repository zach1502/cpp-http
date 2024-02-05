#include "router.hpp"

void Router::addRoute(const std::string& route, RequestHandler handler) {
    routeHandlers[route] = std::move(handler);
}

bool Router::routeRequest(http_session& session, const boost::beast::http::request<boost::beast::http::dynamic_body>& req) {
    auto it = routeHandlers.find(req.target().to_string());
    if (it != routeHandlers.end()) {
        it->second(session, req);
        return true;
    } else {
        // Route not found
        return false;
    }
}
