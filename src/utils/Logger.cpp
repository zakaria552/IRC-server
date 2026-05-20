#include "Logger.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <string>

void Logger::log(const std::string &msg, const Level &level)
{
    if (level > this->level)
        return;
    std::string timestamp = this->timestamp();
    if (!timestamp.empty())
        std::cout << "[" << timestamp << "]";
    if (format & Format::LOGGING_LEVEL || format & Format::DEFAULT)
        std::cout << "[" << getLevel(level) << "]";
    if (format != Format::NONE)
        std::cout << " - " ;
    std::cout << msg << std::endl;
}

void Logger::info(const std::string &msg)
{
     Logger *logger = Logger::getLogger();
     logger->log(msg, Level::INFO);
}

void Logger::warning(const std::string &msg)
{
     Logger *logger = Logger::getLogger();
     logger->log(msg, Level::WARN);
}

void Logger::error(const std::string &msg)
{
     Logger *logger = Logger::getLogger();
     logger->log(msg, Level::ERROR);
}

void Logger::debug(const std::string &msg)
{
    Logger *logger = Logger::getLogger();
    logger->log(msg, Level::DEBUG);
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
    std::string timeFormat;
    if (format & Format::DATE || format & Format::DEFAULT)
    {
        timeFormat += "%Y-%m-%d";
        if (format & TIME || format & Format::DEFAULT)
            timeFormat += " ";
    }
    if (format & TIME || format & Format::DEFAULT)
        timeFormat += "%H:%M:%S";
    oss << std::put_time(&tm, timeFormat.c_str());
    return oss.str();
}

#include <stdio.h> // REMOVE!
void DebugPrintString(std::string_view Str, bool HexOnly)
{
    if (not HexOnly)
    {
        for (auto const& c : Str)
        {
            fprintf(stderr, "%c", c);
        }
        fprintf(stderr, "\r\n");
    }

    std::string_view HexString = "0123456789ABCDEF";
    for (auto const& c : Str)
    {
        fprintf(stderr, "%c%c ", HexString[c >> 4], HexString[c & 0b1111]);
    }
    fprintf(stderr, "\r\n");
}