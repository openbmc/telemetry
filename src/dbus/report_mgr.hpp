#pragma once

#include "core/persistent_storage.hpp"
#include "core/report_mgr.hpp"
#include "dbus/consts.hpp"
#include "dbus/dbus.hpp"
#include "dbus/report.hpp"
#include "dbus/report_configuration.hpp"
#include "dbus/report_factory.hpp"
#include "dbus/sensor.hpp"
#include "log.hpp"
#include "utils/dbus_interfaces.hpp"
#include "utils/detached_timer.hpp"
#include "utils/utils.hpp"

#include <sdbusplus/asio/object_server.hpp>

#include <memory>
#include <sstream>

namespace dbus
{

class ReportManager :
    public Report::RemoveIface,
    public std::enable_shared_from_this<ReportManager>
{
    // Prevents constructor from being called externally
    struct ctor_lock
    {};

  public:
    ReportManager(ctor_lock, std::shared_ptr<sdbusplus::asio::connection> bus,
                  std::shared_ptr<sdbusplus::asio::object_server> objServer) :
        bus_(bus),
        objServer_(objServer), reports_{std::make_shared<typeof(*reports_)>()},
        manager_{std::make_shared<core::ReportManager>(app::maxReports,
                                                       app::pollRateResolution)}
    {
        bus_->request_name(dbus::Service);
    }

    static std::shared_ptr<ReportManager>
        make(std::shared_ptr<sdbusplus::asio::connection> bus,
             std::shared_ptr<sdbusplus::asio::object_server> objServer)
    {
        auto mgr = std::make_shared<ReportManager>(ctor_lock{}, bus, objServer);

        mgr->dbusInterfaces_.addInterface(
            dbus::subPath("Reports"), dbus::subIface("ReportsManagement"),
            [&mgr](sdbusplus::asio::dbus_interface& interface) {
                interface.register_property(
                    "MaxReports",
                    static_cast<uint32_t>(mgr->manager_->maxReports()));

                interface.register_property(
                    "PollRateResolution",
                    static_cast<uint32_t>(
                        mgr->manager_->pollRateResolution().count()));

                interface.register_method(
                    "AddReport",
                    [weakMgr = utils::weak_ptr(mgr)](
                        boost::asio::yield_context& yield,
                        const std::string& name, const std::string& domain,
                        const std::string& reportingType,
                        const std::vector<std::string>& reportAction,
                        const uint32_t scanPeriod,
                        const std::vector<MetricParams>& metrics) {
                        if (auto mgr = weakMgr.lock())
                        {
                            const auto reportName =
                                core::ReportName(domain, name);
                            mgr->manager_->checkIfAllowedToAdd(reportName);

                            auto [coreReport, dbusReport] =
                                mgr->reportFactory_->make(
                                    yield, mgr->bus_, reportName, reportingType,
                                    reportAction, scanPeriod, metrics,
                                    mgr->objServer_);
                            /* TODO: Add rollback of add() in case of failure */
                            mgr->manager_->add(std::move(coreReport));
                            mgr->add(dbusReport);
                            return std::string(dbusReport->path());
                        }

                        utils::throwError(
                            std::errc::resource_unavailable_try_again,
                            "ReportManager unavailable!");
                    });
            });

        mgr->registerForReportsFromPersistentStorage();

        return mgr;
    }

    std::shared_ptr<dbus::Report> add(std::shared_ptr<dbus::Report> report)
    {
        reports_->emplace_back(report);
        report->addRemoveInterface(weak_from_this());
        return report;
    }

    void remove(std::shared_ptr<dbus::Report>& report) override
    {
        auto it = std::find(reports_->begin(), reports_->end(), report);
        if (it != reports_->end())
        {
            LOG_DEBUG << "dbus::ReportManager::Remove " << report->name();
            manager_->remove(report->handle());
            reports_->erase(it);
            report->removeConfiguration();
        }
    }

  private:
    void spawnRegisterForReportFromPersistentStorage(
        const std::shared_ptr<ReportConfiguration>& rc,
        std::chrono::seconds delay)
    {
        boost::asio::spawn(bus_->get_io_context(),
                           [delay, weakMgr = weak_from_this(),
                            rc](boost::asio::yield_context yield) {
                               if (auto mgr = weakMgr.lock())
                               {
                                   mgr->registerForReportFromPersistentStorage(
                                       yield, rc, delay);
                               }
                           });
    }

    void registerForReportsFromPersistentStorage()
    {
        for (const auto& reportPath : ReportConfiguration::list(*storage_))
        {
            if (auto configuration =
                    storage_->load(ReportConfiguration::path(reportPath)))
            {
                if (!configuration)
                {
                    continue;
                }
                try
                {
                    const auto rc =
                        std::make_shared<ReportConfiguration>(*configuration);
                    spawnRegisterForReportFromPersistentStorage(
                        rc, std::chrono::seconds(10));
                }
                catch (const nlohmann::json::exception& e)
                {
                    LOG_ERROR << "Exception throw while parsing "
                              << ReportConfiguration::path(reportPath)
                              << ". Details: " << e.what();
                }
            }
        }
    }

    void registerForReportFromPersistentStorage(
        boost::asio::yield_context& yield,
        const std::shared_ptr<ReportConfiguration>& rc,
        std::chrono::seconds delay)
    {
        const auto reportName = core::ReportName(rc->domain, rc->name);

        try
        {
            manager_->checkIfAllowedToAdd(reportName);

            auto [coreReport, dbusReport] = reportFactory_->make(
                yield, bus_, reportName, rc->reportingType, rc->reportAction,
                rc->scanPeriod, rc->metricParams, objServer_);

            /* TODO: Add rollback of add() in case of failure */
            manager_->add(std::move(coreReport));
            add(dbusReport);

            dbusReport->setPersistency(
                toEnum(core::persistencyConvertData, rc->persistency));
        }
        catch (const sdbusplus::exception::SdBusError& e)
        {
            LOG_ERROR << "Exception throwed while adding report "
                      << ReportConfiguration::path(reportName)
                      << " from persistent storage. Details: " << e.what();

            /*
             * TODO: In case of missing or not loaded sensor service tries once
             *       again to add a Report. It should always create a report
             *       with status that resource/sensor is not available.
             */
            if (e.get_errno() != static_cast<int>(std::errc::io_error) &&
                e.get_errno() !=
                    static_cast<int>(std::errc::no_such_file_or_directory))
            {
                return;
            }

            utils::makeDetachedTimer(
                bus_->get_io_context(), delay,
                [delay, weakMgr = weak_from_this(), rc] {
                    if (auto mgr = weakMgr.lock())
                    {
                        mgr->spawnRegisterForReportFromPersistentStorage(
                            rc, delay + std::chrono::seconds(1));
                    }
                });
        }
    }

    std::shared_ptr<sdbusplus::asio::connection> bus_;
    std::shared_ptr<sdbusplus::asio::object_server> objServer_;
    std::shared_ptr<std::vector<std::shared_ptr<dbus::Report>>> reports_;
    std::shared_ptr<core::ReportManager> manager_;
    std::shared_ptr<core::interfaces::Storage> storage_ =
        std::make_shared<core::PersistentStorage>("/var/telemetry.d");
    std::shared_ptr<ReportFactory<>> reportFactory_ =
        std::make_shared<ReportFactory<>>(
            bus_->get_io_context(), manager_->pollRateResolution(), storage_);
    utils::DbusInterfaces dbusInterfaces_{objServer_};

    ReportManager(const ReportManager&) = delete;
    ReportManager& operator=(const ReportManager&) = delete;
};

} // namespace dbus
