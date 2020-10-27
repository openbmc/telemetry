#pragma once

#include "interfaces/report_factory.hpp"
#include "interfaces/sensor.hpp"

#include <boost/asio/io_context.hpp>
#include <sdbusplus/asio/object_server.hpp>

class ReportFactory : public interfaces::ReportFactory
{
  public:
    ReportFactory(
        std::shared_ptr<sdbusplus::asio::connection> bus,
        const std::shared_ptr<sdbusplus::asio::object_server>& objServer);

    std::unique_ptr<interfaces::Report> make(
        std::optional<std::reference_wrapper<boost::asio::yield_context>> yield,
        const std::string& name, const std::string& reportingType,
        bool emitsReadingsSignal, bool logToMetricReportsCollection,
        std::chrono::milliseconds period, const ReadingParameters& metricParams,
        interfaces::ReportManager& reportManager,
        interfaces::JsonStorage& reportStorage) const override;

  private:
    std::shared_ptr<sdbusplus::asio::connection> bus;
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
};
