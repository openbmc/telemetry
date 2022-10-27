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
                "AddReport",
                [this](boost::asio::yield_context& yield, std::string reportId,
                       std::string reportName, std::string reportingType,
                       std::string reportUpdates, uint64_t appendLimit,
                       std::vector<std::string> reportActions,
                       uint64_t interval, ReadingParameters readingParameters,
                       bool enabled) {
                    if (reportingType.empty())
                    {
                        reportingType =
                            utils::enumToString(ReportingType::onRequest);
                    }

                    if (reportUpdates.empty())
                    {
                        reportUpdates =
                            utils::enumToString(ReportUpdates::overwrite);
                    }

                    if (appendLimit == std::numeric_limits<uint64_t>::max())
                    {
                        appendLimit = maxAppendLimit;
                    }

                    if (interval == std::numeric_limits<uint64_t>::max())
                    {
                        interval = 0;
                    }

                    return addReport(yield, reportId, reportName,
                                     utils::toReportingType(reportingType),
                                     utils::transform(
                                         reportActions,
                                         [](const auto& reportAction) {
                                             return utils::toReportAction(
                                                 reportAction);
                                         }),
                                     Milliseconds(interval), appendLimit,
                                     utils::toReportUpdates(reportUpdates),
                                     readingParameters, enabled)
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
        throw errors::InvalidArgument("AppendLimit", "Out of range.");
    }

    if ((reportingType == ReportingType::periodic && interval < minInterval) ||
        (reportingType != ReportingType::periodic &&
         interval != Milliseconds{0}))
    {
        throw errors::InvalidArgument("Interval");
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
        throw errors::InvalidArgument("MetricParams", "Too many.");
    }

    for (const LabeledMetricParameters& item : readingParams)
    {
        utils::toOperationType(
            utils::toUnderlying(item.at_label<ts::OperationType>()));
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
