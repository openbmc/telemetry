#pragma once

#include <boost/asio/spawn.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <array>
#include <string>
#include <utility>
#include <vector>

namespace utils
{

using SensorPath = std::string;
using ServiceName = std::string;
using Ifaces = std::vector<std::string>;
using SensorIfaces = std::vector<std::pair<ServiceName, Ifaces>>;
using SensorTree = std::pair<SensorPath, SensorIfaces>;

inline std::vector<SensorTree>
    getSubTreeSensors(boost::asio::yield_context& yield,
                      const std::shared_ptr<sdbusplus::asio::connection>& bus)
{
    std::array<const char*, 1> interfaces = {
        "xyz.openbmc_project.Sensor.Value"};
    boost::system::error_code ec;

    auto tree = bus->yield_method_call<std::vector<SensorTree>>(
        yield, ec, "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/sensors", 2, interfaces);
    if (ec)
    {
        throw std::runtime_error("Failed to query ObjectMapper!");
    }
    return tree;
}

} // namespace utils
