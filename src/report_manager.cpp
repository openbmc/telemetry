#include "report_manager.hpp"

#include "telemetry/names.hpp"
#include "telemetry/types.hpp"

#include <sdbusplus/exception.hpp>

#include <system_error>

ReportManager::ReportManager(
    const std::shared_ptr<sdbusplus::asio::connection>& bus,
    const std::shared_ptr<sdbusplus::asio::object_server>& objServerIn) :
    objServer(objServerIn)
{
    reports.reserve(maxReports);

    reportManagerIface = objServer->add_unique_interface(
        reportManagerPath, reportManagerIfaceName,
        [this, &ioc = bus->get_io_context()](auto& dbusIface) {
            dbusIface.register_property_r(
                "MaxReports", uint32_t{}, sdbusplus::vtable::property_::const_,
                [](const auto&) { return maxReports; });
            dbusIface.register_property_r(
                "MinInterval", uint64_t{}, sdbusplus::vtable::property_::const_,
                [](const auto&) -> uint64_t { return minInterval.count(); });

            dbusIface.register_method(
                "AddReport",
                [this, &ioc](const std::string& reportName,
                             const std::string& reportingType,
                             const bool emitsReadingsUpdate,
                             const bool logToMetricReportsCollection,
                             const uint64_t interval,
                             const ReadingParameters& metricParams) {
                    if (reports.size() >= maxReports)
                    {
                        throw sdbusplus::exception::SdBusError(
                            static_cast<int>(std::errc::too_many_files_open),
                            "Reached maximal report count");
                    }

                    for (const auto& report : reports)
                    {
                        if (report->name == reportName)
                        {
                            throw sdbusplus::exception::SdBusError(
                                static_cast<int>(std::errc::file_exists),
                                "Duplicate report");
                        }
                    }

                    std::chrono::milliseconds reportInterval{interval};
                    if (reportInterval < minInterval)
                    {
                        throw sdbusplus::exception::SdBusError(
                            static_cast<int>(std::errc::invalid_argument),
                            "Invalid interval");
                    }

                    reports.emplace_back(std::make_unique<Report>(
                        ioc, objServer, reportName, reportingType,
                        emitsReadingsUpdate, logToMetricReportsCollection,
                        std::move(reportInterval), metricParams, *this));
                    return reports.back()->path;
                });
        });
}

void ReportManager::removeReport(const Report* report)
{
    reports.erase(
        std::remove_if(reports.begin(), reports.end(),
                       [report](const auto& x) { return report == x.get(); }),
        reports.end());
}
