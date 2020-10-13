#include "dbus_environment.hpp"
#include "mocks/report_manager_mock.hpp"
#include "report.hpp"

#include <sdbusplus/exception.hpp>

using namespace testing;

class TestReport : public Test
{
  public:
    DbusEnvironment::AddReportArgs args = {
        "TestReport",
        "Periodic",
        true,
        true,
        std::chrono::milliseconds{1000}.count(),
        ReadingParameters{}};

    std::unique_ptr<Report> createReportWithName(std::string name)
    {
        return std::make_unique<Report>(
            DbusEnvironment::getIoc(), DbusEnvironment::getObjServer(), name,
            args.reportingType, args.emitsReadingsSignal,
            args.logToMetricReportsCollection,
            std::chrono::milliseconds{args.interval}, args.readingParams,
            *reportManagerMock);
    }

    std::unique_ptr<ReportManagerMock> reportManagerMock =
        std::make_unique<StrictMock<ReportManagerMock>>();
    Report sut =
        Report(DbusEnvironment::getIoc(), DbusEnvironment::getObjServer(),
               args.name, args.reportingType, args.emitsReadingsSignal,
               args.logToMetricReportsCollection,
               std::chrono::milliseconds{args.interval}, args.readingParams,
               *reportManagerMock);
};

TEST_F(TestReport, verifyIfPropertiesHaveValidValue)
{
    EXPECT_THAT(
        DbusEnvironment::getReportProperty<uint64_t>(sut.getPath(), "Interval"),
        Eq(args.interval));
    EXPECT_THAT(
        DbusEnvironment::getReportProperty<bool>(sut.getPath(), "Persistency"),
        Eq(false));
    EXPECT_THAT(DbusEnvironment::getReportProperty<std::string>(
                    sut.getPath(), "ReportingType"),
                Eq(args.reportingType));
    EXPECT_THAT(DbusEnvironment::getReportProperty<bool>(sut.getPath(),
                                                         "EmitsReadingsUpdate"),
                Eq(args.emitsReadingsSignal));
    EXPECT_THAT(DbusEnvironment::getReportProperty<bool>(
                    sut.getPath(), "LogToMetricReportsCollection"),
                Eq(args.logToMetricReportsCollection));
    EXPECT_THAT(DbusEnvironment::getReportProperty<ReadingParameters>(
                    sut.getPath(), "ReadingParameters"),
                Eq(args.readingParams));
}

TEST_F(TestReport, modifyInterval)
{
    uint64_t newValue = args.interval + 1;
    EXPECT_THAT(
        DbusEnvironment::setReportProperty(sut.getPath(), "Interval", newValue)
            .value(),
        Eq(boost::system::errc::success));
    EXPECT_THAT(
        DbusEnvironment::getReportProperty<uint64_t>(sut.getPath(), "Interval"),
        Eq(newValue));
}

TEST_F(TestReport, deleteReport)
{
    EXPECT_CALL(*reportManagerMock, removeReport(&sut));
    auto ec = DbusEnvironment::deleteReport(sut.getPath());
    EXPECT_THAT(ec, Eq(boost::system::errc::success));
}

TEST_F(TestReport, deleteNonExistingReport)
{
    using namespace std::literals::string_literals;
    auto ec = DbusEnvironment::deleteReport(reportDir + "NonExisting"s);
    EXPECT_THAT(ec.value(), Eq(EBADR));
}

class TestReportValidNames :
    public TestReport,
    public WithParamInterface<const char*>
{};

INSTANTIATE_TEST_SUITE_P(ValidNames, TestReportValidNames,
                         ValuesIn({"Valid_1", "Valid_1/Valid_2",
                                   "Valid_1/Valid_2/Valid_3"}));

TEST_P(TestReportValidNames, createReportWithValidName)
{
    EXPECT_NO_THROW(createReportWithName(GetParam()));
}

class TestReportInvalidNames :
    public TestReport,
    public WithParamInterface<const char*>
{};

INSTANTIATE_TEST_SUITE_P(InvalidNames, TestReportInvalidNames,
                         ValuesIn({"/", "/Invalid", "Invalid/",
                                   "Invalid/Invalid/", "Invalid?"}));

TEST_P(TestReportInvalidNames, createReportWithInvalidName)
{
    EXPECT_THROW(createReportWithName(GetParam()),
                 sdbusplus::exception::SdBusError);
}
