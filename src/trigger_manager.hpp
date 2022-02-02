#pragma once

#include "interfaces/report_manager.hpp"
#include "interfaces/trigger_factory.hpp"
#include "interfaces/trigger_manager.hpp"
#include "trigger.hpp"

#include <sdbusplus/asio/object_server.hpp>

#include <memory>
#include <vector>

class TriggerManager : public interfaces::TriggerManager
{
  public:
    TriggerManager(
        std::unique_ptr<interfaces::TriggerFactory> triggerFactory,
        std::unique_ptr<interfaces::JsonStorage> triggerStorage,
        const std::shared_ptr<sdbusplus::asio::object_server>& objServer);

    TriggerManager(const TriggerManager&) = delete;
    TriggerManager(TriggerManager&&) = delete;
    TriggerManager& operator=(const TriggerManager&) = delete;
    TriggerManager& operator=(TriggerManager&&) = delete;

    void removeTrigger(const interfaces::Trigger* trigger) override;

    static void verifyReportIds(const std::vector<std::string>& newReportIds);

    std::vector<std::string>
        getTriggerIdsForReport(const std::string& reportId) const override;

  private:
    std::unique_ptr<interfaces::TriggerFactory> triggerFactory;
    std::unique_ptr<interfaces::JsonStorage> triggerStorage;
    std::unique_ptr<sdbusplus::asio::dbus_interface> managerIface;
    std::vector<std::unique_ptr<interfaces::Trigger>> triggers;

    void verifyAddTrigger(const std::string& triggerId,
                          const std::string& triggerName,
                          const std::vector<std::string>& newReportIds) const;

    interfaces::Trigger&
        addTrigger(const std::string& triggerId, const std::string& triggerName,
                   const std::vector<std::string>& triggerActions,
                   const std::vector<LabeledSensorInfo>& labeledSensors,
                   const std::vector<std::string>& reportIds,
                   const LabeledTriggerThresholdParams& labeledThresholdParams);
    void loadFromPersistent();

  public:
    static constexpr size_t maxTriggers{TELEMETRY_MAX_TRIGGERS};
    static constexpr size_t maxTriggerIdLength{
        TELEMETRY_MAX_DBUS_PATH_LENGTH -
        std::string_view(Trigger::triggerDir).length()};
    static constexpr const char* triggerManagerIfaceName =
        "xyz.openbmc_project.Telemetry.TriggerManager";
    static constexpr const char* triggerManagerPath =
        "/xyz/openbmc_project/Telemetry/Triggers";
    static constexpr const char* triggerNameDefault = "Trigger";
};
