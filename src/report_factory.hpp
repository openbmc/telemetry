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
        const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
        SensorCache& sensorCache);

    std::vector<LabeledMetricParameters> convertMetricParams(
        boost::asio::yield_context& yield,
        const ReadingParameters& metricParams) const override;

    std::unique_ptr<interfaces::Report>
        make(const std::string& name, const ReportingType reportingType,
             bool emitsReadingsSignal, bool logToMetricReportsCollection,
             Milliseconds period, interfaces::ReportManager& reportManager,
             interfaces::JsonStorage& reportStorage,
             std::vector<LabeledMetricParameters> labeledMetricParams,
             bool enabled) const override;

  private:
    Sensors getSensors(
        const std::vector<LabeledSensorParameters>& sensorPaths) const;

    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    SensorCache& sensorCache;
};
