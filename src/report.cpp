#include "report.hpp"

#include "report_manager.hpp"
#include "utils/transform.hpp"

#include <phosphor-logging/log.hpp>
#include <sdbusplus/vtable.hpp>

#include <limits>
#include <numeric>

Report::Report(boost::asio::io_context& ioc,
               const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
               const std::string& reportName,
               const ReportingType reportingTypeIn,
               const bool emitsReadingsUpdateIn,
               const bool logToMetricReportsCollectionIn,
               const Milliseconds intervalIn, const uint64_t appendLimitIn,
               const ReportUpdates reportUpdatesIn,
               interfaces::ReportManager& reportManager,
               interfaces::JsonStorage& reportStorageIn,
               std::vector<std::shared_ptr<interfaces::Metric>> metricsIn,
               const bool enabledIn) :
    name(reportName),
    path(reportDir + name), reportingType(reportingTypeIn),
    interval(intervalIn), emitsReadingsUpdate(emitsReadingsUpdateIn),
    logToMetricReportsCollection(logToMetricReportsCollectionIn),
    sensorCount(getSensorCount(metricsIn)),
    appendLimit(deduceAppendLimit(appendLimitIn)),
    reportUpdates(reportUpdatesIn),
    readingsBuffer(deduceBufferSize(reportUpdates, reportingType)),
    objServer(objServer), metrics(std::move(metricsIn)), timer(ioc),
    fileName(std::to_string(std::hash<std::string>{}(name))),
    reportStorage(reportStorageIn), enabled(enabledIn)
{
    readingParameters =
        toReadingParameters(utils::transform(metrics, [](const auto& metric) {
            return metric->dumpConfiguration();
        }));

    readingParametersPastVersion =
        utils::transform(readingParameters, [](const auto& item) {
            return ReadingParametersPastVersion::value_type(
                std::get<0>(item).front(), std::get<1>(item), std::get<2>(item),
                std::get<3>(item));
        });

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

    persistency = storeConfiguration();
    reportIface = makeReportInterface();

    if (reportingType == ReportingType::Periodic)
    {
        scheduleTimer(interval);
    }

    if (enabled)
    {
        for (auto& metric : this->metrics)
        {
            metric->initialize();
        }
    }
}

uint64_t Report::getSensorCount(
    std::vector<std::shared_ptr<interfaces::Metric>>& metrics)
{
    uint64_t sensorCount = 0;
    for (auto& metric : metrics)
    {
        sensorCount += metric->sensorCount();
    }
    return sensorCount;
}

uint64_t Report::deduceAppendLimit(const uint64_t appendLimitIn) const
{
    if (appendLimitIn == std::numeric_limits<uint64_t>::max())
    {
        return sensorCount;
    }
    else
    {
        return appendLimitIn;
    }
}

uint64_t Report::deduceBufferSize(const ReportUpdates reportUpdatesIn,
                                  const ReportingType reportingTypeIn) const
{
    if (reportUpdatesIn == ReportUpdates::Overwrite ||
        reportingTypeIn == ReportingType::OnRequest)
    {
        return sensorCount;
    }
    else
    {
        return appendLimit;
    }
}

void Report::setReportUpdates(const ReportUpdates newReportUpdates)
{
    if (reportUpdates != newReportUpdates)
    {
        if (reportingType != ReportingType::OnRequest &&
            (reportUpdates == ReportUpdates::Overwrite ||
             newReportUpdates == ReportUpdates::Overwrite))
        {
            readingsBuffer.clearAndResize(
                deduceBufferSize(newReportUpdates, reportingType));
        }
        reportUpdates = newReportUpdates;
    }
}

std::unique_ptr<sdbusplus::asio::dbus_interface> Report::makeReportInterface()
{
    auto dbusIface = objServer->add_unique_interface(path, reportIfaceName);
    dbusIface->register_property_rw(
        "Enabled", enabled, sdbusplus::vtable::property_::emits_change,
        [this](bool newVal, const auto&) {
            if (newVal != enabled)
            {
                if (true == newVal && ReportingType::Periodic == reportingType)
                {
                    scheduleTimer(interval);
                }
                if (newVal)
                {
                    for (auto& metric : metrics)
                    {
                        metric->initialize();
                    }
                }
                else
                {
                    for (auto& metric : metrics)
                    {
                        metric->deinitialize();
                    }
                }

                enabled = newVal;
                persistency = storeConfiguration();
            }
            return true;
        },
        [this](const auto&) { return enabled; });
    dbusIface->register_property_rw(
        "Interval", interval.count(),
        sdbusplus::vtable::property_::emits_change,
        [this](uint64_t newVal, auto&) {
            if (Milliseconds newValT{newVal};
                newValT >= ReportManager::minInterval)
            {
                if (newValT != interval)
                {
                    interval = newValT;
                    persistency = storeConfiguration();
                }
                return true;
            }
            return false;
        },
        [this](const auto&) { return interval.count(); });
    dbusIface->register_property_rw(
        "Persistency", persistency, sdbusplus::vtable::property_::emits_change,
        [this](bool newVal, const auto&) {
            if (newVal == persistency)
            {
                return true;
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
            return true;
        },
        [this](const auto&) { return persistency; });

    auto readingsFlag = sdbusplus::vtable::property_::none;
    if (emitsReadingsUpdate)
    {
        readingsFlag = sdbusplus::vtable::property_::emits_change;
    }
    dbusIface->register_property_r("Readings", readings, readingsFlag,
                                   [this](const auto&) { return readings; });
    dbusIface->register_property_r(
        "ReportingType", std::string(), sdbusplus::vtable::property_::const_,
        [this](const auto&) { return reportingTypeToString(reportingType); });
    dbusIface->register_property_r(
        "ReadingParameters", readingParametersPastVersion,
        sdbusplus::vtable::property_::const_,
        [this](const auto&) { return readingParametersPastVersion; });
    dbusIface->register_property_r(
        "ReadingParametersFutureVersion", readingParameters,
        sdbusplus::vtable::property_::const_,
        [this](const auto&) { return readingParameters; });
    dbusIface->register_property_r(
        "EmitsReadingsUpdate", emitsReadingsUpdate,
        sdbusplus::vtable::property_::const_,
        [this](const auto&) { return emitsReadingsUpdate; });
    dbusIface->register_property_r(
        "LogToMetricReportsCollection", logToMetricReportsCollection,
        sdbusplus::vtable::property_::const_,
        [this](const auto&) { return logToMetricReportsCollection; });
    dbusIface->register_property_r("AppendLimit", appendLimit,
                                   sdbusplus::vtable::property_::emits_change,
                                   [this](const auto&) { return appendLimit; });
    dbusIface->register_property_rw(
        "ReportUpdates", std::string(),
        sdbusplus::vtable::property_::emits_change,
        [this](auto newVal, auto& oldVal) {
            ReportManager::verifyReportUpdates(newVal);
            setReportUpdates(stringToReportUpdates(newVal));
            oldVal = newVal;
            return true;
        },
        [this](const auto&) { return reportUpdatesToString(reportUpdates); });
    dbusIface->register_method("Update", [this] {
        if (reportingType == ReportingType::OnRequest)
        {
            updateReadings();
        }
    });
    constexpr bool skipPropertiesChangedSignal = true;
    dbusIface->initialize(skipPropertiesChangedSignal);
    return dbusIface;
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

void Report::scheduleTimer(Milliseconds timerInterval)
{
    timer.expires_after(timerInterval);
    timer.async_wait(
        [this](boost::system::error_code ec) { timerProc(ec, *this); });
}

void Report::updateReadings()
{
    if (!enabled)
    {
        return;
    }

    if (reportUpdates == ReportUpdates::Overwrite ||
        reportingType == ReportingType::OnRequest)
    {
        readingsBuffer.clear();
    }

    for (const auto& metric : metrics)
    {
        for (const auto& [id, metadata, value, timestamp] :
             metric->getReadings())
        {
            if (reportUpdates == ReportUpdates::AppendStopsWhenFull &&
                readingsBuffer.isFull())
            {
                enabled = false;
                for (auto& metric : metrics)
                {
                    metric->deinitialize();
                }
                break;
            }
            readingsBuffer.emplace(id, metadata, value, timestamp);
        }
    }

    readings = {std::time(0), std::vector<ReadingData>(readingsBuffer.begin(),
                                                       readingsBuffer.end())};

    reportIface->signal_property("Readings");
}

bool Report::storeConfiguration() const
{
    try
    {
        nlohmann::json data;

        data["Enabled"] = enabled;
        data["Version"] = reportVersion;
        data["Name"] = name;
        data["ReportingType"] = reportingTypeToString(reportingType);
        data["EmitsReadingsUpdate"] = emitsReadingsUpdate;
        data["LogToMetricReportsCollection"] = logToMetricReportsCollection;
        data["Interval"] = interval.count();
        data["AppendLimit"] = appendLimit;
        data["ReportUpdates"] = reportUpdatesToString(reportUpdates);
        data["ReadingParameters"] =
            utils::transform(metrics, [](const auto& metric) {
                return metric->dumpConfiguration();
            });

        reportStorage.store(fileName, data);
    }
    catch (const std::exception& e)
    {
        phosphor::logging::log<phosphor::logging::level::ERR>(
            "Failed to store a report in storage",
            phosphor::logging::entry("EXCEPTION_MSG=%s", e.what()));
        return false;
    }

    return true;
}
