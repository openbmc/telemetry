#include "core/report_mgr.hpp"
#include "core/sensor.hpp"
#include "mocks/report.hpp"

#include "gmock/gmock.h"
#include "gtest/gtest.h"

using namespace core;
using namespace testing;

// The fixture for testing class Foo.
class ReportManagerTest : public Test
{
  protected:
    // You can do set-up work for each test here.
    ReportManagerTest() = default;

    // You can do clean-up work that doesn't throw exceptions here.
    virtual ~ReportManagerTest() = default;

    // If the constructor and destructor are not enough for setting up
    // and cleaning up each test, you can define the following methods:

    // Code here will be called immediately after the constructor (right
    // before each test).
    virtual void SetUp() override
    {}

    // Code here will be called immediately after each test (right
    // before the destructor).
    virtual void TearDown() override
    {}

    template <template <typename> class MockType>
    auto makeReport(ReportName name)
    {
        auto reportMock = std::make_shared<MockType<ReportMock>>();
        EXPECT_CALL(*reportMock, name())
            .WillRepeatedly(ReturnRefOfCopy(std::move(name)));
        return reportMock;
    }

    auto addReports(size_t amount)
    {
        core::ReportManager::ReportHandle lastAddedReport;

        for (uint32_t i = 0; i < amount; ++i)
        {
            lastAddedReport = mgr.add(makeReport<NiceMock>(
                ReportName("Domain", "Report-"s + std::to_string(i))));
        }

        return lastAddedReport;
    }

    static constexpr uint32_t maxReports = 5;
    static constexpr std::chrono::milliseconds pollRateResolution = 100ms;

    ReportManager mgr{maxReports, pollRateResolution};
};

TEST_F(ReportManagerTest, addDoesntChangeReportInAnyWay)
{
    mgr.add(makeReport<StrictMock>(ReportName("Domain", "Report")));
}

TEST_F(ReportManagerTest, checkIfAllowedToAddOnReportLimitIsReachedThrows)
{
    addReports(maxReports);

    EXPECT_ANY_THROW(
        mgr.checkIfAllowedToAdd(ReportName("Domain", "ReportOverLimit")));
}

TEST_F(ReportManagerTest, checkIfAllowedToAddOnAddingReportWithSameNameThrows)
{
    mgr.add(makeReport<StrictMock>(ReportName("Domain", "ReportWithSameName")));
    EXPECT_ANY_THROW(
        mgr.checkIfAllowedToAdd(ReportName("Domain", "ReportWithSameName")));
}

TEST_F(ReportManagerTest,
       checkIfAllowedToAddOnReportLimitIsReachedAllowsToAddReportIfOneIsRemoved)
{
    core::ReportManager::ReportHandle lastAddedReport = addReports(maxReports);

    EXPECT_ANY_THROW(
        mgr.checkIfAllowedToAdd(ReportName("Domain", "ReportOverLimit")));
    EXPECT_NO_THROW(mgr.remove(lastAddedReport));

    EXPECT_NO_THROW(mgr.checkIfAllowedToAdd(ReportName("Domain", "Report")));
}

TEST_F(ReportManagerTest,
       checkIfAllowedToAddOnReportWithSameNameAllowsToAddReportIfItIsRemoved)
{
    auto reportWithSameName = mgr.add(
        makeReport<NiceMock>(ReportName("Domain", "ReportWithSameName")));
    EXPECT_ANY_THROW(
        mgr.checkIfAllowedToAdd(ReportName("Domain", "ReportWithSameName")));
    EXPECT_NO_THROW(mgr.remove(reportWithSameName));

    EXPECT_NO_THROW(
        mgr.checkIfAllowedToAdd(ReportName("Domain", "ReportWithSameName")));
}

TEST_F(ReportManagerTest, removeStopsReportBeforeRemoving)
{
    auto reportMock = makeReport<StrictMock>(ReportName("Domain", "Report"));
    EXPECT_CALL(*reportMock, stop());

    auto reportHandle = mgr.add(std::move(reportMock));
    mgr.remove(reportHandle);
}

TEST_F(ReportManagerTest, removeNonExistingReportIsSilentlyDiscarded)
{
    core::ReportManager::ReportHandle reportHandle =
        makeReport<StrictMock>(ReportName("Domain", "report"));

    mgr.remove(reportHandle);
}

TEST_F(ReportManagerTest, maxReportsIsSameAsPassedInConstructor)
{
    EXPECT_EQ(maxReports, mgr.maxReports());
}

TEST_F(ReportManagerTest, pollRateResolutionIsSameAsPassedInConstructor)
{
    EXPECT_EQ(pollRateResolution, mgr.pollRateResolution());
}
