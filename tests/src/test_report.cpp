#include "dbus_environment.hpp"
#include "telemetry.hpp"

using namespace testing;

class TestReport : public Test, public Telemetry
{
  public:
    TestReport() : Telemetry(DbusEnvironment::getBus())
    {}

    void SetUp() override
    {
        DbusEnvironment::addReport(basicArgs);
    }

    void TearDown() override
    {
        EXPECT_THAT(
            DbusEnvironment::deleteReport(basicArgs.getReportPath()).value(),
            Eq(boost::system::errc::success));
        EXPECT_THAT(reportManager.getReportsSize(), Eq(0));
    }

    DbusEnvironment::AddReportArgs basicArgs = {
        "TestReport",
        "Periodic",
        true,
        true,
        ReportManager::minInterval.count(),
        ReadingParameters{}};
    std::string reportPath = basicArgs.getReportPath();
};

TEST_F(TestReport, verifyIfPropertiesHaveValidValue)
{
    EXPECT_THAT(
        DbusEnvironment::getReportProperty<uint64_t>(reportPath, "Interval"),
        Eq(basicArgs.interval));
    EXPECT_THAT(
        DbusEnvironment::getReportProperty<bool>(reportPath, "Persistency"),
        Eq(false));
    EXPECT_THAT(DbusEnvironment::getReportProperty<std::string>(
                    reportPath, "ReportingType"),
                Eq(basicArgs.reportingType));
    EXPECT_THAT(DbusEnvironment::getReportProperty<bool>(reportPath,
                                                         "EmitsReadingsUpdate"),
                Eq(basicArgs.emitsReadingsSignal));
    EXPECT_THAT(DbusEnvironment::getReportProperty<bool>(
                    reportPath, "LogToMetricReportsCollection"),
                Eq(basicArgs.logToMetricReportsCollection));
    EXPECT_THAT(DbusEnvironment::getReportProperty<ReadingParameters>(
                    reportPath, "ReadingParameters"),
                Eq(basicArgs.readingParams));
}

TEST_F(TestReport, modifyInterval)
{
    uint64_t newValue = ReportManager::minInterval.count() + 1;
    EXPECT_THAT(
        DbusEnvironment::setReportProperty(reportPath, "Interval", newValue)
            .value(),
        Eq(boost::system::errc::success));
    EXPECT_THAT(
        DbusEnvironment::getReportProperty<uint64_t>(reportPath, "Interval"),
        Eq(newValue));
}
