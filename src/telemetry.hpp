#pragma once

#include "report_factory.hpp"
#include "report_manager.hpp"

#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

#include <memory>

class Telemetry
{
  public:
    Telemetry(std::shared_ptr<sdbusplus::asio::connection> bus) :
        objServer(std::make_shared<sdbusplus::asio::object_server>(bus)),
        reportManager(
            std::make_unique<ReportFactory>(bus->get_io_context(), objServer),
            objServer)
    {}

  private:
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    ReportManager reportManager;
};
