#pragma once

#include "interfaces/report_factory.hpp"
#include "interfaces/sensor.hpp"
#include "sensor_cache.hpp"

#include <boost/asio/io_context.hpp>
#include <sdbusplus/asio/object_server.hpp>

class ReportFactory : public interfaces::ReportFactory
{
  public:
    ReportFactory(
        std::shared_ptr<sdbusplus::asio::connection> bus,
        const std::shared_ptr<sdbusplus::asio::object_server>& objServer);

    std::unique_ptr<interfaces::Report> make(
        std::optional<std::reference_wrapper<boost::asio::yield_context>> yield,
        const std::string& name, const std::string& reportingType,
        bool emitsReadingsSignal, bool logToMetricReportsCollection,
        std::chrono::milliseconds period, const ReadingParameters& metricParams,
        interfaces::ReportManager& reportManager,
        interfaces::JsonStorage& reportStorage) const override;

  private:
    using SensorPath = std::string;
    using ServiceName = std::string;
    using Ifaces = std::vector<std::string>;
    using SensorIfaces = std::vector<std::pair<ServiceName, Ifaces>>;
    using SensorTree = std::pair<SensorPath, SensorIfaces>;

    std::vector<std::shared_ptr<interfaces::Sensor>> getSensors(
        const std::optional<std::vector<SensorTree>>& tree,
        const std::vector<sdbusplus::message::object_path>& sensorPaths) const;
    std::vector<SensorTree>
        getSensorTree(boost::asio::yield_context& yield) const;

    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    std::unique_ptr<SensorCache> sensorCache;
};
