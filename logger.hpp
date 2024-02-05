// ThreadSafeQueue remains unchanged, so it's not repeated here.

#ifndef LOGGER_HPP
#define LOGGER_HPP

#include "thread_safe_queue.hpp"
#include <string>
#include <thread>
#include <atomic>
#include <mutex>

class Logger {
private:
    ThreadSafeQueue<std::string> queue_;
    std::thread loggingThread_;
    std::atomic<bool> terminate_;
    static std::mutex coutMutex_;

    Logger();
    ~Logger();

    void processMessages();

public:
    Logger(const Logger&) = delete;
    Logger& operator=(const Logger&) = delete;

    static Logger& getInstance();

    void log(const std::string& message);
};

#endif // LOGGER_HPP
