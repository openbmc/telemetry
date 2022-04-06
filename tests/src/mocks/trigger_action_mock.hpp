#pragma once

#include "interfaces/trigger_action.hpp"

#include <gmock/gmock.h>

class TriggerActionMock : public interfaces::TriggerAction
{
  public:
    MOCK_METHOD(void, commit,
                (const std::string&, ThresholdName, const std::string&,
                 Milliseconds, TriggerValue),
                (override));
};
