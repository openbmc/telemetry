#pragma once

#include <sdbusplus/exception.hpp>

#include <ctime>
#include <filesystem>
#include <iostream>
#include <sstream>
#include <string>
#include <system_error>

namespace utils
{

enum class LogLevel
{
    Disable = 0,
    Critical,
    Error,
    Warning,
    Info,
    Debug,
};

class EnabledLogger
{
  public:
    EnabledLogger(const char* prefix, const char* file, const size_t line)
    {
        if constexpr (MS_ENABLE_LOGS_TIMESTAMP)
        {
            std::time_t t = std::time(nullptr);
            stringstream
                << "("
                << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S")
                << ") ";
        }
        stringstream << "[" << prefix << "] ";
        stringstream << "{" << std::filesystem::path(file).filename() << ":"
                     << line << "} ";
    }

    ~EnabledLogger()
    {
        stringstream << "\n";
        std::cerr << stringstream.str();
    }

    template <typename T>
    EnabledLogger& operator<<(T const& value)
    {
        stringstream << value;
        return *this;
    }

  private:
    std::ostringstream stringstream;
};

class DisabledLogger
{
  public:
    DisabledLogger(const char*, const char*, const size_t)
    {}

    template <typename T>
    DisabledLogger& operator<<(T const& value)
    {
        return *this;
    }
};

template <LogLevel level>
using Logger = std::conditional_t<LogLevel{MS_ENABLE_LOGS} >= level, EnabledLogger, DisabledLogger>;

} // namespace utils

#define LOG_COMMON(_level_, _level_name_)                                      \
    utils::Logger<_level_>(_level_name_, __FILE__, __LINE__)

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
