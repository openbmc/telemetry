#pragma once

#include "interfaces/report.hpp"

#include <gmock/gmock.h>

class ReportMock : public interfaces::Report
{
  public:
    ReportMock(std::string name)
    {
        using namespace testing;

        ON_CALL(*this, getName).WillByDefault([name] { return name; });
        ON_CALL(*this, getPath).WillByDefault([name] { return "/" + name; });
        EXPECT_CALL(*this, Die).Times(AnyNumber());
    }

    virtual ~ReportMock()
    {
        Die();
    }

    MOCK_METHOD(std::string, getName, (), (override, const));
    MOCK_METHOD(std::string, getPath, (), (override, const));
    MOCK_METHOD(void, updateReadings, (), (override));
    MOCK_METHOD(void, Die, ());
};
