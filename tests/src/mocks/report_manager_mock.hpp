#pragma once

#include "interfaces/report_manager.hpp"

#include <gmock/gmock.h>

class ReportManagerMock : public interfaces::ReportManager
{
  public:
    MOCK_METHOD(void, removeReport, (const interfaces::Report*), (override));
    MOCK_METHOD(void, updateReport, (const std::string& name), (override));
    MOCK_METHOD(void, updateTriggerIds,
                (const std::string& reportId, const std::string& triggerId,
                 TriggerIdUpdate updateType),
                (override));
};
