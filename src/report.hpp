#pragma once

#include "utils/dbus_interface.hpp"

#include <sdbusplus/asio/object_server.hpp>

#include <chrono>
#include <memory>

using Readings = std::tuple<
    uint64_t,
    std::vector<std::tuple<std::string, std::string, double, uint64_t>>>;
using ReadingParameters =
    std::vector<std::tuple<std::vector<sdbusplus::message::object_path>,
                           std::string, std::string, std::string>>;

class Report : std::enable_shared_from_this<Report>
{
  public:
    Report(const std::shared_ptr<sdbusplus::asio::object_server>& objServer,
           const std::string& domain, const std::string& name,
           const std::chrono::milliseconds&& scanPeriod);

    std::string path;

  private:
    std::chrono::milliseconds scanPeriod;
    utils::DbusInterface reportIntf;
};
