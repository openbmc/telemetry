#pragma once

#include <ctime>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

namespace utils
{

enum class LogLevel
{
    Debug = 0,
    Info,
    Warning,
    Error,
    Critical,
    Disable,
};

class Logger
{
  public:
    Logger(const std::string& prefix, const std::string& filename,
           const size_t line)
    {
        if (MS_ENABLE_LOGS_TIMESTAMP)
        {
            stringstream << "(" << timestamp() << ") ";
        }
        stringstream << "[" << prefix << "] ";
        stringstream << "{" << filename << ":" << line << "} ";
    }

    ~Logger()
    {
        stringstream << "\n";
        std::cerr << stringstream.str();
    }

    template <typename T>
    Logger& operator<<(T const& value)
    {
        stringstream << value;
        return *this;
    }

    static void setLogLevel(LogLevel level)
    {
        if (MS_ENABLE_LOGS)
        {
            getLogLevelRef() = level;
        }
    }

    static LogLevel getLogLevel()
    {
        if (MS_ENABLE_LOGS)
        {
            return getLogLevelRef();
        }
        else
        {
            return LogLevel::Disable;
        }
    }

  private:
    static LogLevel& getLogLevelRef()
    {
        static LogLevel currentLevel = LogLevel::Debug;
        return currentLevel;
    }

    static std::string timestamp()
    {
        char stamp[sizeof("YYYY-mm-dd HH:MM:SS")] = {0};
        std::time_t time = std::time(nullptr);
        size_t sz = strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S",
                             std::gmtime(&time));
        if (sz != (sizeof(stamp) - 1))
        {
            return "";
        }
        return stamp;
    }

    std::ostringstream stringstream;
};
} // namespace utils

#define LOG_COMMON(_level_, _level_name_)                                      \
    if (utils::Logger::getLogLevel() <= _level_)                               \
    utils::Logger(_level_name_, std::filesystem::path(__FILE__).filename(),    \
                  __LINE__)

#define LOG_CRITICAL LOG_COMMON(utils::LogLevel::Critical, "CRITICAL")
#define LOG_ERROR LOG_COMMON(utils::LogLevel::Error, "ERROR")
#define LOG_WARNING LOG_COMMON(utils::LogLevel::Warning, "WARNING")
#define LOG_INFO LOG_COMMON(utils::LogLevel::Info, "INFO")
#define LOG_DEBUG LOG_COMMON(utils::LogLevel::Debug, "DEBUG")

#define LOG_CRITICAL_T(_tag_) LOG_CRITICAL << "[" << (_tag_) << "] "
#define LOG_ERROR_T(_tag_) LOG_ERROR << "[" << (_tag_) << "] "
#define LOG_WARNING_T(_tag_) LOG_WARNING << "[" << (_tag_) << "] "
#define LOG_INFO_T(_tag_) LOG_INFO << "[" << (_tag_) << "] "
#define LOG_DEBUG_T(_tag_) LOG_DEBUG << "[" << (_tag_) << "] "
