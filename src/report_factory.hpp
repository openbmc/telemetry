#pragma once

#include "interfaces/report_factory.hpp"
#include "interfaces/sensor.hpp"
#include "sensor_cache.hpp"
#include "types/sensor_types.hpp"
#include "utils/dbus_mapper.hpp"

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

    std::vector<LabeledMetricParameters> convertMetricParams(
        const ReadingParameters& metricParams) const override;

    void
        updateMetrics(std::vector<std::shared_ptr<interfaces::Metric>>& metrics,
                      bool enabled,
                      const ReadingParameters& metricParams) const override;

    std::unique_ptr<interfaces::Report> make(
        const std::string& reportId, const std::string& name,
        const ReportingType reportingType,
        const std::vector<ReportAction>& reportActions, Milliseconds period,
        uint64_t appendLimitIn, const ReportUpdates reportUpdatesIn,
        interfaces::ReportManager& reportManager,
        interfaces::JsonStorage& reportStorage,
        std::vector<LabeledMetricParameters> labeledMetricParams, bool enabled,
        const std::vector<std::string>& triggerIds) const override;

  private:
    Sensors getSensors(const std::vector<LabeledSensorInfo>& sensorPaths) const;
    std::vector<LabeledMetricParameters> getMetricParamsFromSensorTree(
        const ReadingParameters& metricParams,
        const std::vector<utils::SensorTree>& tree) const;

    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    SensorCache& sensorCache;
};
