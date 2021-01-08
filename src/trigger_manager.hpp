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
        const std::shared_ptr<sdbusplus::asio::object_server>& objServer);

    TriggerManager(TriggerManager&) = delete;
    TriggerManager(TriggerManager&&) = delete;
    TriggerManager& operator=(TriggerManager&) = delete;
    TriggerManager& operator=(TriggerManager&&) = delete;

    void removeTrigger(const interfaces::Trigger* trigger) override;

  private:
    std::unique_ptr<interfaces::TriggerFactory> triggerFactory;
    std::unique_ptr<sdbusplus::asio::dbus_interface> managerIface;
    std::vector<std::unique_ptr<interfaces::Trigger>> triggers;

  public:
    static constexpr size_t maxTriggers{TELEMETRY_MAX_TRIGGERS};
    static constexpr const char* triggerManagerIfaceName =
        "xyz.openbmc_project.Telemetry.TriggerManager";
    static constexpr const char* triggerManagerPath =
        "/xyz/openbmc_project/Telemetry/Triggers";
};
