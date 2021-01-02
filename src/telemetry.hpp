#pragma once

#include "persistent_json_storage.hpp"
#include "report_factory.hpp"
#include "report_manager.hpp"
#include "sensor_cache.hpp"
#include "trigger_factory.hpp"
#include "trigger_manager.hpp"

#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <memory>

class Telemetry
{
  public:
    Telemetry(std::shared_ptr<sdbusplus::asio::connection> bus) :
        objServer(std::make_shared<sdbusplus::asio::object_server>(bus)),
        reportManager(
            std::make_unique<ReportFactory>(bus, objServer, sensorCache),
            std::make_unique<PersistentJsonStorage>(
                interfaces::JsonStorage::DirectoryPath(
                    "/var/lib/telemetry/Reports")),
            objServer),
        triggerManager(
            std::make_unique<TriggerFactory>(bus, objServer, sensorCache),
            objServer)
    {}

  private:
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    mutable SensorCache sensorCache;
    ReportManager reportManager;
    TriggerManager triggerManager;
};
