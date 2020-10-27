#pragma once

#include "interfaces/report_factory.hpp"
#include "mocks/report_mock.hpp"

#include <gmock/gmock.h>

class ReportFactoryMock : public interfaces::ReportFactory
{
  public:
    ReportFactoryMock()
    {
        using namespace testing;

        ON_CALL(*this, make)
            .WillByDefault(WithArgs<1>(Invoke([](const std::string& name) {
                return std::make_unique<NiceMock<ReportMock>>(name);
            })));
    }

    MOCK_METHOD(
        std::unique_ptr<interfaces::Report>, make,
        (std::optional<std::reference_wrapper<boost::asio::yield_context>>,
         const std::string& name, const std::string& reportingType,
         bool emitsReadingsSignal, bool logToMetricReportsCollection,
         std::chrono::milliseconds period,
         const ReadingParameters& metricParams,
         interfaces::ReportManager& reportManager,
         interfaces::JsonStorage& reportStorage),
        (const, override));
};
