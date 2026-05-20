#pragma once
#include <cstdint>
#include <string>

#ifdef LOG_LEVEL
    #define DEFAULT_LOG_LEVEL LOG_LEVEL
#else
    #define DEFAULT_LOG_LEVEL 2
#endif

#ifdef LOG_FORMAT
    #define DEFAULT_LOG_FORMAT LOG_FORMAT
#else
    #define DEFAULT_LOG_FORMAT (1 << 4)
#endif

class Logger
{
    enum Level
    {
        ERROR,
        WARN,
        INFO,
        DEBUG,
        TRACE,
        ALL,
    };
    enum Format : uint8_t
    {
        NONE = 0,
        DATE = 1 << 0,
        TIME = 1 << 1,
        LOGGING_LEVEL = 1 << 2,
        FILE_LINE = 1 << 3,
        DEFAULT = 1 << 4
    };
    Level level = static_cast<Level>(DEFAULT_LOG_LEVEL);
    Format format = static_cast<Format>(DEFAULT_LOG_FORMAT);
    std::string levels[5] = { "ERROR", "WARN", "INFO", "DEBUG", "ALL" };
public:
    Logger() = default;
    void setLevel(const Level &level);
    std::string getLevel(void);
    std::string getLevel(const Level &level);
    void log(const std::string &msg, const Level &level);
    std::string timestamp();
    static Logger *getLogger();
    static void info(const std::string &msg);
    static void error(const std::string &msg);
    static void warning(const std::string &msg);
    static void debug(const std::string &msg);
};

void DebugPrintString(std::string_view Str, bool HexOnly = true);