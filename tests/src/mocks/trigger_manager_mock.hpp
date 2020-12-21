#pragma once

#include "interfaces/trigger_manager.hpp"

#include <gmock/gmock.h>

class TriggerManagerMock : public interfaces::TriggerManager
{
  public:
    MOCK_METHOD(void, removeTrigger, (const interfaces::Trigger* trigger),
                (override));
};
