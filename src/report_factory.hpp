#pragma once

#include "interfaces/report_factory.hpp"
#include "persistent_json_storage.hpp"

#include <boost/asio/io_context.hpp>
#include <sdbusplus/asio/object_server.hpp>

class ReportFactory : public interfaces::ReportFactory
{
  public:
    ReportFactory(
        boost::asio::io_context& ioc,
        const std::shared_ptr<sdbusplus::asio::object_server>& objServer);

    std::unique_ptr<interfaces::Report>
        make(const std::string& name, const std::string& reportingType,
             bool emitsReadingsSignal, bool logToMetricReportsCollection,
             std::chrono::milliseconds period,
             const ReadingParameters& metricParams,
             interfaces::ReportManager& reportManager) override;
    void loadFromPersistent(interfaces::ReportManager&) override;

  private:
    boost::asio::io_context& ioc;
    std::shared_ptr<sdbusplus::asio::object_server> objServer;
    PersistentJsonStorage reportStorage;
};
