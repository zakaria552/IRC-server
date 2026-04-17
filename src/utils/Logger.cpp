#include "Logger.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>

void Logger::log(const std::string &msg, const Level &level)
{
    if (level < this->level)
        return;
    std::cout << "[" << timestamp() << "][" << getLevel(level) << "] - " << msg << std::endl;
}

void Logger::info(const std::string &msg)
{
     Logger *logger = Logger::getLogger();
     logger->log(msg, INFO);
}

void Logger::warning(const std::string &msg)
{
     Logger *logger = Logger::getLogger();
     logger->log(msg, WARN);
}

void Logger::error(const std::string &msg)
{
     Logger *logger = Logger::getLogger();
     logger->log(msg, ERROR);
}

void Logger::debug(const std::string &msg)
{
    Logger *logger = Logger::getLogger();
    logger->log(msg, DEBUG);
}

 void Logger::setLevel(const Level &level)
{
    this->level = level;
}

std::string Logger::getLevel(void)
{
     return this->levels[level];
}

std::string Logger::getLevel(const Level &level)
{
    return this->levels[level];
}

 Logger *Logger::getLogger(void)
{
    static Logger log = Logger();
    return &log;
}

std::string Logger::timestamp()
{
    const auto now = std::chrono::system_clock::now();
    std::time_t t = std::chrono::system_clock::to_time_t(now);
    std::tm tm = *std::localtime(&t);
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}
