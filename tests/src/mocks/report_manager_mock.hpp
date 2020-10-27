#pragma once

#include "interfaces/report_manager.hpp"
#include "mocks/json_storage_mock.hpp"

#include <gmock/gmock.h>

class ReportManagerMock : public interfaces::ReportManager
{
  public:
    ReportManagerMock()
    {
        using namespace testing;

        ON_CALL(*this, getReportStorage).WillByDefault(ReturnRef(storageMock));
    }

    MOCK_METHOD(void, removeReport, (const interfaces::Report*), (override));
    MOCK_METHOD(std::unique_ptr<interfaces::Report>&, addReport,
                (const std::string&, const std::string&, const bool, const bool,
                 const uint64_t, const ReadingParameters&),
                (override));
    MOCK_METHOD(interfaces::JsonStorage&, getReportStorage, (), (override));

    testing::NiceMock<StorageMock> storageMock;
};
