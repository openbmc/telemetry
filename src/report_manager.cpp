#include "report_manager.hpp"

#include "utils/api_error.hpp"
#include "utils/log.hpp"

#include <system_error>

constexpr const char* reportManagerIfaceName =
    "xyz.openbmc_project.Telemetry.ReportManager";
constexpr const char* reportManagerPath =
    "/xyz/openbmc_project/Telemetry/Reports";

ReportManager::ReportManager(
    const std::shared_ptr<sdbusplus::asio::object_server>& objServer) :
    reports{}
{
    reportManagerIntf = utils::DbusInterface(
        objServer, reportManagerPath, reportManagerIfaceName,
        [this, &objServer](
            const std::shared_ptr<sdbusplus::asio::dbus_interface>& interface) {
            interface->register_property_r(
                "MaxReports", uint32_t{}, sdbusplus::vtable::property_::const_,
                [](const auto&) { return maxReports; });
            interface->register_property_r(
                "PollRateResolution", uint64_t{},
                sdbusplus::vtable::property_::const_,
                [](const auto&) -> uint64_t {
                    return pollRateResolution.count();
                });

            interface->register_method(
                "AddReport",
                [this, &objServer](const std::string& name,
                                   const std::string& domain,
                                   const std::string& reportingType,
                                   const std::vector<std::string>& reportAction,
                                   const uint64_t scanPeriod,
                                   const ReadingParameters& metricParams) {
                    if (!verifyScanPeriod(scanPeriod))
                    {
                        throw sdBusError(std::errc::invalid_argument,
                                         "Invalid scanPeriod");
                    }
                    if (reports.size() >= maxReports)
                    {
                        throw sdBusError(std::errc::too_many_files_open,
                                         "Reached maximal report count");
                    }

                    auto report = std::make_shared<Report>(
                        objServer, domain, name,
                        std::chrono::milliseconds{scanPeriod});
                    reports.push_back(report);
                    return report->path;
                });
        });
}

bool ReportManager::verifyScanPeriod(const uint64_t scanPeriod)
{
    if (scanPeriod < pollRateResolution.count() ||
        scanPeriod % pollRateResolution.count() == 0)
    {
        return false;
    }
    return true;
}
