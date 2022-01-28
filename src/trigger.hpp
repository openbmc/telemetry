#pragma once

#include "interfaces/json_storage.hpp"
#include "interfaces/report_manager.hpp"
#include "interfaces/threshold.hpp"
#include "interfaces/trigger.hpp"
#include "interfaces/trigger_factory.hpp"
#include "interfaces/trigger_manager.hpp"
#include "types/trigger_types.hpp"

#include <boost/asio/io_context.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <memory>

class Trigger : public interfaces::Trigger
{
  public:
    Trigger(boost::asio::io_context& ioc,
            const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
            const std::string& id, const std::string& name,
            const std::vector<TriggerAction>& triggerActions,
            const std::shared_ptr<std::vector<std::string>> reportIds,
            std::vector<std::shared_ptr<interfaces::Threshold>>&& thresholds,
            interfaces::TriggerManager& triggerManager,
            interfaces::JsonStorage& triggerStorage,
            const interfaces::TriggerFactory& triggerFactory, Sensors sensorsIn,
            interfaces::ReportManager& reportManager);

    Trigger(const Trigger&) = delete;
    Trigger(Trigger&&) = delete;
    Trigger& operator=(const Trigger&) = delete;
    Trigger& operator=(Trigger&&) = delete;

    std::string getId() const override
    {
        return id;
    }

    std::string getPath() const override
    {
        return path;
    }

    bool storeConfiguration() const;

    const std::vector<std::string>& getReportIds() const override
    {
        return *reportIds;
    }

  private:
    std::string id;
    std::string name;
    std::vector<TriggerAction> triggerActions;
    std::string path;
    bool persistent = false;
    std::shared_ptr<std::vector<std::string>> reportIds;
    std::unique_ptr<sdbusplus::asio::dbus_interface> deleteIface;
    std::unique_ptr<sdbusplus::asio::dbus_interface> triggerIface;
    std::vector<std::shared_ptr<interfaces::Threshold>> thresholds;

    interfaces::JsonStorage::FilePath fileName;
    interfaces::JsonStorage& triggerStorage;
    Sensors sensors;
    interfaces::ReportManager& reportManager;

    void
        updateTriggerIdsInReports(const std::vector<std::string>& newReportIds);

  public:
    static constexpr const char* triggerIfaceName =
        "xyz.openbmc_project.Telemetry.Trigger";
    static constexpr const char* triggerDir =
        "/xyz/openbmc_project/Telemetry/Triggers/";
    static constexpr const char* deleteIfaceName =
        "xyz.openbmc_project.Object.Delete";
    static constexpr size_t triggerVersion = 1;
};
