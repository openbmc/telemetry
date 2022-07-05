#pragma once

#include "interfaces/report_manager.hpp"
#include "interfaces/trigger_factory.hpp"
#include "interfaces/trigger_manager.hpp"
#include "trigger.hpp"
#include "utils/dbus_path_utils.hpp"

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
    static void verifyThresholdParams(
        const LabeledTriggerThresholdParams& thresholdParams);

  private:
    std::unique_ptr<interfaces::TriggerFactory> triggerFactory;
    std::unique_ptr<interfaces::JsonStorage> triggerStorage;
    std::unique_ptr<sdbusplus::asio::dbus_interface> managerIface;
    std::vector<std::unique_ptr<interfaces::Trigger>> triggers;

    void verifyAddTrigger(
        const std::vector<std::string>& reportIds,
        const LabeledTriggerThresholdParams& thresholdParams) const;

    interfaces::Trigger&
        addTrigger(const std::string& triggerId, const std::string& triggerName,
                   const std::vector<std::string>& triggerActions,
                   const std::vector<LabeledSensorInfo>& labeledSensors,
                   const std::vector<std::string>& reportIds,
                   const LabeledTriggerThresholdParams& labeledThresholdParams);
    void loadFromPersistent();

  public:
    static constexpr size_t maxTriggers{TELEMETRY_MAX_TRIGGERS};
    static constexpr const char* triggerManagerIfaceName =
        "xyz.openbmc_project.Telemetry.TriggerManager";
    static constexpr const char* triggerManagerPath =
        "/xyz/openbmc_project/Telemetry/Triggers";
    static constexpr const char* triggerNameDefault = "Trigger";
};
