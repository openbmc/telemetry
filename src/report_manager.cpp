#include "report_manager.hpp"

const char* reportManagerIfaceName =
    "xyz.openbmc_project.MonitoringService.ReportManager";
const char* reportManagerPath = "/xyz/openbmc_project/MonitoringService";

ReportManager::ReportManager(
    std::shared_ptr<sdbusplus::asio::object_server> objServer)
{
    reportManagerIntf = utils::DbusInterface(
        objServer, reportManagerPath, reportManagerIfaceName,
        [](const std::shared_ptr<sdbusplus::asio::dbus_interface>& interface) {
            interface->register_property_r(
                "MaxReports", maxReports, sdbusplus::vtable::property_::const_,
                [](const auto&) { return maxReports; });
            interface->register_property_r(
                "PollRateResolution", pollRateResolutionMs,
                sdbusplus::vtable::property_::const_,
                [](const auto&) { return pollRateResolutionMs; });
        });
}
