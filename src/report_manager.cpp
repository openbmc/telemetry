#include "report_manager.hpp"

#include <system_error>

constexpr const char* reportManagerIfaceName =
    "xyz.openbmc_project.Telemetry.ReportManager";
constexpr const char* reportManagerPath =
    "/xyz/openbmc_project/Telemetry/Reports";

ReportManager::ReportManager(
    const std::shared_ptr<sdbusplus::asio::object_server>& objServer) :
    objServer(objServer)
{
    reportManagerIntf =
        objServer->add_interface(reportManagerPath, reportManagerIfaceName);

    reportManagerIntf->register_property_r(
        "MaxReports", uint32_t{}, sdbusplus::vtable::property_::const_,
        [](const auto&) { return maxReports; });
    reportManagerIntf->register_property_r(
        "MinInterval", uint64_t{}, sdbusplus::vtable::property_::const_,
        [](const auto&) -> uint64_t { return minInterval.count(); });

    reportManagerIntf->initialize();
}

ReportManager::~ReportManager()
{
    objServer->remove_interface(reportManagerIntf);
}
