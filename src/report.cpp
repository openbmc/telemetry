#include "report.hpp"

#include "report_manager.hpp"
#include "utils/api_error.hpp"
#include "utils/log.hpp"

#include <system_error>

constexpr const char* reportIfaceName = "xyz.openbmc_project.Telemetry.Report";
constexpr const char* reportPath = "/xyz/openbmc_project/Telemetry/Reports/";
constexpr const char* deleteIfaceName = "xyz.openbmc_project.Object.Delete";

Report::Report(
    const std::shared_ptr<sdbusplus::asio::connection>& bus,
    const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
    const std::string& domain, const std::string& name, const std::string& type,
    const bool emitsReadingsSignal, const bool logToMetricReportsCollection,
    const std::chrono::milliseconds&& period,
    const ReadingParameters& metricParams,
    std::function<void(const std::shared_ptr<Report>&)> removeCallback) :
    path{reportPath + domain + "/" + name},
    scanPeriod{std::move(period)}, objServer(objServer)
{
    reportIntf = objServer->add_interface(path, reportIfaceName);

    reportIntf->register_property(
        "ScanPeriod", uint64_t{},
        [this](const auto& val, const auto&) {
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

    reportIntf->register_property("Persistency", bool{false});
    reportIntf->register_property("Readings", Readings{});
    reportIntf->register_property("ReportingType", type);
    reportIntf->register_property("ReadingParameters", metricParams);
    reportIntf->register_property("EmitsReadingsUpdate", emitsReadingsSignal);
    reportIntf->register_property("LogToMetricReportsCollection",
                                  logToMetricReportsCollection);

    reportIntf->initialize();

    deleteIface = objServer->add_interface(path, deleteIfaceName);

    deleteIface->register_method(
        "Delete", [this, bus, removeCallback = std::move(removeCallback)] {
            boost::asio::post(
                bus->get_io_context(),
                [this, removeCallback = std::move(removeCallback)] {
                    removeCallback(shared_from_this());
                });
        });

    deleteIface->initialize();
}

Report::~Report()
{
    objServer->remove_interface(reportIntf);
    objServer->remove_interface(deleteIface);
}
