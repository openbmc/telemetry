#pragma once

#include "types/readings.hpp"
#include "types/report_action.hpp"

#include <ostream>

#include <gmock/gmock.h>

template <class Param>
void PrintTo(const std::vector<Param>& vec, std::ostream* os);

inline void PrintTo(const ReadingData& data, std::ostream* os)
{
    const auto& [id, value, timestamp] = data;
    *os << "( " << id << ", " << std::to_string(value) << ", "
        << std::to_string(timestamp) << " )";
}

inline void PrintTo(const Readings& readings, std::ostream* os)
{
    const auto& [timestamp, readingDataVec] = readings;
    *os << "( " << std::to_string(timestamp) << ", ";
    PrintTo(readingDataVec, os);
    *os << " )";
}

inline void PrintTo(const ReportAction& action, std::ostream* os)
{
    *os << utils::enumToString(action);
}
