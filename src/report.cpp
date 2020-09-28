#include "report.hpp"

#include "report_manager.hpp"
#include "utils/log.hpp"

#include <system_error>

constexpr const char* reportIfaceName = "xyz.openbmc_project.Telemetry.Report";
constexpr const char* reportPath = "/xyz/openbmc_project/Telemetry/Reports/";
constexpr const char* deleteIfaceName = "xyz.openbmc_project.Object.Delete";

Report::Report(
    const std::shared_ptr<sdbusplus::asio::connection>& bus,
    const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
    const std::string& domain, const std::string& name, const std::string& type,
    const bool emitsReadingsSignal, const bool logToMetricReportsCollection,
    const std::chrono::milliseconds&& period,
    const ReadingParameters& metricParams,
    std::function<void(const std::shared_ptr<Report>&)> removeCallback) :
    path{reportPath + domain + "/" + name},
    scanPeriod{std::move(period)}
{
    reportIntf = utils::DbusInterface(
        objServer, path, reportIfaceName,
        [this, &type, &emitsReadingsSignal, &logToMetricReportsCollection,
         &metricParams](
            const std::shared_ptr<sdbusplus::asio::dbus_interface>& interface) {
            interface->register_property(
                "ScanPeriod", uint64_t{},
                [this](const auto& val, const auto&) {
                    if (!ReportManager::verifyScanPeriod(val))
                    {
                        return false;
                    }
                    scanPeriod = std::chrono::milliseconds{val};
                    return true;
                },
                [this](const auto&) {
                    return static_cast<uint64_t>(scanPeriod.count());
                });

            interface->register_property("Persistency", bool{false});
            interface->register_property("Readings", Readings{});
            interface->register_property("ReportingType", type);
            interface->register_property("ReadingParameters", metricParams);
            interface->register_property("EmitsReadingsUpdate",
                                         emitsReadingsSignal);
            interface->register_property("LogToMetricReportsCollection",
                                         logToMetricReportsCollection);
        });

    deleteIface = utils::DbusInterface(
        objServer, path, deleteIfaceName,
        [this, bus, removeCallback = std::move(removeCallback)](
            const std::shared_ptr<sdbusplus::asio::dbus_interface>& interface) {
            interface->register_method(
                "Delete",
                [this, bus, removeCallback = std::move(removeCallback)] {
                    boost::asio::post(
                        bus->get_io_context(),
                        [this, removeCallback = std::move(removeCallback)] {
                            removeCallback(shared_from_this());
                        });
                });
        });
}
