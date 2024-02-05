#include "globals.hpp"

Logger& getGlobalLogger() {
    static Logger& instance = Logger::getInstance();
    return instance;
}