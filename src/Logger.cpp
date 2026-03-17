#include "Logger.hpp"
#include <iostream>

void Logger::log(const std::string &msg, const Level &level)
{
    if (level < this->level)
        return;
    std::cout << "[" << getLevel(level) << "] - " << msg << std::endl;
}

void Logger::info(const std::string &msg)
{
     Logger *logger = Logger::getLogger();
     logger->log(msg, INFO);
}

void Logger::warning(const std::string &msg)
{
     Logger *logger = Logger::getLogger();
     logger->log(msg, INFO);
}

void Logger::error(const std::string &msg)
{
     Logger *logger = Logger::getLogger();
     logger->log(msg, INFO);
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
