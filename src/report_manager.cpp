#include "report_manager.hpp"

#include "utils/api_error.hpp"
#include "utils/log.hpp"

#include <system_error>

constexpr const char* reportManagerIfaceName =
    "xyz.openbmc_project.Telemetry.ReportManager";
constexpr const char* reportManagerPath =
    "/xyz/openbmc_project/Telemetry/Reports";

auto ReportManager::getRemoveReportCallback()
{
    return [this](const Report& report) {
        reports.erase(std::remove_if(reports.begin(), reports.end(),
                                     [&report](const auto& x) {
                                         return report.name == x.name;
                                     }),
                      reports.end());
    };
}

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
            const std::string& name, const std::string& domain,
            const std::string& reportingType, const bool emitsReadingsUpdate,
            const bool logToMetricReportsCollection, const uint64_t interval,
            const ReadingParameters& metricParams) {
            if (reports.size() >= maxReports)
            {
                throw sdBusError(std::errc::too_many_files_open,
                                 "Reached maximal report count");
            }

            std::string reportName = domain + '/' + name;
            for (const auto& report : reports)
            {
                if (report.name == reportName)
                {
                    throw sdBusError(std::errc::invalid_argument,
                                     "Duplicate report");
                }
            }

            std::chrono::milliseconds reportInterval{interval};
            if (reportInterval < minInterval)
            {
                throw sdBusError(std::errc::invalid_argument,
                                 "Invalid interval");
            }

            reports.emplace_back(io_context, objServer, reportName,
                                 reportingType, emitsReadingsUpdate,
                                 logToMetricReportsCollection,
                                 std::move(reportInterval), metricParams,
                                 getRemoveReportCallback());
            return reports.back().path;
        });

    reportManagerIntf->initialize();
}

ReportManager::~ReportManager()
{
    objServer->remove_interface(reportManagerIntf);
}
