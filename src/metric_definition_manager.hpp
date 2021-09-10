#pragma once

#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <memory>

class MetricDefinitionManager
{
  public:
    MetricDefinitionManager(
        const std::shared_ptr<sdbusplus::asio::connection>&,
        const std::shared_ptr<sdbusplus::asio::object_server>&);
    MetricDefinitionManager(const MetricDefinitionManager&) = delete;
    MetricDefinitionManager(MetricDefinitionManager&&) = delete;

    ~MetricDefinitionManager();

    MetricDefinitionManager& operator=(const MetricDefinitionManager&) = delete;
    MetricDefinitionManager& operator=(MetricDefinitionManager&&) = delete;

    static constexpr char metricDefinitionManagerPath[] =
        "/xyz/openbmc_project/Telemetry/MetricDefinitions";
    static constexpr char metricManagerIfaceName[] =
        "xyz.openbmc_project.Telemetry.MetricDefinitionManager";

  private:
    class MetricDefinitionCollection;

    std::shared_ptr<sdbusplus::asio::connection> connection;
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    std::unique_ptr<MetricDefinitionCollection> metricDefinitions;
    std::unique_ptr<sdbusplus::asio::dbus_interface> metricManagerInterface;
};
