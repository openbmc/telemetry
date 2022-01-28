#pragma once

#include "interfaces/trigger.hpp"

#include <gmock/gmock.h>

class TriggerMock : public interfaces::Trigger
{
  public:
    explicit TriggerMock(std::string id)
    {
        using namespace testing;

        ON_CALL(*this, getId).WillByDefault([id] { return id; });
        ON_CALL(*this, getPath).WillByDefault([id] { return "/" + id; });
        EXPECT_CALL(*this, Die).Times(AnyNumber());
    }

    virtual ~TriggerMock()
    {
        Die();
    }

    MOCK_METHOD(std::string, getId, (), (const, override));
    MOCK_METHOD(std::string, getPath, (), (const, override));
    MOCK_METHOD(const std::vector<std::string>&, getReportIds, (),
                (const, override));
    MOCK_METHOD(void, Die, ());
};
