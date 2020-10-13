#include "dbus_environment.hpp"
#include "mocks/report_factory_mock.hpp"
#include "report_manager.hpp"

using namespace testing;

class TestReportManager : public Test
{
  public:
    std::string defaultReportName = "TestReport";
    std::string defaultReportType = "Periodic";
    bool defaultEmitReadingSignal = true;
    bool defaultLogToMetricReportCollection = true;
    uint64_t defaultInterval = ReportManager::minInterval.count();
    ReadingParameters defaultReadingParams = {};

    std::unique_ptr<ReportFactoryMock> reportFactoryMockPtr =
        std::make_unique<StrictMock<ReportFactoryMock>>();
    ReportFactoryMock& reportFactoryMock = *reportFactoryMockPtr;
    ReportManager sut = ReportManager(std::move(reportFactoryMockPtr),
                                      DbusEnvironment::getObjServer());

    MockFunction<void(std::string)> checkPoint;

    std::pair<boost::system::error_code, std::string>
        addReport(const std::string& reportName,
                  uint64_t interval = ReportManager::minInterval.count())
    {
        std::promise<std::pair<boost::system::error_code, std::string>>
            addReportPromise;
        DbusEnvironment::getBus()->async_method_call(
            [&addReportPromise](boost::system::error_code ec,
                                const std::string& path) {
                addReportPromise.set_value({ec, path});
            },
            DbusEnvironment::serviceName(), reportManagerPath,
            reportManagerIfaceName, "AddReport", reportName, defaultReportType,
            defaultEmitReadingSignal, defaultLogToMetricReportCollection,
            interval, defaultReadingParams);
        return addReportPromise.get_future().get();
    }

    template <class T>
    static T getProperty(std::string property)
    {
        std::promise<T> propertyPromise;
        sdbusplus::asio::getProperty<T>(
            *DbusEnvironment::getBus(), DbusEnvironment::serviceName(),
            reportManagerPath, reportManagerIfaceName, property,
            [&propertyPromise](boost::system::error_code ec) {
                EXPECT_THAT(static_cast<bool>(ec), ::testing::Eq(false));
                propertyPromise.set_value(T{});
            },
            [&propertyPromise](T t) { propertyPromise.set_value(t); });
        return propertyPromise.get_future().get();
    }
};

TEST_F(TestReportManager, minInterval)
{
    EXPECT_THAT(getProperty<uint64_t>("MinInterval"),
                Eq(static_cast<uint64_t>(ReportManager::minInterval.count())));
}

TEST_F(TestReportManager, maxReports)
{
    EXPECT_THAT(getProperty<uint32_t>("MaxReports"),
                Eq(ReportManager::maxReports));
}

TEST_F(TestReportManager, addReport)
{
    auto reportMockPtr =
        std::make_unique<NiceMock<ReportMock>>(defaultReportName);
    auto& reportMock = *reportMockPtr;

    EXPECT_CALL(reportFactoryMock,
                make(defaultReportName, defaultReportType,
                     defaultEmitReadingSignal,
                     defaultLogToMetricReportCollection,
                     std::chrono::milliseconds{defaultInterval},
                     defaultReadingParams, Ref(sut)))
        .WillOnce(Return(ByMove(std::move(reportMockPtr))));

    auto [ec, path] = addReport(defaultReportName);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    EXPECT_THAT(path, Eq(reportMock.getPath()));
}

TEST_F(TestReportManager, failToAddReportTwice)
{
    EXPECT_CALL(reportFactoryMock, make(_, _, _, _, _, _, _));

    addReport(defaultReportName);

    auto [ec, path] = addReport(defaultReportName);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::file_exists));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestReportManager, failToAddReportWithInvalidInterval)
{
    EXPECT_CALL(reportFactoryMock, make(_, _, _, _, _, _, _)).Times(0);

    uint64_t interval = defaultInterval - 1;

    auto [ec, path] = addReport(defaultReportName, interval);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::invalid_argument));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestReportManager, failToAddReportWhenMaxReportIsReached)
{
    EXPECT_CALL(reportFactoryMock, make(_, _, _, _, _, _, _))
        .Times(ReportManager::maxReports);

    for (size_t i = 0; i < ReportManager::maxReports; i++)
    {
        std::string reportName = defaultReportName + std::to_string(i);

        auto [ec, path] = addReport(reportName);
        EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    }

    std::string reportName =
        defaultReportName + std::to_string(ReportManager::maxReports);
    auto [ec, path] = addReport(reportName);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::too_many_files_open));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestReportManager, removeReport)
{
    auto reportMockPtr =
        std::make_unique<NiceMock<ReportMock>>(defaultReportName);
    auto& reportMock = *reportMockPtr;

    {
        InSequence seq;
        EXPECT_CALL(reportFactoryMock, make(_, _, _, _, _, _, _))
            .WillOnce(Return(ByMove(std::move(reportMockPtr))));
        EXPECT_CALL(reportMock, Die());
        EXPECT_CALL(checkPoint, Call("end"));
    }

    addReport(defaultReportName);
    sut.removeReport(&reportMock);
    checkPoint.Call("end");
}

TEST_F(TestReportManager, removingReportThatIsNotInContainerHasNoEffect)
{
    auto reportMockPtr =
        std::make_unique<NiceMock<ReportMock>>(defaultReportName);
    auto& reportMock = *reportMockPtr;

    {
        InSequence seq;
        EXPECT_CALL(checkPoint, Call("end"));
        EXPECT_CALL(reportMock, Die());
    }

    sut.removeReport(&reportMock);
    checkPoint.Call("end");
}

TEST_F(TestReportManager, removingSameReportTwiceHasNoSideEffect)
{
    auto reportMockPtr =
        std::make_unique<NiceMock<ReportMock>>(defaultReportName);
    auto& reportMock = *reportMockPtr;

    {
        InSequence seq;
        EXPECT_CALL(reportFactoryMock,
                    make(defaultReportName, _, _, _, _, _, _))
            .WillOnce(Return(ByMove(std::move(reportMockPtr))));
        EXPECT_CALL(reportMock, Die());
        EXPECT_CALL(checkPoint, Call("end"));
    }

    addReport(defaultReportName);
    sut.removeReport(&reportMock);
    sut.removeReport(&reportMock);
    checkPoint.Call("end");
}
