#pragma once

#include "interfaces/trigger_factory.hpp"

#include <sdbusplus/asio/object_server.hpp>

#include <memory>

class TriggerFactory : public interfaces::TriggerFactory
{
  public:
    TriggerFactory(std::shared_ptr<sdbusplus::asio::connection> bus,
                   std::shared_ptr<sdbusplus::asio::object_server> objServer);

    std::unique_ptr<interfaces::Trigger>
        make(const std::string& name, const bool isDiscrete,
             const bool logToJournal, const bool logToRedfish,
             const bool updateReport,
             const std::vector<sdbusplus::message::object_path>& sensors,
             const std::vector<std::string>& reportNames,
             const TriggerThresholdParams& thresholdParams,
             interfaces::TriggerManager& triggerManager) const override;

  private:
    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
};
