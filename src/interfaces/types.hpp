#pragma once

#include <sdbusplus/message/types.hpp>

#include <string>
#include <tuple>
#include <vector>

using ReadingParameters =
    std::vector<std::tuple<std::vector<sdbusplus::message::object_path>,
                           std::string, std::string, std::string>>;
