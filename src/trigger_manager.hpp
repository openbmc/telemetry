#pragma once

#include "interfaces/report_manager.hpp"
#include "interfaces/trigger_factory.hpp"
#include "interfaces/trigger_manager.hpp"

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

    TriggerManager(TriggerManager&) = delete;
    TriggerManager(TriggerManager&&) = delete;
    TriggerManager& operator=(TriggerManager&) = delete;
    TriggerManager& operator=(TriggerManager&&) = delete;

    void removeTrigger(const interfaces::Trigger* trigger) override;
    static void verifyTriggerNameLength(const std::string& triggerName);

  private:
    std::unique_ptr<interfaces::TriggerFactory> triggerFactory;
    std::unique_ptr<interfaces::JsonStorage> triggerStorage;
    std::unique_ptr<sdbusplus::asio::dbus_interface> managerIface;
    std::vector<std::unique_ptr<interfaces::Trigger>> triggers;

    void verifyAddTrigger(const std::string& triggerId,
                          const std::string& triggerName) const;
    std::string generateId(const std::string& triggerName) const;
    static void verifyTriggerIdLength(const std::string& triggerId);

    interfaces::Trigger&
        addTrigger(const std::string& triggerId, const std::string& triggerName,
                   const std::vector<std::string>& triggerActions,
                   const std::vector<LabeledSensorInfo>& labeledSensors,
                   const std::vector<std::string>& reportNames,
                   const LabeledTriggerThresholdParams& labeledThresholdParams);
    void loadFromPersistent();

  public:
    static constexpr size_t maxTriggers{TELEMETRY_MAX_TRIGGERS};
    static constexpr size_t maxTriggerNameAndIdLength{
        TELEMETRY_MAX_TRIGGER_NAME_AND_ID_LENGTH};
    static constexpr const char* triggerManagerIfaceName =
        "xyz.openbmc_project.Telemetry.TriggerManager";
    static constexpr const char* triggerManagerPath =
        "/xyz/openbmc_project/Telemetry/Triggers";

    static constexpr std::string_view allowedCharactersInId =
        "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";
    static constexpr const char* triggerNameDefault = "Trigger";
};
