#pragma once

#include "core/interfaces/report.hpp"

#include <gmock/gmock.h>

namespace core
{

class ReportMock : public core::interfaces::Report
{
  public:
    ReportMock()
    {
        using namespace testing;

        ON_CALL(*this, reportAction())
            .WillByDefault(ReturnRefOfCopy(std::vector<ReportAction>{}));
    }

    MOCK_METHOD0(update, void());
    MOCK_METHOD1(setCallback, void(ReadingsUpdated&&));
    MOCK_CONST_METHOD0(name, const ReportName&());
    MOCK_CONST_METHOD0(reportingType, ReportingType());
    MOCK_CONST_METHOD0(reportAction, const std::vector<ReportAction>&());
    MOCK_CONST_METHOD0(timestamp, std::time_t());
    MOCK_CONST_METHOD0(scanPeriod, std::chrono::milliseconds());
    MOCK_CONST_METHOD0(
        metrics,
        boost::beast::span<const std::shared_ptr<interfaces::Metric>>());
    MOCK_CONST_METHOD0(readings, std::vector<Reading>());
    MOCK_METHOD0(stop, void());
};

} // namespace core
