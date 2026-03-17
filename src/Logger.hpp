#pragma once
#include <string>

enum Level
{
    ALL,
    DEBUG,
    INFO,
    WARN,
    ERROR,
};

#ifdef LOG_LEVEL
    #define DEFAULT_LOG_LEVEL LOG_LEVEL;
#else
    #define DEFAULT_LOG_LEVEL INFO;
#endif

class Logger
{
    Level level = DEFAULT_LOG_LEVEL;
    std::string levels[5] = { "ALL", "DEBUG", "INFO", "WARN", "ERROR" };
public:
    Logger() = default;
    void setLevel(const Level &level);
    std::string getLevel(void);
    std::string getLevel(const Level &level);
    void log(const std::string &msg, const Level &level);
    static Logger *getLogger();
    static void info(const std::string &msg);
    static void error(const std::string &msg);
    static void warning(const std::string &msg);
    static void debug(const std::string &msg);
};
