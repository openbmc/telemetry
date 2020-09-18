#include "report_manager.hpp"

constexpr const char* reportManagerIfaceName =
    "xyz.openbmc_project.MonitoringService.ReportManager";
constexpr const char* reportManagerPath =
    "/xyz/openbmc_project/MonitoringService";

ReportManager::ReportManager(
    const std::shared_ptr<sdbusplus::asio::object_server>& objServer)
{
    reportManagerIntf = utils::DbusInterface(
        objServer, reportManagerPath, reportManagerIfaceName,
        [](const std::shared_ptr<sdbusplus::asio::dbus_interface>& interface) {
            interface->register_property_r(
                "MaxReports", size_t{}, sdbusplus::vtable::property_::const_,
                [](const auto&) { return maxReports; });
            interface->register_property_r(
                "PollRateResolution", uint64_t{},
                sdbusplus::vtable::property_::const_,
                [](const auto&) -> uint64_t {
                    return pollRateResolution.count();
                });
        });
}
