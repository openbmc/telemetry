#include "report.hpp"

#include "report_manager.hpp"
#include "utils/api_error.hpp"
#include "utils/log.hpp"

#include <system_error>

constexpr const char* reportIfaceName =
    "xyz.openbmc_project.MonitoringService.Report";
constexpr const char* reportPath =
    "/xyz/openbmc_project/MonitoringService/Reports/";

Report::Report(const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
               const std::string& domain, const std::string& name,
               const std::chrono::milliseconds&& period) :
    path{reportPath + domain + "/" + name},
    scanPeriod{std::move(period)}
{
    reportIntf = utils::DbusInterface(
        objServer, path, reportIfaceName,
        [this](
            const std::shared_ptr<sdbusplus::asio::dbus_interface>& interface) {
            interface->register_property(
                "ScanPeriod", uint64_t{},
                [this](const auto& val, const auto&) mutable {
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

            interface->register_property("Persistency", std::string{});
            interface->register_property("Readings", Readings{});
            interface->register_property("Timestamp", uint64_t{});
            interface->register_property("ReportingType", std::string{});
            interface->register_property_r(
                "ReportActions", std::vector<std::string>{},
                sdbusplus::vtable::property_::const_,
                [](const auto&) { return std::vector<std::string>{}; });
            interface->register_property("ReadingParameters",
                                         ReadingParameters{});

            interface->register_method("Update", [] {
                throw sdBusError(std::errc::not_supported, "Not implemented");
            });

            interface->register_signal<void>("ReportUpdate");
        });
}
