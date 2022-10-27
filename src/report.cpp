#include "report.hpp"

#include "errors.hpp"
#include "messages/collect_trigger_id.hpp"
#include "messages/trigger_presence_changed_ind.hpp"
#include "messages/update_report_ind.hpp"
#include "report_manager.hpp"
#include "utils/clock.hpp"
#include "utils/contains.hpp"
#include "utils/dbus_path_utils.hpp"
#include "utils/ensure.hpp"
#include "utils/transform.hpp"

#include <phosphor-logging/log.hpp>
#include <sdbusplus/vtable.hpp>

#include <limits>
#include <numeric>
#include <optional>

Report::Report(boost::asio::io_context& ioc,
               const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
               const std::string& reportId, const std::string& reportName,
               const ReportingType reportingTypeIn,
               std::vector<ReportAction> reportActionsIn,
               const Milliseconds intervalIn, const uint64_t appendLimitIn,
               const ReportUpdates reportUpdatesIn,
               interfaces::ReportManager& reportManager,
               interfaces::JsonStorage& reportStorageIn,
               std::vector<std::shared_ptr<interfaces::Metric>> metricsIn,
               const interfaces::ReportFactory& reportFactory,
               const bool enabledIn, std::unique_ptr<interfaces::Clock> clock,
               Readings readingsIn) :
    id(reportId),
    path(utils::pathAppend(utils::constants::reportDirPath, id)),
    name(reportName), reportingType(reportingTypeIn), interval(intervalIn),
    reportActions(reportActionsIn.begin(), reportActionsIn.end()),
    metricCount(getMetricCount(metricsIn)), appendLimit(appendLimitIn),
    reportUpdates(reportUpdatesIn), readings(std::move(readingsIn)),
    readingsBuffer(std::get<1>(readings),
                   deduceBufferSize(reportUpdates, reportingType)),
    objServer(objServer), metrics(std::move(metricsIn)), timer(ioc),
    triggerIds(collectTriggerIds(ioc)), reportStorage(reportStorageIn),
    clock(std::move(clock)), messanger(ioc)
{
    readingParameters =
        toReadingParameters(utils::transform(metrics, [](const auto& metric) {
            return metric->dumpConfiguration();
        }));

    reportActions.insert(ReportAction::logToMetricReportsCollection);

    deleteIface = objServer->add_unique_interface(
        getPath(), deleteIfaceName,
        [this, &ioc, &reportManager](auto& dbusIface) {
            dbusIface.register_method("Delete", [this, &ioc, &reportManager] {
                if (persistency)
                {
                    persistency = false;

                    reportIface->signal_property("Persistency");
                }

                boost::asio::post(ioc, [this, &reportManager] {
                    reportManager.removeReport(this);
                });
            });
        });

    auto errorMessages = verify(reportingType, interval);
    state.set<ReportFlags::enabled, ReportFlags::valid>(enabledIn,
                                                        errorMessages.empty());

    reportIface = makeReportInterface(reportFactory);
    persistency = storeConfiguration();

    messanger.on_receive<messages::TriggerPresenceChangedInd>(
        [this](const auto& msg) {
            const auto oldSize = triggerIds.size();

            if (msg.presence == messages::Presence::Exist)
            {
                if (utils::contains(msg.reportIds, id))
                {
                    triggerIds.insert(msg.triggerId);
                }
                else if (!utils::contains(msg.reportIds, id))
                {
                    triggerIds.erase(msg.triggerId);
                }
            }
            else if (msg.presence == messages::Presence::Removed)
            {
                triggerIds.erase(msg.triggerId);
            }

            if (triggerIds.size() != oldSize)
            {
                reportIface->signal_property("Triggers");
            }
        });

    messanger.on_receive<messages::UpdateReportInd>([this](const auto& msg) {
        if (utils::contains(msg.reportIds, id))
        {
            updateReadings();
        }
    });
}

Report::~Report()
{
    if (persistency)
    {
        if (shouldStoreMetricValues())
        {
            storeConfiguration();
        }
    }
    else
    {
        reportStorage.remove(reportFileName());
    }
}

void Report::activate()
{
    for (auto& metric : metrics)
    {
        metric->initialize();
    }

    scheduleTimer();
}

void Report::deactivate()
{
    for (auto& metric : metrics)
    {
        metric->deinitialize();
    }

    unregisterFromMetrics = nullptr;
    timer.cancel();
}

uint64_t Report::getMetricCount(
    const std::vector<std::shared_ptr<interfaces::Metric>>& metrics)
{
    uint64_t metricCount = 0;
    for (auto& metric : metrics)
    {
        metricCount += metric->metricCount();
    }
    return metricCount;
}

uint64_t Report::deduceBufferSize(const ReportUpdates reportUpdatesIn,
                                  const ReportingType reportingTypeIn) const
{
    if (reportUpdatesIn == ReportUpdates::overwrite ||
        reportingTypeIn == ReportingType::onRequest)
    {
        return metricCount;
    }
    else
    {
        return appendLimit;
    }
}

void Report::setReadingBuffer(const ReportUpdates newReportUpdates)
{
    const auto newBufferSize =
        deduceBufferSize(newReportUpdates, reportingType);
    if (readingsBuffer.size() != newBufferSize)
    {
        readingsBuffer.clearAndResize(newBufferSize);
    }
}

void Report::setReportUpdates(const ReportUpdates newReportUpdates)
{
    if (reportUpdates != newReportUpdates)
    {
        setReadingBuffer(newReportUpdates);
        reportUpdates = newReportUpdates;
    }
}

std::unique_ptr<sdbusplus::asio::dbus_interface>
    Report::makeReportInterface(const interfaces::ReportFactory& reportFactory)
{
    auto dbusIface =
        objServer->add_unique_interface(getPath(), reportIfaceName);
    dbusIface->register_property_rw<bool>(
        "Enabled", sdbusplus::vtable::property_::emits_change,
        [this](bool newVal, auto& oldValue) {
            if (newVal != state.get<ReportFlags::enabled>())
            {
                state.set<ReportFlags::enabled>(oldValue = newVal);

                persistency = storeConfiguration();
            }
            return 1;
        },
        [this](const auto&) { return state.get<ReportFlags::enabled>(); });
    dbusIface->register_method(
        "SetReportingProperties",
        [this](std::string newReportingType, uint64_t newInterval) {
            ReportingType newReportingTypeT = reportingType;

            if (!newReportingType.empty())
            {
                newReportingTypeT = utils::toReportingType(newReportingType);
            }

            Milliseconds newIntervalT = interval;

            if (newInterval != std::numeric_limits<uint64_t>::max())
            {
                newIntervalT = Milliseconds(newInterval);
            }

            auto errorMessages = verify(newReportingTypeT, newIntervalT);

            if (!errorMessages.empty())
            {
                if (newIntervalT != interval)
                {
                    throw errors::InvalidArgument("Interval");
                }

                throw errors::InvalidArgument("ReportingType");
            }

            if (reportingType != newReportingTypeT)
            {
                reportingType = newReportingTypeT;
                reportIface->signal_property("ReportingType");
            }

            if (interval != newIntervalT)
            {
                interval = newIntervalT;
                reportIface->signal_property("Interval");
            }

            if (state.set<ReportFlags::valid>(errorMessages.empty()) ==
                StateEvent::active)
            {
                scheduleTimer();
            }

            persistency = storeConfiguration();

            setReadingBuffer(reportUpdates);
        });
    dbusIface->register_property_r<uint64_t>(
        "Interval", sdbusplus::vtable::property_::emits_change,
        [this](const auto&) { return interval.count(); });
    dbusIface->register_property_rw<bool>(
        "Persistency", sdbusplus::vtable::property_::emits_change,
        [this](bool newVal, auto& oldVal) {
            if (newVal == persistency)
            {
                return 1;
            }
            if (newVal)
            {
                persistency = oldVal = storeConfiguration();
            }
            else
            {
                reportStorage.remove(reportFileName());
                persistency = oldVal = false;
            }
            return 1;
        },
        [this](const auto&) { return persistency; });

    dbusIface->register_property_r("Readings", readings,
                                   sdbusplus::vtable::property_::emits_change,
                                   [this](const auto&) { return readings; });
    dbusIface->register_property_r<std::string>(
        "ReportingType", sdbusplus::vtable::property_::emits_change,
        [this](const auto&) { return utils::enumToString(reportingType); });
    dbusIface->register_property_rw(
        "ReadingParameters", readingParameters,
        sdbusplus::vtable::property_::emits_change,
        [this, &reportFactory](auto newVal, auto& oldVal) {
            auto labeledMetricParams =
                reportFactory.convertMetricParams(newVal);
            reportFactory.updateMetrics(metrics,
                                        state.get<ReportFlags::enabled>(),
                                        labeledMetricParams);
            readingParameters = toReadingParameters(
                utils::transform(metrics, [](const auto& metric) {
                    return metric->dumpConfiguration();
                }));
            metricCount = getMetricCount(metrics);
            setReadingBuffer(reportUpdates);
            persistency = storeConfiguration();
            oldVal = std::move(newVal);
            return 1;
        },
        [this](const auto&) { return readingParameters; });
    dbusIface->register_property_r<bool>(
        "EmitsReadingsUpdate", sdbusplus::vtable::property_::none,
        [this](const auto&) {
            return reportActions.contains(ReportAction::emitsReadingsUpdate);
        });
    dbusIface->register_property_r<std::string>(
        "Name", sdbusplus::vtable::property_::const_,
        [this](const auto&) { return name; });
    dbusIface->register_property_r<bool>(
        "LogToMetricReportsCollection", sdbusplus::vtable::property_::const_,
        [this](const auto&) {
            return reportActions.contains(
                ReportAction::logToMetricReportsCollection);
        });
    dbusIface->register_property_rw<std::vector<std::string>>(
        "ReportActions", sdbusplus::vtable::property_::emits_change,
        [this](auto newVal, auto& oldVal) {
            auto tmp = utils::transform<std::unordered_set>(
                newVal, [](const auto& reportAction) {
                    return utils::toReportAction(reportAction);
                });
            tmp.insert(ReportAction::logToMetricReportsCollection);

            if (tmp != reportActions)
            {
                reportActions = tmp;
                persistency = storeConfiguration();
                oldVal = std::move(newVal);
            }
            return 1;
        },
        [this](const auto&) {
            return utils::transform<std::vector>(
                reportActions, [](const auto reportAction) {
                    return utils::enumToString(reportAction);
                });
        });
    dbusIface->register_property_r<uint64_t>(
        "AppendLimit", sdbusplus::vtable::property_::emits_change,
        [this](const auto&) { return appendLimit; });
    dbusIface->register_property_rw(
        "ReportUpdates", std::string(),
        sdbusplus::vtable::property_::emits_change,
        [this](auto newVal, auto& oldVal) {
            setReportUpdates(utils::toReportUpdates(newVal));
            oldVal = newVal;
            return 1;
        },
        [this](const auto&) { return utils::enumToString(reportUpdates); });
    dbusIface->register_property_r(
        "Triggers", std::vector<sdbusplus::message::object_path>{},
        sdbusplus::vtable::property_::emits_change, [this](const auto&) {
            return utils::transform<std::vector>(
                triggerIds, [](const auto& triggerId) {
                    return utils::pathAppend(utils::constants::triggerDirPath,
                                             triggerId);
                });
        });
    dbusIface->register_method("Update", [this] {
        if (reportingType == ReportingType::onRequest)
        {
            updateReadings();
        }
    });
    constexpr bool skipPropertiesChangedSignal = true;
    dbusIface->initialize(skipPropertiesChangedSignal);
    return dbusIface;
}

void Report::timerProcForPeriodicReport(boost::system::error_code ec,
                                        Report& self)
{
    if (ec)
    {
        return;
    }

    self.updateReadings();
    self.scheduleTimerForPeriodicReport(self.interval);
}

void Report::timerProcForOnChangeReport(boost::system::error_code ec,
                                        Report& self)
{
    if (ec)
    {
        return;
    }

    const auto ensure =
        utils::Ensure{[&self] { self.onChangeContext = std::nullopt; }};

    self.onChangeContext.emplace(self);

    const auto steadyTimestamp = self.clock->steadyTimestamp();

    for (auto& metric : self.metrics)
    {
        metric->updateReadings(steadyTimestamp);
    }

    self.scheduleTimerForOnChangeReport();
}

void Report::scheduleTimerForPeriodicReport(Milliseconds timerInterval)
{
    timer.expires_after(timerInterval);
    timer.async_wait([this](boost::system::error_code ec) {
        timerProcForPeriodicReport(ec, *this);
    });
}

void Report::scheduleTimerForOnChangeReport()
{
    constexpr Milliseconds timerInterval{100};

    timer.expires_after(timerInterval);
    timer.async_wait([this](boost::system::error_code ec) {
        timerProcForOnChangeReport(ec, *this);
    });
}

void Report::updateReadings()
{
    if (!state.isActive())
    {
        return;
    }

    if (reportUpdates == ReportUpdates::overwrite ||
        reportingType == ReportingType::onRequest)
    {
        readingsBuffer.clear();
    }

    for (const auto& metric : metrics)
    {
        if (!state.isActive())
        {
            break;
        }

        for (const auto& [metadata, value, timestamp] :
             metric->getUpdatedReadings())
        {
            if (reportUpdates == ReportUpdates::appendStopsWhenFull &&
                readingsBuffer.isFull())
            {
                state.set<ReportFlags::enabled>(false);
                reportIface->signal_property("Enabled");
                break;
            }
            readingsBuffer.emplace(metadata, value, timestamp);
        }
    }

    std::get<0>(readings) =
        std::chrono::duration_cast<Milliseconds>(clock->systemTimestamp())
            .count();

    if (utils::contains(reportActions, ReportAction::emitsReadingsUpdate))
    {
        reportIface->signal_property("Readings");
    }
}

bool Report::shouldStoreMetricValues() const
{
    return reportingType != ReportingType::onRequest &&
           reportUpdates == ReportUpdates::appendStopsWhenFull;
}

bool Report::storeConfiguration() const
{
    try
    {
        nlohmann::json data;

        data["Enabled"] = state.get<ReportFlags::enabled>();
        data["Version"] = reportVersion;
        data["Id"] = id;
        data["Name"] = name;
        data["ReportingType"] = utils::toUnderlying(reportingType);
        data["ReportActions"] =
            utils::transform(reportActions, [](const auto reportAction) {
                return utils::toUnderlying(reportAction);
            });
        data["Interval"] = interval.count();
        data["AppendLimit"] = appendLimit;
        data["ReportUpdates"] = utils::toUnderlying(reportUpdates);
        data["ReadingParameters"] =
            utils::transform(metrics, [](const auto& metric) {
                return metric->dumpConfiguration();
            });

        if (shouldStoreMetricValues())
        {
            data["MetricValues"] = utils::toLabeledReadings(readings);
        }

        reportStorage.store(reportFileName(), data);
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

interfaces::JsonStorage::FilePath Report::reportFileName() const
{
    return interfaces::JsonStorage::FilePath{
        std::to_string(std::hash<std::string>{}(id))};
}

std::unordered_set<std::string>
    Report::collectTriggerIds(boost::asio::io_context& ioc) const
{
    utils::Messanger tmp(ioc);

    auto result = std::unordered_set<std::string>();

    tmp.on_receive<messages::CollectTriggerIdResp>(
        [&result](const auto& msg) { result.insert(msg.triggerId); });

    tmp.send(messages::CollectTriggerIdReq{id});

    return result;
}

void Report::metricUpdated()
{
    if (onChangeContext)
    {
        onChangeContext->metricUpdated();
        return;
    }

    updateReadings();
}

void Report::scheduleTimer()
{
    switch (reportingType)
    {
        case ReportingType::periodic:
        {
            unregisterFromMetrics = nullptr;
            scheduleTimerForPeriodicReport(interval);
            break;
        }
        case ReportingType::onChange:
        {
            if (!unregisterFromMetrics)
            {
                unregisterFromMetrics = [this] {
                    for (auto& metric : metrics)
                    {
                        metric->unregisterFromUpdates(*this);
                    }
                };

                for (auto& metric : metrics)
                {
                    metric->registerForUpdates(*this);
                }
            }

            bool isTimerRequired = false;

            for (auto& metric : metrics)
            {
                if (metric->isTimerRequired())
                {
                    isTimerRequired = true;
                }
            }

            if (isTimerRequired)
            {
                scheduleTimerForOnChangeReport();
            }
            else
            {
                timer.cancel();
            }
            break;
        }
        default:
            unregisterFromMetrics = nullptr;
            timer.cancel();
            break;
    }
}

std::vector<ErrorMessage> Report::verify(ReportingType reportingType,
                                         Milliseconds interval)
{
    if (interval != Milliseconds{0} && interval < ReportManager::minInterval)
    {
        throw errors::InvalidArgument("Interval");
    }

    std::vector<ErrorMessage> result;

    if ((reportingType == ReportingType::periodic &&
         interval == Milliseconds{0}) ||
        (reportingType != ReportingType::periodic &&
         interval != Milliseconds{0}))
    {
        result.emplace_back(ErrorType::propertyConflict, "Interval");
        result.emplace_back(ErrorType::propertyConflict, "ReportingType");
    }

    return result;
}
