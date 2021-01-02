#pragma once

#include "interfaces/trigger_action.hpp"

#include <gmock/gmock.h>

class TriggerActionMock : public interfaces::TriggerAction
{
  public:
    MOCK_METHOD(void, commit, (uint64_t, double), (override));
};
