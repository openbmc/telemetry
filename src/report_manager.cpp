#include "report_manager.hpp"

#include "report.hpp"

#include <phosphor-logging/log.hpp>
#include <sdbusplus/exception.hpp>

#include <stdexcept>
#include <system_error>

ReportManager::ReportManager(
    std::unique_ptr<interfaces::ReportFactory> reportFactoryIn,
    std::unique_ptr<interfaces::JsonStorage> reportStorageIn,
    const std::shared_ptr<sdbusplus::asio::object_server>& objServerIn) :
    reportFactory(std::move(reportFactoryIn)),
    reportStorage(std::move(reportStorageIn)), objServer(objServerIn)
{
    reports.reserve(maxReports);
    loadFromPersistent();

    reportManagerIface = objServer->add_unique_interface(
        reportManagerPath, reportManagerIfaceName, [this](auto& dbusIface) {
            dbusIface.register_property_r(
                "MaxReports", uint32_t{}, sdbusplus::vtable::property_::const_,
                [](const auto&) { return maxReports; });
            dbusIface.register_property_r(
                "MinInterval", uint64_t{}, sdbusplus::vtable::property_::const_,
                [](const auto&) -> uint64_t { return minInterval.count(); });

            dbusIface.register_method(
                "AddReport", [this](const std::string& reportName,
                                    const std::string& reportingType,
                                    const bool emitsReadingsUpdate,
                                    const bool logToMetricReportsCollection,
                                    const uint64_t interval,
                                    const ReadingParameters& metricParams) {
                    return addReport(reportName, reportingType,
                                     emitsReadingsUpdate,
                                     logToMetricReportsCollection, interval,
                                     metricParams)
                        ->getPath();
                });
        });
}

void ReportManager::removeReport(const interfaces::Report* report)
{
    reports.erase(
        std::remove_if(reports.begin(), reports.end(),
                       [report](const auto& x) { return report == x.get(); }),
        reports.end());
}

std::unique_ptr<interfaces::Report>& ReportManager::addReport(
    const std::string& reportName, const std::string& reportingType,
    const bool emitsReadingsUpdate, const bool logToMetricReportsCollection,
    const uint64_t interval, const ReadingParameters& metricParams)
{
    if (reports.size() >= maxReports)
    {
        throw sdbusplus::exception::SdBusError(
            static_cast<int>(std::errc::too_many_files_open),
            "Reached maximal report count");
    }

    for (const auto& report : reports)
    {
        if (report->getName() == reportName)
        {
            throw sdbusplus::exception::SdBusError(
                static_cast<int>(std::errc::file_exists), "Duplicate report");
        }
    }

    std::chrono::milliseconds reportInterval{interval};
    if (reportInterval < minInterval)
    {
        throw sdbusplus::exception::SdBusError(
            static_cast<int>(std::errc::invalid_argument), "Invalid interval");
    }

    reports.emplace_back(reportFactory->make(
        reportName, reportingType, emitsReadingsUpdate,
        logToMetricReportsCollection, std::move(reportInterval), metricParams,
        *this, *reportStorage));
    return reports.back();
}

void ReportManager::loadFromPersistent()
{
    std::vector<interfaces::JsonStorage::FilePath> paths =
        reportStorage->list();

    for (const auto& path : paths)
    {
        std::optional<nlohmann::json> data = reportStorage->load(path);
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

            addReport(name, reportingType, emitsReadingsSignal,
                      logToMetricReportsCollection, interval,
                      readingParameters);
        }
        catch (const std::exception& e)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Failed to load report from storage",
                phosphor::logging::entry(
                    "filename=",
                    static_cast<std::filesystem::path>(path).c_str()),
                phosphor::logging::entry("msg=", e.what()));
            reportStorage->remove(path);
        }
    }
}
