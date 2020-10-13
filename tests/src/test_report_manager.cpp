#include "dbus_environment.hpp"
#include "mocks/report_factory_mock.hpp"
#include "report_manager.hpp"

using namespace testing;

class TestReportManager : public Test
{
  public:
    DbusEnvironment::AddReportArgs basicArgs = {
        "TestReport",
        "Periodic",
        true,
        true,
        ReportManager::minInterval.count(),
        ReadingParameters{}};

    std::unique_ptr<ReportFactoryMock> reportFactoryMockPtr =
        std::make_unique<StrictMock<ReportFactoryMock>>();
    ReportFactoryMock& reportFactoryMock = *reportFactoryMockPtr;
    ReportManager sut = ReportManager(std::move(reportFactoryMockPtr),
                                      DbusEnvironment::getObjServer());

    MockFunction<void(std::string)> checkPoint;
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
    EXPECT_CALL(reportFactoryMock,
                make(basicArgs.name, basicArgs.reportingType,
                     basicArgs.emitsReadingsSignal,
                     basicArgs.logToMetricReportsCollection,
                     std::chrono::milliseconds{basicArgs.interval},
                     basicArgs.readingParams, _))
        .Times(1);

    auto [ec, path] = DbusEnvironment::addReport(basicArgs);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    EXPECT_THAT(path, Eq(basicArgs.getReportPath()));
}

TEST_F(TestReportManager, addSameReportTwoTimes)
{
    EXPECT_CALL(reportFactoryMock, make(_, _, _, _, _, _, _)).Times(1);

    DbusEnvironment::addReport(basicArgs);

    auto [ec, path] = DbusEnvironment::addReport(basicArgs);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::file_exists));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestReportManager, addReportWithInvalidInterval)
{
    EXPECT_CALL(reportFactoryMock, make(_, _, _, _, _, _, _)).Times(0);

    DbusEnvironment::AddReportArgs args = basicArgs;
    args.interval = ReportManager::minInterval.count() - 1;

    auto [ec, path] = DbusEnvironment::addReport(args);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::invalid_argument));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestReportManager, addManyReportUpToMaxReportsAndOneMore)
{
    EXPECT_CALL(reportFactoryMock, make(_, _, _, _, _, _, _))
        .Times(ReportManager::maxReports);

    DbusEnvironment::AddReportArgs args = basicArgs;
    for (size_t i = 0; i < ReportManager::maxReports; i++)
    {
        args.name = "TestReport" + std::to_string(i);

        auto [ec, path] = DbusEnvironment::addReport(args);
        EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
        EXPECT_THAT(path, Eq(args.getReportPath()));
    }

    args.name = "TestReport" + std::to_string(ReportManager::maxReports);
    auto [ec, path] = DbusEnvironment::addReport(args);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::too_many_files_open));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestReportManager, removeReport)
{
    auto reportMockPtr = std::make_unique<NiceMock<ReportMock>>(basicArgs.name);
    auto& reportMock = *reportMockPtr;

    {
        InSequence seq;
        EXPECT_CALL(reportFactoryMock, make(basicArgs.name, _, _, _, _, _, _))
            .WillOnce(Return(ByMove(std::move(reportMockPtr))));
        EXPECT_CALL(reportMock, Die()).Times(1);
        EXPECT_CALL(checkPoint, Call("end"));
    }

    DbusEnvironment::addReport(basicArgs);
    sut.removeReport(&reportMock);
    checkPoint.Call("end");
}

TEST_F(TestReportManager, removeNonExistingReport)
{
    auto reportMockPtr = std::make_unique<NiceMock<ReportMock>>(basicArgs.name);
    auto& reportMock = *reportMockPtr;

    {
        InSequence seq;
        EXPECT_CALL(reportMock, Die()).Times(0);
        EXPECT_CALL(checkPoint, Call("end"));
    }

    sut.removeReport(&reportMock);
    checkPoint.Call("end");
}

TEST_F(TestReportManager, removeReportTwice)
{
    auto reportMockPtr = std::make_unique<NiceMock<ReportMock>>(basicArgs.name);
    auto& reportMock = *reportMockPtr;

    {
        InSequence seq;
        EXPECT_CALL(reportFactoryMock, make(basicArgs.name, _, _, _, _, _, _))
            .WillOnce(Return(ByMove(std::move(reportMockPtr))));
        EXPECT_CALL(reportMock, Die()).Times(1);
        EXPECT_CALL(checkPoint, Call("end"));
    }

    DbusEnvironment::addReport(basicArgs);
    sut.removeReport(&reportMock);
    sut.removeReport(&reportMock);
    checkPoint.Call("end");
}
