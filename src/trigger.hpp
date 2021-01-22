#pragma once

#include "interfaces/threshold.hpp"
#include "interfaces/trigger.hpp"
#include "interfaces/trigger_manager.hpp"
#include "interfaces/trigger_types.hpp"

#include <boost/asio/io_context.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <memory>

class Trigger : public interfaces::Trigger
{
  public:
    Trigger(
        boost::asio::io_context& ioc,
        const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
        const std::string& name, const bool isDiscrete, const bool logToJournal,
        const bool logToRedfish, const bool updateReport,
        const std::vector<
            std::pair<sdbusplus::message::object_path, std::string>>& sensorsIn,
        const std::vector<std::string>& reportNames,
        const TriggerThresholdParams& thresholdParams,
        std::vector<std::shared_ptr<interfaces::Threshold>>&& thresholds,
        interfaces::TriggerManager& triggerManager);

    Trigger(const Trigger&) = delete;
    Trigger(Trigger&&) = delete;
    Trigger& operator=(const Trigger&) = delete;
    Trigger& operator=(Trigger&&) = delete;

    std::string getName() const override
    {
        return name;
    }

    std::string getPath() const override
    {
        return path;
    }

  private:
    const std::string name;
    const std::string path;
    bool persistent;
    std::vector<std::pair<sdbusplus::message::object_path, std::string>>
        sensors;
    std::vector<std::string> reportNames;
    TriggerThresholdParams thresholdParams;
    std::unique_ptr<sdbusplus::asio::dbus_interface> deleteIface;
    std::unique_ptr<sdbusplus::asio::dbus_interface> triggerIface;
    std::vector<std::shared_ptr<interfaces::Threshold>> thresholds;

  public:
    static constexpr const char* triggerIfaceName =
        "xyz.openbmc_project.Telemetry.Trigger";
    static constexpr const char* triggerDir =
        "/xyz/openbmc_project/Telemetry/Triggers/";
    static constexpr const char* deleteIfaceName =
        "xyz.openbmc_project.Object.Delete";
};
