#include "report.hpp"

#include "report_manager.hpp"
#include "utils/transform.hpp"

#include <phosphor-logging/log.hpp>

#include <numeric>

Report::Report(boost::asio::io_context& ioc,
               const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
               const std::string& reportName,
               const std::string& reportingTypeIn,
               const bool emitsReadingsUpdateIn,
               const bool logToMetricReportsCollectionIn,
               const std::chrono::milliseconds intervalIn,
               const ReadingParameters& readingParametersIn,
               interfaces::ReportManager& reportManager,
               std::vector<std::shared_ptr<interfaces::Metric>> metrics) :
    name(reportName),
    path(reportDir + name), reportingType(reportingTypeIn),
    interval(intervalIn), emitsReadingsUpdate(emitsReadingsUpdateIn),
    logToMetricReportsCollection(logToMetricReportsCollectionIn),
    readingParameters(readingParametersIn), objServer(objServer),
    metrics(std::move(metrics)), timer(ioc),
    fileName(std::to_string(std::hash<std::string>{}(name))),
    reportStorage(reportManager.getReportStorage())
{
    deleteIface = objServer->add_unique_interface(
        path, deleteIfaceName, [this, &ioc, &reportManager](auto& dbusIface) {
            dbusIface.register_method("Delete", [this, &ioc, &reportManager] {
                if (persistency)
                {
                    reportStorage.remove(fileName);
                }
                boost::asio::post(ioc, [this, &reportManager] {
                    reportManager.removeReport(this);
                });
            });
        });

    reportIface = objServer->add_unique_interface(
        path, reportIfaceName, [this](auto& dbusIface) {
            dbusIface.register_property(
                "Interval", static_cast<uint64_t>(interval.count()),
                [this](const uint64_t newVal, uint64_t& actualVal) {
                    std::chrono::milliseconds newValT(newVal);
                    if (newValT < ReportManager::minInterval)
                    {
                        return false;
                    }
                    actualVal = newVal;
                    interval = newValT;
                    return true;
                });
            persistency = storeConfiguration();
            dbusIface.register_property(
                "Persistency", persistency,
                [this](const bool newVal, bool& actualVal) {
                    if (newVal == actualVal)
                    {
                        return false;
                    }
                    if (newVal)
                    {
                        persistency = storeConfiguration();
                    }
                    else
                    {
                        reportStorage.remove(fileName);
                        persistency = false;
                    }
                    actualVal = persistency;
                    return true;
                });
            dbusIface.register_property_r(
                "Readings", readings,
                sdbusplus::vtable::property_::emits_change,
                [this](const auto&) { return readings; });
            dbusIface.register_property_r(
                "ReportingType", reportingType,
                sdbusplus::vtable::property_::const_,
                [this](const auto&) { return reportingType; });
            dbusIface.register_property_r(
                "ReadingParameters", readingParameters,
                sdbusplus::vtable::property_::const_,
                [this](const auto&) { return readingParameters; });
            dbusIface.register_property("EmitsReadingsUpdate",
                                        emitsReadingsUpdate);
            dbusIface.register_property("LogToMetricReportsCollection",
                                        logToMetricReportsCollection);
        });

    if (reportingType == "Periodic")
    {
        scheduleTimer(interval);
    }
}

void Report::timerProc(boost::system::error_code ec, Report& self)
{
    if (ec)
    {
        return;
    }

    self.updateReadings();
    self.scheduleTimer(self.interval);
}

void Report::scheduleTimer(std::chrono::milliseconds timerInterval)
{
    timer.expires_from_now(
        boost::posix_time::milliseconds(timerInterval.count()));
    timer.async_wait(
        [this](boost::system::error_code ec) { timerProc(ec, *this); });
}

void Report::updateReadings()
{
    auto numElements = std::accumulate(
        metrics.begin(), metrics.end(), 0u, [](auto sum, const auto& metric) {
            return sum + metric->getReadings().size();
        });

    readingsCache.resize(numElements);

    auto it = readingsCache.begin();

    for (const auto& metric : metrics)
    {
        for (const auto& reading : metric->getReadings())
        {
            *(it++) = std::make_tuple(reading.a, reading.b, reading.value,
                                      reading.timestamp);
        }
    }

    std::get<0>(readings) = std::time(0);
    std::get<1>(readings) = readingsCache;
    reportIface->signal_property("Readings");
}

bool Report::storeConfiguration() const
{
    try
    {
        nlohmann::json data;

        data["Version"] = reportVersion;
        data["Name"] = name;
        data["ReportingType"] = reportingType;
        data["EmitsReadingsUpdate"] = emitsReadingsUpdate;
        data["LogToMetricReportsCollection"] = logToMetricReportsCollection;
        data["Interval"] = interval.count();
        data["ReadingParameters"] =
            utils::transform(readingParameters, [](const auto& item) {
                return ReadingParameterJson(&item);
            });

        reportStorage.store(fileName, data);
    }
    catch (std::exception& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Failed to store a report in storage",
            phosphor::logging::entry("msg=", e.what()));
        return false;
    }

    return true;
}
