#include "report.hpp"

#include "report_manager.hpp"
#include "utils/transform.hpp"

#include <phosphor-logging/log.hpp>
#include <sdbusplus/vtable.hpp>

#include <numeric>

Report::Report(boost::asio::io_context& ioc,
               const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
               const std::string& reportName,
               const std::string& reportingTypeIn,
               const bool emitsReadingsUpdateIn,
               const bool logToMetricReportsCollectionIn,
               const Milliseconds intervalIn,
               interfaces::ReportManager& reportManager,
               interfaces::JsonStorage& reportStorageIn,
               std::vector<std::shared_ptr<interfaces::Metric>> metricsIn) :
    name(reportName),
    path(reportDir + name), reportingType(reportingTypeIn),
    interval(intervalIn), emitsReadingsUpdate(emitsReadingsUpdateIn),
    logToMetricReportsCollection(logToMetricReportsCollectionIn),
    objServer(objServer), metrics(std::move(metricsIn)), timer(ioc),
    fileName(std::to_string(std::hash<std::string>{}(name))),
    reportStorage(reportStorageIn)
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

    if (reportingType == "Periodic")
    {
        scheduleTimer(interval);
    }

    for (auto& metric : this->metrics)
    {
        metric->initialize();
    }
}

std::unique_ptr<sdbusplus::asio::dbus_interface> Report::makeReportInterface()
{
    auto dbusIface = objServer->add_unique_interface(path, reportIfaceName);
    dbusIface->register_property_rw(
        "Interval", interval.count(),
        sdbusplus::vtable::property_::emits_change,
        [this](uint64_t newVal, auto&) {
            Milliseconds newValT(newVal);
            if (newValT < ReportManager::minInterval)
            {
                return false;
            }
            interval = newValT;
            return true;
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
        "ReportingType", reportingType, sdbusplus::vtable::property_::const_,
        [this](const auto&) { return reportingType; });
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
    dbusIface->register_method("Update", [this] {
        if (reportingType == "OnRequest")
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
    using ReadingsValue = std::tuple_element_t<1, Readings>;
    std::get<ReadingsValue>(cachedReadings).clear();

    for (const auto& metric : metrics)
    {
        for (const auto& reading : metric->getReadings())
        {
            std::get<1>(cachedReadings)
                .emplace_back(reading.id, reading.metadata, reading.value,
                              reading.timestamp);
        }
    }

    using ReadingsTimestamp = std::tuple_element_t<0, Readings>;
    std::get<ReadingsTimestamp>(cachedReadings) = std::time(0);

    std::swap(readings, cachedReadings);

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
