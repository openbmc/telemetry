#pragma once

#include "interfaces/trigger_factory.hpp"

#include <sdbusplus/asio/object_server.hpp>

class TriggerFactory : public interfaces::TriggerFactory
{
  public:
    TriggerFactory(std::shared_ptr<sdbusplus::asio::connection> bus,
                   std::shared_ptr<sdbusplus::asio::object_server> objServer);

    std::unique_ptr<interfaces::Trigger> make(
        const std::string& name, bool isDiscrete, bool logToJournal,
        bool logToRedfish, bool updateReport,
        const std::vector<
            std::pair<sdbusplus::message::object_path, std::string>>& sensors,
        const std::vector<std::string>& reportNames,
        const TriggerThresholdParams& thresholdParams,
        interfaces::TriggerManager& triggerManager) const override;

  private:
    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
};
