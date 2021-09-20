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
        make(const std::string& name, const std::string& reportingType,
             bool emitsReadingsSignal, bool logToMetricReportsCollection,
             Milliseconds period, uint64_t appendLimitIn,
             const std::string& reportUpdatesIn,
             interfaces::ReportManager& reportManager,
             interfaces::JsonStorage& reportStorage,
             std::vector<LabeledMetricParameters> labeledMetricParams)
            const override;

  private:
    Sensors getSensors(
        const std::vector<LabeledSensorParameters>& sensorPaths) const;

    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    SensorCache& sensorCache;
};
