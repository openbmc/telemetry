#include "dbus_environment.hpp"
#include "telemetry.hpp"

using namespace testing;

class TestReportManager : public Test, public Telemetry
{
  public:
    TestReportManager() : Telemetry(DbusEnvironment::getBus())
    {}

    DbusEnvironment::AddReportArgs basicArgs = {
        "TestReport",
        "Periodic",
        true,
        true,
        ReportManager::minInterval.count(),
        ReadingParameters{}};
};

TEST_F(TestReportManager, minInterval)
{
    EXPECT_THAT(
        DbusEnvironment::getReportManagerProperty<uint64_t>("MinInterval"),
        Eq(static_cast<uint64_t>(ReportManager::minInterval.count())));
}

TEST_F(TestReportManager, maxReports)
{
    EXPECT_THAT(
        DbusEnvironment::getReportManagerProperty<uint32_t>("MaxReports"),
        Eq(ReportManager::maxReports));
}

TEST_F(TestReportManager, addReport)
{
    auto [ec, path] = DbusEnvironment::addReport(basicArgs);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    EXPECT_THAT(path, Eq(basicArgs.getReportPath()));
    EXPECT_THAT(reportManager.getReportsSize(), Eq(1));
}

TEST_F(TestReportManager, addSameReportTwoTimes)
{
    DbusEnvironment::addReport(basicArgs);

    auto [ec, path] = DbusEnvironment::addReport(basicArgs);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::file_exists));
    EXPECT_THAT(path, Eq(std::string()));
    EXPECT_THAT(reportManager.getReportsSize(), Eq(1));
}

TEST_F(TestReportManager, addReportWithInvalidInterval)
{
    DbusEnvironment::AddReportArgs args = basicArgs;
    args.interval = ReportManager::minInterval.count() - 1;

    auto [ec, path] = DbusEnvironment::addReport(args);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::invalid_argument));
    EXPECT_THAT(path, Eq(std::string()));
    EXPECT_THAT(reportManager.getReportsSize(), Eq(0));
}

TEST_F(TestReportManager, addManyReportUpToMaxReportsAndOneMore)
{
    DbusEnvironment::AddReportArgs args = basicArgs;
    for (size_t i = 0; i < ReportManager::maxReports; i++)
    {
        args.name = "TestReport" + std::to_string(i);

        auto [ec, path] = DbusEnvironment::addReport(args);
        EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
        EXPECT_THAT(path, Eq(args.getReportPath()));
        EXPECT_THAT(reportManager.getReportsSize(), Eq(i + 1));
    }

    args.name = "TestReport" + std::to_string(ReportManager::maxReports);
    auto [ec, path] = DbusEnvironment::addReport(args);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::too_many_files_open));
    EXPECT_THAT(path, Eq(std::string()));
    EXPECT_THAT(reportManager.getReportsSize(), Eq(ReportManager::maxReports));
}

TEST_F(TestReportManager, addReportAndDeleteIt)
{
    DbusEnvironment::addReport(basicArgs);
    auto ec = DbusEnvironment::deleteReport(basicArgs.getReportPath());
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
}

TEST_F(TestReportManager, deleteReportThatDoesNotExist)
{
    auto ec = DbusEnvironment::deleteReport(basicArgs.getReportPath());
    EXPECT_THAT(ec.value(), Eq(EBADR));
}

TEST_F(TestReportManager, addReportAndDeleteItTwice)
{
    DbusEnvironment::addReport(basicArgs);
    auto ec = DbusEnvironment::deleteReport(basicArgs.getReportPath());
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    ec = DbusEnvironment::deleteReport(basicArgs.getReportPath());
    EXPECT_THAT(ec.value(), Eq(EBADR));
}

class TestReportManagerValidReportNames :
    public TestReportManager,
    public WithParamInterface<const char*>
{};

INSTANTIATE_TEST_SUITE_P(ValidNames, TestReportManagerValidReportNames,
                         ValuesIn({"Valid_1", "Valid_1/Valid_2",
                                   "Valid_1/Valid_2/Valid_3"}));

TEST_P(TestReportManagerValidReportNames, addReportWithValidName)
{
    DbusEnvironment::AddReportArgs args = basicArgs;
    args.name = GetParam();
    auto [ec, path] = DbusEnvironment::addReport(args);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    EXPECT_THAT(path, Eq(args.getReportPath()));
}

class TestReportManagerInvalidReportNames :
    public TestReportManager,
    public WithParamInterface<const char*>
{};

INSTANTIATE_TEST_SUITE_P(InvalidNames, TestReportManagerInvalidReportNames,
                         ValuesIn({"/", "/Invalid", "Invalid/",
                                   "Invalid/Invalid/", "Invalid?"}));

TEST_P(TestReportManagerInvalidReportNames, addReportWithInvalidName)
{
    DbusEnvironment::AddReportArgs args = basicArgs;
    args.name = GetParam();
    auto [ec, path] = DbusEnvironment::addReport(args);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::invalid_argument));
    EXPECT_THAT(path, Eq(std::string()));
}
