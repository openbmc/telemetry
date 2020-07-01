#pragma once

#include <cstdio>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>

namespace utils
{
enum class LogLevel
{
    debug = 0,
    info,
    warning,
    error,
    critical,
};

class Logger
{
  public:
    Logger(const std::string& prefix, const std::string& file,
           const size_t line, LogLevel levelIn, const std::string& tag = {}) :
        level(levelIn)
    {
#ifdef ENABLE_LOGS
        stringstream << "(" << timestamp() << ") ";
        stringstream << "[" << prefix << "] ";
        stringstream << "{" << std::filesystem::path(file).filename().string()
                     << ":" << line << "} ";
#endif
    }

    ~Logger()
    {
        if (level >= getLogLevel())
        {
#ifdef ENABLE_LOGS
            stringstream << std::endl;
            std::cerr << stringstream.str();
#endif
        }
    }

    //
    template <typename T>
    Logger& operator<<(T const& value)
    {
        if (level >= getLogLevel())
        {
#ifdef ENABLE_LOGS
            stringstream << value;
#endif
        }
        return *this;
    }

    static void setLogLevel(LogLevel level)
    {
        getLogLevelRef() = level;
    }

    static LogLevel getLogLevel()
    {
        return getLogLevelRef();
    }

  private:
    static LogLevel& getLogLevelRef()
    {
        static auto currentLevel = LogLevel::debug;
        return currentLevel;
    }

    static std::string timestamp()
    {
        std::string date;
        date.resize(32, '\0');
        time_t t = time(nullptr);

        tm myTm{};

        gmtime_r(&t, &myTm);

        size_t sz =
            strftime(date.data(), date.size(), "%Y-%m-%d %H:%M:%S", &myTm);
        date.resize(sz);
        return date;
    }

    std::ostringstream stringstream;
    LogLevel level;
};
} // namespace utils

#define _LOG_COMMON(_level_, _level_name_)                                     \
    if (utils::Logger::getLogLevel() <= (_level_))                             \
    utils::Logger((_level_name_), __FILE__, __LINE__, (_level_))

#define LOG_CRITICAL _LOG_COMMON(utils::LogLevel::critical, "CRITICAL")
#define LOG_ERROR _LOG_COMMON(utils::LogLevel::error, "ERROR")
#define LOG_WARNING _LOG_COMMON(utils::LogLevel::warning, "WARNING")
#define LOG_INFO _LOG_COMMON(utils::LogLevel::info, "INFO")
#define LOG_DEBUG _LOG_COMMON(utils::LogLevel::debug, "DEBUG")

#define LOG_CRITICAL_T(_tag_) LOG_CRITICAL << "[" << (_tag_) << "] "
#define LOG_ERROR_T(_tag_) LOG_ERROR << "[" << (_tag_) << "] "
#define LOG_WARNING_T(_tag_) LOG_WARNING << "[" << (_tag_) << "] "
#define LOG_INFO_T(_tag_) LOG_INFO << "[" << (_tag_) << "] "
#define LOG_DEBUG_T(_tag_) LOG_DEBUG << "[" << (_tag_) << "] "
