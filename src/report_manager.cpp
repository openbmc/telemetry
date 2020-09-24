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
    return [this](const std::shared_ptr<Report>& report) {
        for (auto it = reports.begin(); it != reports.end(); it++)
        {
            if (report == *it)
            {
                reports.erase(it);
                return;
            }
        }
    };
}

ReportManager::ReportManager(
    const std::shared_ptr<sdbusplus::asio::connection>& bus,
    const std::shared_ptr<sdbusplus::asio::object_server>& objServer) :
    objServer(objServer)
{
    reportManagerIntf =
        objServer->add_interface(reportManagerPath, reportManagerIfaceName);

    reportManagerIntf->register_property_r(
        "MaxReports", uint32_t{}, sdbusplus::vtable::property_::const_,
        [](const auto&) { return maxReports; });
    reportManagerIntf->register_property_r(
        "PollRateResolution", uint64_t{}, sdbusplus::vtable::property_::const_,
        [](const auto&) -> uint64_t { return pollRateResolution.count(); });

    reportManagerIntf->register_method(
        "AddReport",
        [this, &bus, &objServer](
            const std::string& name, const std::string& domain,
            const std::string& reportingType, const bool emitsReadingsUpdate,
            const bool logToMetricReportsCollection, const uint64_t scanPeriod,
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
                bus, objServer, domain, name, reportingType,
                emitsReadingsUpdate, logToMetricReportsCollection,
                std::chrono::milliseconds{scanPeriod}, metricParams,
                getRemoveReportCallback());
            reports.push_back(report);

            return report->path;
        });

    reportManagerIntf->initialize();
}

ReportManager::~ReportManager()
{
    objServer->remove_interface(reportManagerIntf);
}

bool ReportManager::verifyScanPeriod(const uint64_t scanPeriod)
{
    if (scanPeriod < pollRateResolution.count() ||
        scanPeriod % pollRateResolution.count() != 0)
    {
        return false;
    }
    return true;
}
