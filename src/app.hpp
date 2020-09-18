#pragma once

#include "report_manager.hpp"

#include <sdbusplus/asio/connection.hpp>
#include <sdbusplus/asio/object_server.hpp>

class App
{
  public:
    App(std::shared_ptr<sdbusplus::asio::connection> bus);

  private:
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    ReportManager reportManager;
};
