#include "report_manager.hpp"

#include "interfaces/types.hpp"
#include "report.hpp"
#include "utils/conversion.hpp"
#include "utils/transform.hpp"

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
                "MaxReports", size_t{}, sdbusplus::vtable::property_::const_,
                [](const auto&) { return maxReports; });
            dbusIface.register_property_r(
                "MinInterval", uint64_t{}, sdbusplus::vtable::property_::const_,
                [](const auto&) -> uint64_t { return minInterval.count(); });

            dbusIface.register_method(
                "AddReport", [this](boost::asio::yield_context& yield,
                                    const std::string& reportName,
                                    const std::string& reportingType,
                                    const bool emitsReadingsUpdate,
                                    const bool logToMetricReportsCollection,
                                    const uint64_t interval,
                                    ReadingParameters metricParams) {
                    return addReport(yield, reportName, reportingType,
                                     emitsReadingsUpdate,
                                     logToMetricReportsCollection,
                                     std::chrono::milliseconds(interval),
                                     std::move(metricParams))
                        .getPath();
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

void ReportManager::verifyReportNameComponentsLength(
    const std::string& reportName)
{
    size_t characterCount = 0;
    for (auto character : reportName)
    {
        if (character == '/')
        {
            characterCount = 0;
        }
        else
        {
            ++characterCount;

            if (characterCount > maxReportNameComponentLength)
            {
                throw sdbusplus::exception::SdBusError(
                    static_cast<int>(std::errc::invalid_argument),
                    "Report name component exceed maximum length");
            }
        }
    }
}

void ReportManager::verifyAddReport(const std::string& reportName,
                                    const std::string& reportingType,
                                    std::chrono::milliseconds interval,
                                    const ReadingParameters& readingParams)
{
    if (reports.size() >= maxReports)
    {
        throw sdbusplus::exception::SdBusError(
            static_cast<int>(std::errc::too_many_files_open),
            "Reached maximal report count");
    }

    verifyReportNameComponentsLength(reportName);

    for (const auto& report : reports)
    {
        if (report->getName() == reportName)
        {
            throw sdbusplus::exception::SdBusError(
                static_cast<int>(std::errc::file_exists), "Duplicate report");
        }
    }

    auto found = std::find(supportedReportingType.begin(),
                           supportedReportingType.end(), reportingType);
    if (found == supportedReportingType.end())
    {
        throw sdbusplus::exception::SdBusError(
            static_cast<int>(std::errc::invalid_argument),
            "Invalid reportingType");
    }

    if (reportingType == "Periodic" && interval < minInterval)
    {
        throw sdbusplus::exception::SdBusError(
            static_cast<int>(std::errc::invalid_argument), "Invalid interval");
    }

    if (readingParams.size() > maxReadingParams)

    {
        throw sdbusplus::exception::SdBusError(
            static_cast<int>(std::errc::argument_list_too_long),
            "Too many reading parameters");
    }

    try
    {
        for (const auto& item : readingParams)
        {
            utils::stringToOperationType(std::get<1>(item));
        }
    }
    catch (const std::exception& e)
    {
        throw sdbusplus::exception::SdBusError(
            static_cast<int>(std::errc::invalid_argument), e.what());
    }
}

interfaces::Report& ReportManager::addReport(
    boost::asio::yield_context& yield, const std::string& reportName,
    const std::string& reportingType, const bool emitsReadingsUpdate,
    const bool logToMetricReportsCollection, std::chrono::milliseconds interval,
    ReadingParameters metricParams)
{
    verifyAddReport(reportName, reportingType, interval, metricParams);

    reports.emplace_back(reportFactory->make(
        yield, reportName, reportingType, emitsReadingsUpdate,
        logToMetricReportsCollection, interval, std::move(metricParams), *this,
        *reportStorage));
    return *reports.back();
}

interfaces::Report& ReportManager::addReport(
    const std::string& reportName, const std::string& reportingType,
    const bool emitsReadingsUpdate, const bool logToMetricReportsCollection,
    std::chrono::milliseconds interval,
    std::vector<LabeledMetricParameters> labeledMetricParams)
{
    auto metricParams = utils::transform(
        labeledMetricParams, [](const LabeledMetricParameters& param) {
            using namespace utils::tstring;

            return ReadingParameters::value_type(
                sdbusplus::message::object_path(
                    param.at_index<0>().at_label<Path>()),
                utils::enumToString(param.at_index<1>()), param.at_index<2>(),
                param.at_index<3>());
        });

    verifyAddReport(reportName, reportingType, interval, metricParams);

    reports.emplace_back(reportFactory->make(
        reportName, reportingType, emitsReadingsUpdate,
        logToMetricReportsCollection, interval, std::move(metricParams), *this,
        *reportStorage, labeledMetricParams));
    return *reports.back();
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
            auto readingParameters =
                data->at("ReadingParameters")
                    .get<std::vector<LabeledMetricParameters>>();

            addReport(name, reportingType, emitsReadingsSignal,
                      logToMetricReportsCollection,
                      std::chrono::milliseconds(interval),
                      std::move(readingParameters));
        }
        catch (const std::exception& e)
        {
            phosphor::logging::log<phosphor::logging::level::ERR>(
                "Failed to load report from storage",
                phosphor::logging::entry(
                    "FILENAME=%s",
                    static_cast<std::filesystem::path>(path).c_str()),
                phosphor::logging::entry("EXCEPTION_MSG=%s", e.what()));
            reportStorage->remove(path);
        }
    }
}

void ReportManager::updateReport(const std::string& name)
{
    for (auto& report : reports)
    {
        if (report->getName() == name)
        {
            report->updateReadings();
            return;
        }
    }
}
