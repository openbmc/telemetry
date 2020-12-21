#pragma once

#include "interfaces/trigger.hpp"

#include <gmock/gmock.h>

class TriggerMock : public interfaces::Trigger
{
  public:
    TriggerMock(std::string name)
    {
        using namespace testing;

        ON_CALL(*this, getName).WillByDefault([name] { return name; });
        ON_CALL(*this, getPath).WillByDefault([name] { return "/" + name; });
        EXPECT_CALL(*this, Die).Times(AnyNumber());
    }

    virtual ~TriggerMock()
    {
        Die();
    }

    MOCK_METHOD(std::string, getName, (), (const, override));
    MOCK_METHOD(std::string, getPath, (), (const, override));
    MOCK_METHOD(void, Die, ());
};
