#pragma once

#include "interfaces/report_manager.hpp"

#include <gmock/gmock.h>

class ReportManagerMock : public interfaces::ReportManager
{
  public:
    ReportManagerMock()
    {}

    MOCK_METHOD(void, removeReport, (const interfaces::Report*), (override));
    MOCK_METHOD(std::unique_ptr<interfaces::Report>&, addReport,
                (const std::string&, const std::string&, const bool, const bool,
                 const uint64_t, const ReadingParameters&),
                (override));
};
