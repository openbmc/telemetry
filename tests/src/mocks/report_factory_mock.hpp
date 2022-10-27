#pragma once

#include "interfaces/report_factory.hpp"
#include "mocks/report_mock.hpp"
#include "params/report_params.hpp"
#include "utils/transform.hpp"

#include <gmock/gmock.h>

class ReportFactoryMock : public interfaces::ReportFactory
{
    static std::vector<LabeledMetricParameters>
        convertToLabeled(const ReadingParameters& readingParams)
    {
        return utils::transform(readingParams, [](const auto& params) {
            return LabeledMetricParameters(
                utils::transform(std::get<0>(params),
                                 [](const auto& sensorData) {
                                     return LabeledSensorInfo(
                                         "Service", std::get<0>(sensorData),
                                         std::get<1>(sensorData));
                                 }),
                utils::toOperationType(std::get<1>(params)),
                utils::toCollectionTimeScope(std::get<2>(params)),
                CollectionDuration(Milliseconds(std::get<3>(params))));
        });
    }

  public:
    ReportFactoryMock()
    {
        using namespace testing;

        ON_CALL(*this, convertMetricParams(_))
            .WillByDefault(
                WithArgs<0>(Invoke(&ReportFactoryMock::convertToLabeled)));

        ON_CALL(*this, convertMetricParams(_, _))
            .WillByDefault(
                WithArgs<1>(Invoke(&ReportFactoryMock::convertToLabeled)));

        ON_CALL(*this,
                make(A<const std::string&>(), _, _, _, _, _, _, _, _, _, _, _))
            .WillByDefault(WithArgs<0>(Invoke([](const std::string& id) {
                return std::make_unique<NiceMock<ReportMock>>(id);
            })));
    }

    MOCK_METHOD(std::vector<LabeledMetricParameters>, convertMetricParams,
                (boost::asio::yield_context&, const ReadingParameters&),
                (const, override));

    MOCK_METHOD(std::vector<LabeledMetricParameters>, convertMetricParams,
                (const ReadingParameters&), (const, override));

    MOCK_METHOD(void, updateMetrics,
                (std::vector<std::shared_ptr<interfaces::Metric>> & metrics,
                 bool enabled, const std::vector<LabeledMetricParameters>&),
                (const, override));

    MOCK_METHOD(std::unique_ptr<interfaces::Report>, make,
                (const std::string&, const std::string&, const ReportingType,
                 const std::vector<ReportAction>&, Milliseconds, uint64_t,
                 const ReportUpdates, interfaces::ReportManager&,
                 interfaces::JsonStorage&, std::vector<LabeledMetricParameters>,
                 bool, Readings),
                (const, override));

    auto& expectMake(
        std::optional<std::reference_wrapper<const ReportParams>> paramsRef,
        const testing::Matcher<interfaces::ReportManager&>& rm,
        const testing::Matcher<interfaces::JsonStorage&>& js)
    {
        using testing::_;
        if (paramsRef)
        {
            const ReportParams& params = *paramsRef;
            return EXPECT_CALL(
                *this,
                make(params.reportId(), params.reportName(),
                     params.reportingType(), params.reportActions(),
                     params.interval(), params.appendLimit(),
                     params.reportUpdates(), rm, js, params.metricParameters(),
                     params.enabled(), params.readings()));
        }
        else
        {
            return EXPECT_CALL(*this,
                               make(_, _, _, _, _, _, _, rm, js, _, _, _));
        }
    }
};
