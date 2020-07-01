#pragma once

#include "core/interfaces/storage.hpp"
#include "core/report.hpp"
#include "dbus/dbus.hpp"
#include "dbus/interfaces/report_factory.hpp"
#include "dbus/report.hpp"
#include "dbus/sensor.hpp"

#include <sdbusplus/asio/object_server.hpp>

namespace dbus
{

template <class Strategy = core::ReportSignalStrategy>
class ReportFactory : public interfaces::ReportFactory
{
  public:
    ReportFactory(boost::asio::io_context& ioc,
                  std::chrono::milliseconds pollRateResolution,
                  const std::shared_ptr<core::interfaces::Storage>& storage) :
        ioc_(ioc),
        pollRateResolution_(pollRateResolution), storage_(storage)
    {}

    std::shared_ptr<core::interfaces::Report> makeCoreReport(
        const core::ReportName& name,
        std::vector<std::shared_ptr<core::interfaces::Metric>>&& metrics,
        core::ReportingType reportingType,
        std::vector<core::ReportAction>&& reportAction,
        std::chrono::milliseconds scanPeriod) const
    {
        validateScanPeriod(reportingType, scanPeriod);

        auto report = std::make_shared<core::Report<Strategy>>(
            ioc_, name, std::move(metrics), reportingType,
            std::move(reportAction), scanPeriod);

        report->schedule();

        return report;
    }

    std::shared_ptr<core::interfaces::Report> makeCoreReport(
        boost::asio::yield_context& yield,
        std::shared_ptr<sdbusplus::asio::connection> bus,
        const core::ReportName& name, const std::string& reportingType,
        const std::vector<std::string>& reportAction, const uint32_t scanPeriod,
        const std::vector<MetricParams>& metricParams) const
    {
        auto reportingType_ = dbus::getReportingType(reportingType);
        auto reportAction_ = getReportAction(reportAction);
        auto metrics = getMetrics(yield, bus, metricParams);

        return makeCoreReport(name, std::move(metrics), reportingType_,
                              std::move(reportAction_),
                              std::chrono::milliseconds(scanPeriod));
    }

    std::tuple<std::shared_ptr<core::interfaces::Report>,
               std::shared_ptr<dbus::Report>>
        make(boost::asio::yield_context& yield,
             std::shared_ptr<sdbusplus::asio::connection> bus,
             const core::ReportName& name, const std::string& reportingType,
             const std::vector<std::string>& reportAction,
             const uint32_t scanPeriod,
             const std::vector<MetricParams>& metricParams,
             std::shared_ptr<sdbusplus::asio::object_server> objServer)
            const override
    {
        auto coreReport =
            makeCoreReport(yield, bus, name, reportingType, reportAction,
                           scanPeriod, metricParams);

        return std::make_tuple(coreReport,
                               std::make_shared<dbus::Report>(
                                   bus, objServer, coreReport, name, storage_));
    }

  private:
    boost::asio::io_context& ioc_;
    std::chrono::milliseconds pollRateResolution_;
    std::shared_ptr<core::interfaces::Storage> storage_;

    void validateScanPeriod(core::ReportingType reportingType,
                            std::chrono::milliseconds scanPeriod) const
    {
        using namespace std::chrono_literals;

        if (reportingType == core::ReportingType::periodic)
        {
            if (scanPeriod == 0ms || scanPeriod % pollRateResolution_ != 0ms)
            {
                utils::throwError(std::errc::invalid_argument,
                                  "Report creation failed: Invalid scanPeriod");
            }
        }
        else
        {
            if (scanPeriod != 0ms)
            {
                utils::throwError(std::errc::invalid_argument,
                                  "Report creation failed: only 0ms scanPeriod "
                                  "is allowed for not periodic reporting type");
            }
        }
    }

    static std::vector<std::shared_ptr<core::interfaces::Metric>>
        getMetrics(boost::asio::yield_context& yield,
                   std::shared_ptr<sdbusplus::asio::connection> bus,
                   const std::vector<MetricParams>& metricParams)
    {
        std::vector<std::shared_ptr<core::interfaces::Metric>> metrics;
        metrics.reserve(metricParams.size());

        for (auto& [sensorPaths, operationType, id, metadata] : metricParams)
        {
            auto sensors = getSensors(yield, bus, sensorPaths);

            metrics.emplace_back(std::make_shared<core::Metric>(
                id, metadata, std::move(sensors),
                getOperationType(operationType)));
        }

        return metrics;
    }

    static std::vector<std::shared_ptr<core::interfaces::Sensor>>
        getSensors(boost::asio::yield_context& yield,
                   std::shared_ptr<sdbusplus::asio::connection> bus,
                   const api::SensorPaths& sensorPaths)
    {
        auto sensorIds = getSensorIds(yield, bus, sensorPaths);

        std::vector<std::shared_ptr<core::interfaces::Sensor>> sensors;
        sensors.reserve(sensorIds.size());

        for (const auto& id : sensorIds)
        {
            sensors.push_back(core::SensorCache::make<dbus::Sensor>(id, bus));
        }

        return sensors;
    }

    static std::vector<std::shared_ptr<const dbus::Sensor::Id>>
        getSensorIds(boost::asio::yield_context& yield,
                     std::shared_ptr<sdbusplus::asio::connection> bus,
                     const api::SensorPaths& sensorPaths)
    {
        using SensorPath = std::string;
        using ServiceName = std::string;
        using Ifaces = std::vector<std::string>;
        using SensorIfaces = std::vector<std::pair<ServiceName, Ifaces>>;
        using ServiceIface = std::pair<SensorPath, SensorIfaces>;
        using ServiceIfaces = std::vector<ServiceIface>;

        std::vector<std::shared_ptr<const dbus::Sensor::Id>> ids;
        ids.reserve(sensorPaths.size());

        std::array<const char*, 1> interfaces = {
            "xyz.openbmc_project.Sensor.Value"};
        boost::system::error_code ec;

        ServiceIfaces tree = bus->yield_method_call<ServiceIfaces>(
            yield, ec, "xyz.openbmc_project.ObjectMapper",
            "/xyz/openbmc_project/object_mapper",
            "xyz.openbmc_project.ObjectMapper", "GetSubTree",
            "/xyz/openbmc_project/sensors", 2, interfaces);
        if (ec)
        {
            utils::throwError(std::errc::io_error,
                              "Unable to retrieve sensor metadata");
        }

        for (const auto& [sensor, ifacesMap] : tree)
        {
            for (const auto& [service, ifaces] : ifacesMap)
            {
                auto it =
                    std::find(sensorPaths.begin(), sensorPaths.end(), sensor);
                if (it != sensorPaths.end())
                {
                    ids.push_back(std::make_shared<const dbus::Sensor::Id>(
                        service, sensor));
                }
            }
        }

        if (ids.size() != sensorPaths.size())
        {
            utils::throwError(std::errc::no_such_file_or_directory,
                              "Unable to get info about all sensor paths. "
                              "Wrong paths provided?");
        }

        return ids;
    }
};

} // namespace dbus
