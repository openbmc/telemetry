#include "dbus_environment.hpp"
#include "mocks/json_storage_mock.hpp"
#include "mocks/report_factory_mock.hpp"
#include "params/report_params.hpp"
#include "report.hpp"
#include "report_manager.hpp"

using namespace testing;

class TestReportManager : public Test
{
  public:
    ReportParams reportParams;

    std::unique_ptr<ReportFactoryMock> reportFactoryMockPtr =
        std::make_unique<StrictMock<ReportFactoryMock>>();
    ReportFactoryMock& reportFactoryMock = *reportFactoryMockPtr;

    std::unique_ptr<StorageMock> storageMockPtr =
        std::make_unique<NiceMock<StorageMock>>();
    StorageMock& storageMock = *storageMockPtr;

    std::unique_ptr<ReportManager> sut;

    MockFunction<void(std::string)> checkPoint;

    void SetUp() override
    {
        sut = std::make_unique<ReportManager>(std::move(reportFactoryMockPtr),
                                              std::move(storageMockPtr),
                                              DbusEnvironment::getObjServer());
    }

    void TearDown() override
    {
        DbusEnvironment::synchronizeIoc();
    }

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
            DbusEnvironment::serviceName(), ReportManager::reportManagerPath,
            ReportManager::reportManagerIfaceName, "AddReport", reportName,
            reportParams.reportingType(), reportParams.emitReadingSignal(),
            reportParams.logToMetricReportCollection(), interval,
            reportParams.readingParameters());
        return DbusEnvironment::waitForFuture(addReportPromise.get_future())
            .value_or(std::pair<boost::system::error_code, std::string>{});
    }

    template <class T>
    static T getProperty(std::string property)
    {
        std::promise<T> propertyPromise;
        sdbusplus::asio::getProperty<T>(
            *DbusEnvironment::getBus(), DbusEnvironment::serviceName(),
            ReportManager::reportManagerPath,
            ReportManager::reportManagerIfaceName, property,
            [&propertyPromise](boost::system::error_code ec) {
                EXPECT_THAT(static_cast<bool>(ec), ::testing::Eq(false));
                propertyPromise.set_value(T{});
            },
            [&propertyPromise](T t) { propertyPromise.set_value(t); });
        return DbusEnvironment::waitForFuture(propertyPromise.get_future())
            .value_or(T{});
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
        std::make_unique<NiceMock<ReportMock>>(reportParams.reportName());
    auto& reportMock = *reportMockPtr;

    EXPECT_CALL(reportFactoryMock,
                make(reportParams.reportName(), reportParams.reportingType(),
                     reportParams.emitReadingSignal(),
                     reportParams.logToMetricReportCollection(),
                     std::chrono::milliseconds{reportParams.interval()},
                     reportParams.readingParameters(), Ref(*sut)))
        .WillOnce(Return(ByMove(std::move(reportMockPtr))));

    auto [ec, path] = addReport(reportParams.reportName());
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    EXPECT_THAT(path, Eq(reportMock.getPath()));
}

TEST_F(TestReportManager, failToAddReportTwice)
{
    EXPECT_CALL(reportFactoryMock, make(_, _, _, _, _, _, _));

    addReport(reportParams.reportName());

    auto [ec, path] = addReport(reportParams.reportName());
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::file_exists));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestReportManager, failToAddReportWithInvalidInterval)
{
    EXPECT_CALL(reportFactoryMock, make(_, _, _, _, _, _, _)).Times(0);

    uint64_t interval = reportParams.interval() - 1;

    auto [ec, path] = addReport(reportParams.reportName(), interval);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::invalid_argument));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestReportManager, failToAddReportWhenMaxReportIsReached)
{
    EXPECT_CALL(reportFactoryMock, make(_, _, _, _, _, _, _))
        .Times(ReportManager::maxReports);

    for (size_t i = 0; i < ReportManager::maxReports; i++)
    {
        std::string reportName = reportParams.reportName() + std::to_string(i);

        auto [ec, path] = addReport(reportName);
        EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    }

    std::string reportName =
        reportParams.reportName() + std::to_string(ReportManager::maxReports);
    auto [ec, path] = addReport(reportName);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::too_many_files_open));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestReportManager, removeReport)
{
    auto reportMockPtr =
        std::make_unique<NiceMock<ReportMock>>(reportParams.reportName());
    auto& reportMock = *reportMockPtr;

    {
        InSequence seq;
        EXPECT_CALL(reportFactoryMock, make(_, _, _, _, _, _, _))
            .WillOnce(Return(ByMove(std::move(reportMockPtr))));
        EXPECT_CALL(reportMock, Die());
        EXPECT_CALL(checkPoint, Call("end"));
    }

    addReport(reportParams.reportName());
    sut->removeReport(&reportMock);
    checkPoint.Call("end");
}

TEST_F(TestReportManager, removingReportThatIsNotInContainerHasNoEffect)
{
    auto reportMockPtr =
        std::make_unique<NiceMock<ReportMock>>(reportParams.reportName());
    auto& reportMock = *reportMockPtr;

    {
        InSequence seq;
        EXPECT_CALL(checkPoint, Call("end"));
        EXPECT_CALL(reportMock, Die());
    }

    sut->removeReport(&reportMock);
    checkPoint.Call("end");
}

TEST_F(TestReportManager, removingSameReportTwiceHasNoSideEffect)
{
    auto reportMockPtr =
        std::make_unique<NiceMock<ReportMock>>(reportParams.reportName());
    auto& reportMock = *reportMockPtr;

    {
        InSequence seq;
        EXPECT_CALL(reportFactoryMock,
                    make(reportParams.reportName(), _, _, _, _, _, _))
            .WillOnce(Return(ByMove(std::move(reportMockPtr))));
        EXPECT_CALL(reportMock, Die());
        EXPECT_CALL(checkPoint, Call("end"));
    }

    addReport(reportParams.reportName());
    sut->removeReport(&reportMock);
    sut->removeReport(&reportMock);
    checkPoint.Call("end");
}

class TestReportManagerStorage : public TestReportManager
{
  public:
    using FilePath = interfaces::JsonStorage::FilePath;
    using DirectoryPath = interfaces::JsonStorage::DirectoryPath;

    void SetUp() override
    {}

    void makeReportManager()
    {
        sut = std::make_unique<ReportManager>(std::move(reportFactoryMockPtr),
                                              std::move(storageMockPtr),
                                              DbusEnvironment::getObjServer());
    }
};

TEST_F(TestReportManagerStorage, reportManagerCtorListsAllStoredReports)
{
    EXPECT_CALL(storageMock, list);

    makeReportManager();
}

TEST_F(TestReportManagerStorage, reportManagerCtorAddReportFromStorage)
{

    {
        InSequence seq;
        EXPECT_CALL(storageMock, list)
            .WillOnce(Return(std::vector<FilePath>{FilePath("report1")}));
        EXPECT_CALL(storageMock, load).WillOnce(Return(reportParams.getJson()));
        EXPECT_CALL(reportFactoryMock,
                    make(reportParams.reportName(), _, _, _, _, _, _));
    }

    makeReportManager();
}

TEST_F(TestReportManagerStorage, reportManagerCtorRemoveFileIfVersionDoesNotMatch)
{
    nlohmann::json data = reportParams.getJson();
    data["Version"] = Report::reportVersion - 1;

    {
        InSequence seq;
        EXPECT_CALL(storageMock, list)
            .WillOnce(Return(std::vector<FilePath>{FilePath("report1")}));
        EXPECT_CALL(storageMock, load).WillOnce(Return(data));
        EXPECT_CALL(storageMock, remove);
    }

    makeReportManager();
}

TEST_F(TestReportManagerStorage, reportManagerCtorRemoveFileIfIntervalHasWrongType)
{
    nlohmann::json data = reportParams.getJson();
    data["Interval"] = "1000";

    {
        InSequence seq;
        EXPECT_CALL(storageMock, list)
            .WillOnce(Return(std::vector<FilePath>{FilePath("report1")}));
        EXPECT_CALL(storageMock, load).WillOnce(Return(data));
        EXPECT_CALL(storageMock, remove);
    }

    makeReportManager();
}
