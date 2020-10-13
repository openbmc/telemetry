#pragma once

#include "interfaces/report_manager.hpp"

#include <gmock/gmock.h>

class ReportManagerMock : public interfaces::ReportManager
{
  public:
    ReportManagerMock()
    {}

    MOCK_METHOD(void, removeReport, (const interfaces::Report*), (override));
};
