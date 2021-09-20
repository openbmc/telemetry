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
                                 [](const auto& sensorPath) {
                                     return LabeledSensorParameters("Service",
                                                                    sensorPath);
                                 }),
                utils::stringToOperationType(std::get<1>(params)),
                std::get<2>(params), std::get<3>(params),
                utils::stringToCollectionTimeScope(std::get<4>(params)),
                CollectionDuration(Milliseconds(std::get<5>(params))));
        });
    }

  public:
    ReportFactoryMock()
    {
        using namespace testing;

        ON_CALL(*this, convertMetricParams(_, _))
            .WillByDefault(
                WithArgs<1>(Invoke(&ReportFactoryMock::convertToLabeled)));

        ON_CALL(*this, make(A<const std::string&>(), _, _, _, _, _, _, _, _, _))
            .WillByDefault(WithArgs<0>(Invoke([](const std::string& name) {
                return std::make_unique<NiceMock<ReportMock>>(name);
            })));
    }

    MOCK_METHOD(std::vector<LabeledMetricParameters>, convertMetricParams,
                (boost::asio::yield_context&, const ReadingParameters&),
                (const, override));

    MOCK_METHOD(std::unique_ptr<interfaces::Report>, make,
                (const std::string&, const std::string&, bool, bool,
                 Milliseconds, uint64_t, const std::string&,
                 interfaces::ReportManager&, interfaces::JsonStorage&,
                 std::vector<LabeledMetricParameters>),
                (const, override));

    auto& expectMake(
        std::optional<std::reference_wrapper<const ReportParams>> paramsRef,
        const testing::Matcher<interfaces::ReportManager&>& rm,
        const testing::Matcher<interfaces::JsonStorage&>& js)
    {
        if (paramsRef)
        {
            const ReportParams& params = *paramsRef;
            return EXPECT_CALL(*this,
                               make(params.reportName(), params.reportingType(),
                                    params.emitReadingUpdate(),
                                    params.logToMetricReportCollection(),
                                    params.interval(), params.appendLimit(),
                                    params.reportUpdates(), rm, js,
                                    params.metricParameters()));
        }
        else
        {
            using testing::_;
            return EXPECT_CALL(*this, make(_, _, _, _, _, _, _, rm, js, _));
        }
    }
};
