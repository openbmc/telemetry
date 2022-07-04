#include "report_manager.hpp"

#include "report.hpp"
#include "types/report_types.hpp"
#include "utils/conversion.hpp"
#include "utils/dbus_path_utils.hpp"
#include "utils/make_id_name.hpp"
#include "utils/transform.hpp"

#include <phosphor-logging/log.hpp>
#include <sdbusplus/exception.hpp>
#include <sdbusplus/unpack_properties.hpp>

#include <optional>
#include <stdexcept>
#include <system_error>

ReadingParameters
    convertToReadingParameters(ReadingParametersPastVersion params)
{
    return utils::transform(params, [](const auto& param) {
        using namespace std::chrono_literals;

        const auto& [sensorPath, operationType, id, metadata] = param;

        return ReadingParameters::value_type(
            std::vector<
                std::tuple<sdbusplus::message::object_path, std::string>>{
                {sensorPath, metadata}},
            operationType, id, utils::enumToString(CollectionTimeScope::point),
            0u);
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
                "MinInterval", uint64_t{}, sdbusplus::vtable::property_::const_,
                [](const auto&) -> uint64_t { return minInterval.count(); });
            dbusIface.register_property_r(
                "SupportedOperationTypes", std::vector<std::string>{},
                sdbusplus::vtable::property_::const_,
                [](const auto&) -> std::vector<std::string> {
                    return utils::transform<std::vector>(
                        utils::convDataOperationType, [](const auto& item) {
                            return std::string(item.first);
                        });
                });

            dbusIface.register_method(
                "AddReport", [this](boost::asio::yield_context& yield,
                                    const std::string& reportId,
                                    const std::string& reportingType,
                                    const bool emitsReadingsUpdate,
                                    const bool logToMetricReportsCollection,
                                    const uint64_t interval,
                                    ReadingParametersPastVersion metricParams) {
                    constexpr auto enabledDefault = true;
                    constexpr uint64_t appendLimitDefault = 0;
                    constexpr ReportUpdates reportUpdatesDefault =
                        ReportUpdates::overwrite;

                    std::vector<ReportAction> reportActions;

                    if (emitsReadingsUpdate)
                    {
                        reportActions.emplace_back(
                            ReportAction::emitsReadingsUpdate);
                    }
                    if (logToMetricReportsCollection)
                    {
                        reportActions.emplace_back(
                            ReportAction::logToMetricReportsCollection);
                    }

                    return addReport(yield, reportId, reportId,
                                     utils::toReportingType(reportingType),
                                     reportActions, Milliseconds(interval),
                                     appendLimitDefault, reportUpdatesDefault,
                                     convertToReadingParameters(
                                         std::move(metricParams)),
                                     enabledDefault)
                        .getPath();
                });

            dbusIface.register_method(
                "AddReportFutureVersion",
                [this](
                    boost::asio::yield_context& yield,
                    const std::vector<
                        std::pair<std::string, AddReportFutureVersionVariant>>&
                        properties) {
                    std::optional<std::string> reportId;
                    std::optional<std::string> reportName;
                    std::optional<std::string> reportingType;
                    std::optional<std::vector<std::string>> reportActions;
                    std::optional<uint64_t> interval;
                    std::optional<uint64_t> appendLimit;
                    std::optional<std::string> reportUpdates;
                    std::optional<ReadingParameters> metricParams;
                    std::optional<bool> enabled;

                    sdbusplus::unpackProperties(
                        properties, "Id", reportId, "Name", reportName,
                        "ReportingType", reportingType, "ReportActions",
                        reportActions, "Interval", interval, "AppendLimit",
                        appendLimit, "ReportUpdates", reportUpdates,
                        "MetricParams", metricParams, "Enabled", enabled);

                    return addReport(
                               yield, reportId.value_or(""),
                               reportName.value_or(""),
                               utils::toReportingType(
                                   reportingType.value_or(utils::enumToString(
                                       ReportingType::onRequest))),
                               utils::transform(
                                   reportActions.value_or(
                                       std::vector<std::string>{}),
                                   [](const auto& reportAction) {
                                       return utils::toReportAction(
                                           reportAction);
                                   }),
                               Milliseconds(interval.value_or(0)),
                               appendLimit.value_or(0),
                               utils::toReportUpdates(
                                   reportUpdates.value_or(utils::enumToString(
                                       ReportUpdates::overwrite))),
                               metricParams.value_or(ReadingParameters{}),
                               enabled.value_or(true))
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

void ReportManager::verifyMetricParameters(
    const std::vector<LabeledMetricParameters>& readingParams)
{
    namespace ts = utils::tstring;

    for (auto readingParam : readingParams)
    {
        if (readingParam.at_label<ts::Id>().length() >
            utils::constants::maxIdNameLength)
        {
            throw sdbusplus::exception::SdBusError(
                static_cast<int>(std::errc::invalid_argument),
                "MetricId too long");
        }
    }
}

void ReportManager::verifyAddReport(
    const std::string& reportId, const std::string& reportName,
    const ReportingType reportingType, Milliseconds interval,
    const ReportUpdates reportUpdates, const uint64_t appendLimit,
    const std::vector<LabeledMetricParameters>& readingParams)
{
    namespace ts = utils::tstring;

    if (reports.size() >= maxReports)
    {
        throw sdbusplus::exception::SdBusError(
            static_cast<int>(std::errc::too_many_files_open),
            "Reached maximal report count");
    }

    if (appendLimit > maxAppendLimit &&
        appendLimit != std::numeric_limits<uint64_t>::max())
    {
        throw sdbusplus::exception::SdBusError(
            static_cast<int>(std::errc::invalid_argument),
            "Append limit out of range");
    }

    if ((reportingType == ReportingType::periodic && interval < minInterval) ||
        (reportingType != ReportingType::periodic &&
         interval != Milliseconds{0}))
    {
        throw sdbusplus::exception::SdBusError(
            static_cast<int>(std::errc::invalid_argument), "Invalid interval");
    }

    size_t metricCount = 0;
    for (auto metricParam : readingParams)
    {
        auto metricParamsVec =
            metricParam.at_label<utils::tstring::SensorPath>();
        metricCount += metricParamsVec.size();
    }

    if (readingParams.size() > maxNumberMetrics ||
        metricCount > maxNumberMetrics)
    {
        throw sdbusplus::exception::SdBusError(
            static_cast<int>(std::errc::argument_list_too_long),
            "Too many reading parameters");
    }

    verifyMetricParameters(readingParams);

    try
    {
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
    boost::asio::yield_context& yield, const std::string& reportId,
    const std::string& reportName, const ReportingType reportingType,
    const std::vector<ReportAction>& reportActions, Milliseconds interval,
    const uint64_t appendLimit, const ReportUpdates reportUpdates,
    ReadingParameters metricParams, const bool enabled)
{
    auto labeledMetricParams =
        reportFactory->convertMetricParams(yield, metricParams);

    return addReport(reportId, reportName, reportingType, reportActions,
                     interval, appendLimit, reportUpdates,
                     std::move(labeledMetricParams), enabled, Readings{});
}

interfaces::Report& ReportManager::addReport(
    const std::string& reportId, const std::string& reportName,
    const ReportingType reportingType,
    const std::vector<ReportAction>& reportActions, Milliseconds interval,
    const uint64_t appendLimit, const ReportUpdates reportUpdates,
    std::vector<LabeledMetricParameters> labeledMetricParams,
    const bool enabled, Readings readings)
{
    const auto existingReportIds = utils::transform(
        reports, [](const auto& report) { return report->getId(); });

    auto [id, name] = utils::makeIdName(reportId, reportName, reportNameDefault,
                                        existingReportIds);

    verifyAddReport(id, name, reportingType, interval, reportUpdates,
                    appendLimit, labeledMetricParams);

    reports.emplace_back(
        reportFactory->make(id, name, reportingType, reportActions, interval,
                            appendLimit, reportUpdates, *this, *reportStorage,
                            labeledMetricParams, enabled, std::move(readings)));
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
            bool enabled = data->at("Enabled").get<bool>();
            std::string& id = data->at("Id").get_ref<std::string&>();
            std::string& name = data->at("Name").get_ref<std::string&>();

            uint32_t reportingType = data->at("ReportingType").get<uint32_t>();
            std::vector<ReportAction> reportActions = utils::transform(
                data->at("ReportActions").get<std::vector<uint32_t>>(),
                [](const auto reportAction) {
                    return utils::toReportAction(reportAction);
                });
            uint64_t interval = data->at("Interval").get<uint64_t>();
            uint64_t appendLimit = data->at("AppendLimit").get<uint64_t>();
            uint32_t reportUpdates = data->at("ReportUpdates").get<uint32_t>();
            auto readingParameters =
                data->at("ReadingParameters")
                    .get<std::vector<LabeledMetricParameters>>();

            Readings readings = {};

            if (auto it = data->find("MetricValues"); it != data->end())
            {
                const auto labeledReadings = it->get<LabeledReadings>();
                readings = utils::toReadings(labeledReadings);
            }

            addReport(id, name, utils::toReportingType(reportingType),
                      reportActions, Milliseconds(interval), appendLimit,
                      utils::toReportUpdates(reportUpdates),
                      std::move(readingParameters), enabled,
                      std::move(readings));
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
