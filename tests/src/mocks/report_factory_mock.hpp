#pragma once

#include "interfaces/report_factory.hpp"
#include "mocks/report_mock.hpp"
#include "params/report_params.hpp"

#include <gmock/gmock.h>

class ReportFactoryMock : public interfaces::ReportFactory
{
  public:
    ReportFactoryMock()
    {
        using namespace testing;

        ON_CALL(*this,
                make(A<boost::asio::yield_context&>(), _, _, _, _, _, _, _, _))
            .WillByDefault(WithArgs<1>(Invoke([](const std::string& name) {
                return std::make_unique<NiceMock<ReportMock>>(name);
            })));
        ON_CALL(*this, make(A<const std::string&>(), _, _, _, _, _, _, _, _))
            .WillByDefault(WithArgs<0>(Invoke([](const std::string& name) {
                return std::make_unique<NiceMock<ReportMock>>(name);
            })));
    }

    MOCK_METHOD(std::unique_ptr<interfaces::Report>, make,
                (boost::asio::yield_context&, const std::string&,
                 const std::string&, bool, bool, std::chrono::milliseconds,
                 const ReadingParameters&, interfaces::ReportManager&,
                 interfaces::JsonStorage&),
                (const, override));
    MOCK_METHOD(std::unique_ptr<interfaces::Report>, make,
                (const std::string&, const std::string&, bool, bool,
                 std::chrono::milliseconds, const ReadingParameters&,
                 interfaces::ReportManager&, interfaces::JsonStorage&,
                 std::vector<LabeledMetricParameters>),
                (const, override));

    auto& expectMake(
        const testing::Matcher<boost::asio::yield_context&>& yield,
        std::optional<std::reference_wrapper<const ReportParams>> paramsRef,
        const testing::Matcher<interfaces::ReportManager&>& rm,
        const testing::Matcher<interfaces::JsonStorage&>& js)
    {
        if (paramsRef)
        {
            const ReportParams& params = *paramsRef;
            return EXPECT_CALL(*this, make(yield, params.reportName(),
                                           params.reportingType(),
                                           params.emitReadingUpdate(),
                                           params.logToMetricReportCollection(),
                                           params.interval(),
                                           params.readingParameters(), rm, js));
        }
        else
        {
            using testing::_;
            return EXPECT_CALL(*this, make(yield, _, _, _, _, _, _, rm, js));
        }
    }

    auto& expectMake(
        std::optional<std::reference_wrapper<const ReportParams>> paramsRef,
        const testing::Matcher<interfaces::ReportManager&>& rm,
        const testing::Matcher<interfaces::JsonStorage&>& js,
        const testing::Matcher<std::vector<LabeledMetricParameters>>& lrp)
    {
        if (paramsRef)
        {
            const ReportParams& params = *paramsRef;
            return EXPECT_CALL(*this,
                               make(params.reportName(), params.reportingType(),
                                    params.emitReadingUpdate(),
                                    params.logToMetricReportCollection(),
                                    params.interval(),
                                    params.readingParameters(), rm, js, lrp));
        }
        else
        {
            using testing::_;
            return EXPECT_CALL(*this, make(_, _, _, _, _, _, rm, js, lrp));
        }
    }
};
