#include "report_factory.hpp"

#include "report.hpp"

#include <phosphor-logging/log.hpp>

#include <stdexcept>

ReportFactory::ReportFactory(
    boost::asio::io_context& ioc,
    const std::shared_ptr<sdbusplus::asio::object_server>& objServer) :
    ioc(ioc),
    objServer(objServer), reportStorage(interfaces::JsonStorage::DirectoryPath(
                              "/var/lib/telemetry/Reports"))
{}

std::unique_ptr<interfaces::Report> ReportFactory::make(
    const std::string& name, const std::string& reportingType,
    bool emitsReadingsSignal, bool logToMetricReportsCollection,
    std::chrono::milliseconds period, const ReadingParameters& metricParams,
    interfaces::ReportManager& reportManager)
{
    std::vector<std::shared_ptr<interfaces::Metric>> metrics;

    return std::make_unique<Report>(
        ioc, objServer, name, reportingType, emitsReadingsSignal,
        logToMetricReportsCollection, period, metricParams, reportManager,
        std::move(metrics), reportStorage);
}

void ReportFactory::loadFromPersistent(interfaces::ReportManager& reportManager)
{
    std::vector<interfaces::JsonStorage::FilePath> paths = reportStorage.list();

    for (const auto& path : paths)
    {
        std::optional<nlohmann::json> data = reportStorage.load(path);
        try
        {
            size_t version = data->at("Version").get<size_t>();
            if (version != Report::reportVersion)
            {
                throw std::logic_error("Invalid version");
            }
            std::string& name = data->at("Name").get_ref<std::string&>();
            std::string& reportingType =
                data->at("ReportingType").get_ref<std::string&>();
            bool emitsReadingsSignal =
                data->at("EmitsReadingsUpdate").get<bool>();
            bool logToMetricReportsCollection =
                data->at("LogToMetricReportsCollection").get<bool>();
            uint64_t interval = data->at("Interval").get<uint64_t>();
            ReadingParameters readingParameters;
            for (auto& item : data->at("ReadingParameters"))
            {
                readingParameters.emplace_back(
                    ReadingParameterJson::from_json(item));
            }

            reportManager.addReport(name, reportingType, emitsReadingsSignal,
                                    logToMetricReportsCollection, interval,
                                    readingParameters);
        }
        catch (std::exception& e)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Failed to load report from storage",
                phosphor::logging::entry(
                    "filename=",
                    static_cast<std::filesystem::path>(path).c_str()),
                phosphor::logging::entry("msg=", e.what()));
            reportStorage.remove(path);
        }
    }
}
