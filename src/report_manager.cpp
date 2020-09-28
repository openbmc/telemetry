#include "report_manager.hpp"

#include <sdbusplus/exception.hpp>

#include <system_error>

constexpr const char* reportManagerIfaceName =
    "xyz.openbmc_project.Telemetry.ReportManager";
constexpr const char* reportManagerPath =
    "/xyz/openbmc_project/Telemetry/Reports";

ReportManager::ReportManager(
    const std::shared_ptr<sdbusplus::asio::connection>& bus,
    const std::shared_ptr<sdbusplus::asio::object_server>& objServerIn) :
    objServer(objServerIn)
{
    reports.reserve(maxReports);

    reportManagerIntf =
        objServer->add_interface(reportManagerPath, reportManagerIfaceName);

    reportManagerIntf->register_property_r(
        "MaxReports", uint32_t{}, sdbusplus::vtable::property_::const_,
        [](const auto&) { return maxReports; });
    reportManagerIntf->register_property_r(
        "MinInterval", uint64_t{}, sdbusplus::vtable::property_::const_,
        [](const auto&) -> uint64_t { return minInterval.count(); });

    reportManagerIntf->register_method(
        "AddReport",
        [this, &io_context = bus->get_io_context()](
            const std::string& reportName, const std::string& reportingType,
            const bool emitsReadingsUpdate,
            const bool logToMetricReportsCollection, const uint64_t interval,
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
                io_context, objServer, reportName, reportingType,
                emitsReadingsUpdate, logToMetricReportsCollection,
                std::move(reportInterval), metricParams, *this));
            return reports.back()->path;
        });

    reportManagerIntf->initialize();
}

ReportManager::~ReportManager()
{
    objServer->remove_interface(reportManagerIntf);
}

void ReportManager::removeReport(const Report* report)
{
    reports.erase(
        std::remove_if(reports.begin(), reports.end(),
                       [report](const auto& x) { return report == x.get(); }),
        reports.end());
}
