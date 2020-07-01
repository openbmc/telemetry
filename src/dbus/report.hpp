#pragma once

#include "core/interfaces/storage.hpp"
#include "core/persistency.hpp"
#include "core/report_mgr.hpp"
#include "dbus/dbus.hpp"
#include "dbus/report_configuration.hpp"
#include "dbus/sensor.hpp"
#include "log.hpp"
#include "utils/dbus_interfaces.hpp"
#include "utils/utils.hpp"

#include <nlohmann/json.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <memory>
#include <tuple>

namespace dbus
{

class Report : public std::enable_shared_from_this<Report>
{
  public:
    class RemoveIface
    {
      public:
        virtual void remove(std::shared_ptr<Report>& report) = 0;

        virtual ~RemoveIface() = default;
    };

    Report(std::shared_ptr<sdbusplus::asio::connection> bus,
           std::shared_ptr<sdbusplus::asio::object_server> objServer,
           core::ReportManager::ReportHandle report,
           const core::ReportName& name,
           const std::shared_ptr<core::interfaces::Storage> storage) :
        bus_(bus),
        objServer_(objServer), report_(report),
        path_(dbus::subPath("Reports/" + name.str())), name_(name),
        storage_(storage)
    {
        LOG_DEBUG_T(path_) << "dbus::Report Constructor";

        dbusInterfaces_.addInterface(
            path_, dbus::subIface("Report"), [this](auto& iface) {
                timestamp_ = dbusInterfaces_.make_property_view(
                    iface, "Timestamp",
                    [this] { return toIsoTime(report_->timestamp()); });
                readings_ = dbusInterfaces_.make_property_view(
                    iface, "Readings",
                    [this] { return convert(report_->readings()); });
                persistency_ = dbusInterfaces_.make_property_rw(
                    iface, "Persistency",
                    toString(core::persistencyConvertData,
                             core::defaultPersistency));

                reportUpdate_ = dbusInterfaces_.make_signal<void>(
                    bus_, iface, "ReportUpdate");

                iface.register_property(
                    "ReportingType",
                    getReportingType(report_->reportingType()));
                iface.register_property(
                    "ReportAction", getReportAction(report_->reportAction()));
                iface.register_property(
                    "ScanPeriod",
                    static_cast<uint32_t>(report_->scanPeriod().count()));
                iface.register_property("ReadingParameters",
                                        convert(report_->metrics()));

                iface.register_method(
                    "Update",
                    [name = name_, &iface, report = utils::weak_ptr(report_)] {
                        if (auto report_ = report.lock())
                        {
                            report_->update();
                        }
                    });
            });

        onPersistencyChanged_ = persistency_->onChange(
            [this](const auto& persistency) { store(); });
        persistency_->addValidator([](const auto& value) {
            toEnum(core::persistencyConvertData, value);
            return true;
        });

        report_->setCallback([name = name_,
                              readings = utils::weak_ptr(readings_),
                              timestamp = utils::weak_ptr(timestamp_),
                              reportUpdate = utils::weak_ptr(reportUpdate_)] {
            if (auto readings_ = readings.lock())
            {
                readings_->update();
            }
            if (auto timestamp_ = timestamp.lock())
            {
                timestamp_->update();
            }
            if (auto reportUpdate_ = reportUpdate.lock())
            {
                reportUpdate_->signal();
            }
        });

        store();
    }

    Report(const Report&) = delete;
    Report& operator=(const Report&) = delete;

    void addRemoveInterface(std::weak_ptr<RemoveIface> weakRemoveIface)
    {
        dbusInterfaces_.addInterface(
            path_, open_bmc::DeleteIface,
            [this, weakRemoveIface](auto& deleteIface) {
                deleteIface.register_method(
                    "Delete", [weakSelf = weak_from_this(), weakRemoveIface] {
                        if (auto self = weakSelf.lock())
                        {
                            LOG_DEBUG_T(self->path()) << "Triggered delete";
                            self->bus_->get_io_context().post(
                                [weakSelf, weakRemoveIface] {
                                    if (auto self = weakSelf.lock())
                                    {
                                        LOG_DEBUG_T(self->path()) << "Removing";
                                        if (auto removeIface =
                                                weakRemoveIface.lock())
                                        {
                                            removeIface->remove(self);
                                        }
                                    }
                                });
                        }
                    });
            });
    }

    ~Report()
    {
        LOG_DEBUG_T(path_) << "dbus::Report ~Report";
    }

    std::string_view path() const
    {
        return path_;
    }

    core::ReportName name() const
    {
        return name_;
    }

    core::ReportManager::ReportHandle& handle()
    {
        return report_;
    }

    void setPersistency(core::Persistency persistency)
    {
        persistency_->set(toString(core::persistencyConvertData, persistency));
    }

    void store() const
    {
        const auto value =
            toEnum(core::persistencyConvertData, persistency_->get());

        if (value == core::Persistency::configurationOnly or
            value == core::Persistency::configurationAndData)
        {
            storage_->store(ReportConfiguration::path(name_),
                            dumpConfiguration());
        }
        else
        {
            storage_->remove(ReportConfiguration::path(name_));
        }
    }

    void removeConfiguration() const
    {
        storage_->remove(ReportConfiguration::path(name_));
    }

    /* TODO: Move to core::Report if possible */
    nlohmann::json dumpConfiguration() const
    {
        auto result = nlohmann::json::object();

        result["version"] = ReportConfiguration::Version;
        result["name"] = std::string(name_.name());
        result["domain"] = std::string(name_.domain());
        result["reportingType"] = getReportingType(report_->reportingType());
        result["reportAction"] = getReportAction(report_->reportAction());
        result["scanPeriod"] = report_->scanPeriod().count();
        result["persistency"] = persistency_->get();
        result["metricParams"] = nlohmann::json::array();
        for (const ReadingParam& metric : convert(report_->metrics()))
        {
            auto item = nlohmann::json::object();

            item["sensorPaths"] = std::get<0>(metric);
            item["operationType"] = std::get<1>(metric);
            item["id"] = std::get<2>(metric);
            item["metricMetadata"] = std::get<3>(metric);

            result["metricParams"].emplace_back(std::move(item));
        }

        return result;
    }

    friend std::ostream& operator<<(std::ostream& os, const Report& r);

  private:
    // TODO: Optimize? Called only once, so maybe no need
    static inline std::vector<ReadingParam>
        convert(const boost::beast::span<
                const std::shared_ptr<core::interfaces::Metric>>& coreMetrics)
    {
        // TODO: DOCS: MetricParams and ReadingParameters are basically the
        // same, but with different order.. UNIFY
        std::vector<ReadingParam> readingParams;
        readingParams.reserve(coreMetrics.size());

        for (auto& metric : coreMetrics)
        {
            api::SensorPaths sensorPaths;
            sensorPaths.reserve(metric->sensors().size());
            for (auto& sensor : metric->sensors())
            {
                sensorPaths.push_back(std::string(sensor->id()->str()));
            }

            readingParams.emplace_back(std::make_tuple(
                std::move(sensorPaths), getOperationType(metric->type()),
                metric->id(), metric->metadata()));
        }

        return readingParams;
    }

    // TODO: Optimize - most of the time structures are the same, only value and
    // timestamp changes
    static inline std::vector<dbus::Reading> convert(
        const std::vector<core::interfaces::Report::Reading>& coreReadings)
    {
        std::vector<dbus::Reading> readings;
        readings.reserve(coreReadings.size());

        for (auto& reportReadings : coreReadings)
        {
            for (auto& reading : reportReadings.readings)
            {
                if (reading)
                {
                    readings.emplace_back(std::make_tuple(
                        reportReadings.metric->id(),
                        reportReadings.metric->metadata(), reading->value,
                        toIsoTime(reading->timestamp)));
                }
            }
        }

        return readings;
    }

    std::shared_ptr<sdbusplus::asio::connection> bus_;
    std::shared_ptr<sdbusplus::asio::object_server> objServer_;
    std::shared_ptr<core::interfaces::Report> report_;
    const std::string path_;
    const core::ReportName name_;
    std::shared_ptr<core::interfaces::Storage> storage_;

    utils::DbusInterfaces dbusInterfaces_{objServer_};
    utils::PropertyViewPtr<std::string> timestamp_;
    utils::PropertyViewPtr<std::vector<dbus::Reading>> readings_;
    utils::SignalPtr<void> reportUpdate_;
    utils::PropertyPtr<std::string> persistency_;
    boost::signals2::scoped_connection onPersistencyChanged_;
};

inline std::ostream& operator<<(std::ostream& os, const Report& r)
{
    return os << r.name_;
}

} // namespace dbus
