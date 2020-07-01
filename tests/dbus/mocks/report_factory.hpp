#pragma once

#include "dbus/interfaces/report_factory.hpp"

#include <gmock/gmock.h>

namespace dbus
{
class ReportFactoryMock : interfaces::ReportFactory
{
  public:
    MOCK_CONST_METHOD8(
        make, std::tuple<std::shared_ptr<core::interfaces::Report>,
                         std::shared_ptr<dbus::Report>>(
                  const std::string&, const std::string&,
                  std::vector<std::shared_ptr<core::interfaces::Metric>>&&,
                  core::ReportingType&&, std::vector<core::ReportAction>&&,
                  std::chrono::milliseconds,
                  std::shared_ptr<sdbusplus::asio::connection>,
                  std::shared_ptr<sdbusplus::asio::object_server>));

    MOCK_CONST_METHOD9(make,
                       std::tuple<std::shared_ptr<core::interfaces::Report>,
                                  std::shared_ptr<dbus::Report>>(
                           boost::asio::yield_context&,
                           std::shared_ptr<sdbusplus::asio::connection>&,
                           const std::string&, const std::string&,
                           const std::string&, const std::vector<std::string>&,
                           const uint32_t&, const std::vector<MetricParams>&,
                           std::shared_ptr<sdbusplus::asio::object_server>));
};
} // namespace dbus
