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

constexpr std::array<const char*, 1> sensorInterfaces = {
    "xyz.openbmc_project.Sensor.Value"};

inline std::vector<SensorTree> getSubTreeSensors(
    boost::asio::yield_context& yield,
    const std::shared_ptr<sdbusplus::asio::connection>& bus)
{
    boost::system::error_code ec;

    auto tree = bus->yield_method_call<std::vector<SensorTree>>(
        yield, ec, "xyz.openbmc_project.ObjectMapper",
        "/xyz/openbmc_project/object_mapper",
        "xyz.openbmc_project.ObjectMapper", "GetSubTree",
        "/xyz/openbmc_project/sensors", 2, sensorInterfaces);
    if (ec)
    {
        throw std::runtime_error("Failed to query ObjectMapper!");
    }
    return tree;
}

inline std::vector<SensorTree> getSubTreeSensors(
    const std::shared_ptr<sdbusplus::asio::connection>& bus)
{
    auto method_call =
        bus->new_method_call("xyz.openbmc_project.ObjectMapper",
                             "/xyz/openbmc_project/object_mapper",
                             "xyz.openbmc_project.ObjectMapper", "GetSubTree");
    method_call.append("/xyz/openbmc_project/sensors/", 2, sensorInterfaces);
    auto reply = bus->call(method_call);

    auto tree = reply.unpack<std::vector<SensorTree>>();

    return tree;
}

} // namespace utils
