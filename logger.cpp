#include "logger.hpp"
#include "thread_safe_queue.hpp"
#include <iostream>

std::mutex Logger::coutMutex_;

Logger::Logger() : terminate_(false) {
    loggingThread_ = std::thread(&Logger::processMessages, this);
}

Logger::~Logger() {
    terminate_ = true;
    queue_.push(""); // Push an empty message to unblock the thread
    if (loggingThread_.joinable()) {
        loggingThread_.join();
    }
}

void Logger::processMessages() {
    while (!terminate_ || !queue_.empty()) {
        auto message = queue_.pop();
        if (message) {
            std::lock_guard<std::mutex> lock(coutMutex_);
            std::cout << *message << std::endl;
        }
    }
}

Logger& Logger::getInstance() {
    static Logger instance;
    return instance;
}

void Logger::log(const std::string& message) {
    queue_.push(message);
}
