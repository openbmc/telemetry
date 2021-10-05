#include "report_manager.hpp"

#include "report.hpp"
#include "types/generated.hpp"
#include "types/report_types.hpp"
#include "utils/conversion.hpp"
#include "utils/transform.hpp"

#include <phosphor-logging/log.hpp>
#include <sdbusplus/exception.hpp>

#include <iostream>
#include <stdexcept>
#include <system_error>

ReadingParameters
    convertToReadingParameters(ReadingParametersPastVersion params)
{
    return utils::transform(params, [](const auto& param) {
        using namespace std::chrono_literals;

        return ReadingParameters::value_type(
            std::vector{{std::get<0>(param)}}, std::get<1>(param),
            std::get<2>(param), std::get<3>(param),
            utils::enumToString(CollectionTimeScope::point), 0u);
    });
}

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
                "MaxReportNameLength", size_t{},
                sdbusplus::vtable::property_::const_,
                [](const auto&) { return maxReportNameLength; });
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
                                    ReadingParametersPastVersion metricParams) {
                    return addReport(yield, reportName, reportingType,
                                     emitsReadingsUpdate,
                                     logToMetricReportsCollection,
                                     Milliseconds(interval),
                                     convertToReadingParameters(
                                         std::move(metricParams)))
                        .getPath();
                });

            dbusIface.register_method(
                "AddReportFutureVersion",
                [this](boost::asio::yield_context& yield,
                       const std::string& reportName,
                       const std::string& reportingType,
                       const bool emitsReadingsUpdate,
                       const bool logToMetricReportsCollection,
                       const uint64_t interval,
                       ReadingParameters metricParams) {
                    return addReport(yield, reportName, reportingType,
                                     emitsReadingsUpdate,
                                     logToMetricReportsCollection,
                                     Milliseconds(interval),
                                     std::move(metricParams))
                        .getPath();
                });

            dbusIface.register_method(
                "AddReportJson", [this](boost::asio::yield_context& yield,
                                        const std::string& jsonStr) {
                    try
                    {
                        return addReport(yield, jsonStr).getPath();
                    }
                    catch (const std::exception& e)
                    {
                        std::cout << e.what() << std::endl;
                        return std::string(e.what());
                    }
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

void ReportManager::verifyReportNameLength(const std::string& reportName)
{
    if (reportName.length() > maxReportNameLength)
    {
        throw sdbusplus::exception::SdBusError(
            static_cast<int>(std::errc::invalid_argument),
            "Report name exceed maximum length");
    }
}

void ReportManager::verifyAddReport(
    const std::string& reportName, const std::string& reportingType,
    Milliseconds interval,
    const std::vector<LabeledMetricParameters>& readingParams)
{
    if (reports.size() >= maxReports)
    {
        throw sdbusplus::exception::SdBusError(
            static_cast<int>(std::errc::too_many_files_open),
            "Reached maximal report count");
    }

    verifyReportNameLength(reportName);

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
        namespace ts = utils::tstring;

        for (const LabeledMetricParameters& item : readingParams)
        {
            utils::toOperationType(
                utils::toUnderlying(item.at_label<ts::OperationType>()));
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
    const bool logToMetricReportsCollection, Milliseconds interval,
    ReadingParameters metricParams)
{
    auto labeledMetricParams =
        reportFactory->convertMetricParams(yield, metricParams);

    return addReport(reportName, reportingType, emitsReadingsUpdate,
                     logToMetricReportsCollection, interval,
                     std::move(labeledMetricParams));
}

interfaces::Report& ReportManager::addReport(boost::asio::yield_context& yield,
                                             const std::string& jsonStr)
{
    auto jsonData = nlohmann::json::parse(jsonStr);
    auto addReportArgs = jsonData.get<generated::AddReportParams>();

    // The whole addReport needs to be refactored, following lines are just
    // quick compatibility patch. In the final version it should be look more
    // like "addReport(yield, addReportArgs)":
    auto reportName = addReportArgs.reportName;
    auto reportingType =
        generated::reportingTypeToString(addReportArgs.reportingType);
    auto emitsReadingsUpdate = addReportArgs.emitsReadingsUpdate;
    auto logToMetricReportsCollection =
        addReportArgs.logToMetricReportsCollection;
    auto interval = Milliseconds(addReportArgs.interval);
    auto labeledMetricParams = reportFactory->convertMetricParams2(
        yield, addReportArgs.metricParameters);

    return addReport(reportName, reportingType, emitsReadingsUpdate,
                     logToMetricReportsCollection, interval,
                     std::move(labeledMetricParams));
}

interfaces::Report& ReportManager::addReport(
    const std::string& reportName, const std::string& reportingType,
    const bool emitsReadingsUpdate, const bool logToMetricReportsCollection,
    Milliseconds interval,
    std::vector<LabeledMetricParameters> labeledMetricParams)
{
    verifyAddReport(reportName, reportingType, interval, labeledMetricParams);

    reports.emplace_back(
        reportFactory->make(reportName, reportingType, emitsReadingsUpdate,
                            logToMetricReportsCollection, interval, *this,
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
                      logToMetricReportsCollection, Milliseconds(interval),
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
