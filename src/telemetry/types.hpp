#pragma once

#include <sdbusplus/message/types.hpp>

#include <tuple>
#include <vector>
#include <string>

using Readings = std::tuple<
    uint64_t,
    std::vector<std::tuple<std::string, std::string, double, uint64_t>>>;
using ReadingParameters =
    std::vector<std::tuple<std::vector<sdbusplus::message::object_path>,
                           std::string, std::string, std::string>>;
