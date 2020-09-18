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
            interface->register_property(
                "MaxReports", maxReports,
                sdbusplus::asio::PropertyPermission::readOnly);
            interface->register_property(
                "PollRateResolution", pollRateResolutionMs,
                sdbusplus::asio::PropertyPermission::readOnly);
        });
}
