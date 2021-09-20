#include "dbus_environment.hpp"
#include "helpers.hpp"
#include "mocks/json_storage_mock.hpp"
#include "mocks/report_factory_mock.hpp"
#include "params/report_params.hpp"
#include "report.hpp"
#include "report_manager.hpp"
#include "utils/conversion.hpp"
#include "utils/set_exception.hpp"
#include "utils/transform.hpp"

using namespace testing;
using namespace std::string_literals;
using namespace std::chrono_literals;

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

    std::unique_ptr<ReportMock> reportMockPtr =
        std::make_unique<NiceMock<ReportMock>>(reportParams.reportName());
    ReportMock& reportMock = *reportMockPtr;

    std::unique_ptr<ReportManager> sut;

    MockFunction<void(std::string)> checkPoint;

    void SetUp() override
    {
        EXPECT_CALL(reportFactoryMock, convertMetricParams(_, _))
            .Times(AnyNumber());

        sut = std::make_unique<ReportManager>(std::move(reportFactoryMockPtr),
                                              std::move(storageMockPtr),
                                              DbusEnvironment::getObjServer());
    }

    void TearDown() override
    {
        DbusEnvironment::synchronizeIoc();
    }

    std::pair<boost::system::error_code, std::string>
        addReport(const ReportParams& params)
    {
        std::promise<std::pair<boost::system::error_code, std::string>>
            addReportPromise;
        DbusEnvironment::getBus()->async_method_call(
            [&addReportPromise](boost::system::error_code ec,
                                const std::string& path) {
                addReportPromise.set_value({ec, path});
            },
            DbusEnvironment::serviceName(), ReportManager::reportManagerPath,
            ReportManager::reportManagerIfaceName, "AddReportFutureVersion",
            params.reportName(), params.reportingType(),
            params.emitReadingUpdate(), params.logToMetricReportCollection(),
            params.interval().count(), params.appendLimit(),
            params.reportUpdates(),
            toReadingParameters(params.metricParameters()));
        return DbusEnvironment::waitForFuture(addReportPromise.get_future());
    }

    template <class T>
    static T getProperty(std::string property)
    {
        auto propertyPromise = std::promise<T>();
        auto propertyFuture = propertyPromise.get_future();
        sdbusplus::asio::getProperty<T>(
            *DbusEnvironment::getBus(), DbusEnvironment::serviceName(),
            ReportManager::reportManagerPath,
            ReportManager::reportManagerIfaceName, property,
            [&propertyPromise](const boost::system::error_code& ec, T t) {
                if (ec)
                {
                    utils::setException(propertyPromise, "Get property failed");
                    return;
                }
                propertyPromise.set_value(t);
            });
        return DbusEnvironment::waitForFuture(std::move(propertyFuture));
    }

    static std::string prepareReportNameWithLength(size_t length)
    {
        std::stringstream reportNameStream;
        for (size_t i = 0; i < length; ++i)
        {
            reportNameStream << "z";
        }
        return reportNameStream.str();
    }
};

TEST_F(TestReportManager, minInterval)
{
    EXPECT_THAT(getProperty<uint64_t>("MinInterval"),
                Eq(ReportManager::minInterval.count()));
}

TEST_F(TestReportManager, maxReports)
{
    EXPECT_THAT(getProperty<size_t>("MaxReports"),
                Eq(ReportManager::maxReports));
}

TEST_F(TestReportManager, addReport)
{
    EXPECT_CALL(reportFactoryMock, convertMetricParams(_, _));
    reportFactoryMock.expectMake(reportParams, Ref(*sut), Ref(storageMock))
        .WillOnce(Return(ByMove(std::move(reportMockPtr))));

    auto [ec, path] = addReport(reportParams);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    EXPECT_THAT(path, Eq(reportMock.getPath()));
}

TEST_F(TestReportManager, addReportWithMaxLengthName)
{
    std::string reportName =
        prepareReportNameWithLength(ReportManager::maxReportNameLength);
    reportParams.reportName(reportName);
    reportFactoryMock.expectMake(reportParams, Ref(*sut), Ref(storageMock));

    auto [ec, path] = addReport(reportParams);

    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    EXPECT_THAT(path, Eq("/"s + reportName));
}

TEST_F(TestReportManager, DISABLED_failToAddReportWithTooLongName)
{
    reportFactoryMock.expectMake(std::nullopt, Ref(*sut), Ref(storageMock))
        .Times(0);

    reportParams.reportName(
        prepareReportNameWithLength(ReportManager::maxReportNameLength + 1));

    auto [ec, path] = addReport(reportParams);

    EXPECT_THAT(ec.value(), Eq(boost::system::errc::invalid_argument));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestReportManager, DISABLED_failToAddReportTwice)
{
    reportFactoryMock.expectMake(reportParams, Ref(*sut), Ref(storageMock))
        .WillOnce(Return(ByMove(std::move(reportMockPtr))));

    addReport(reportParams);

    auto [ec, path] = addReport(reportParams);

    EXPECT_THAT(ec.value(), Eq(boost::system::errc::file_exists));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestReportManager, DISABLED_failToAddReportWithInvalidInterval)
{
    reportFactoryMock.expectMake(std::nullopt, Ref(*sut), Ref(storageMock))
        .Times(0);

    reportParams.reportingType("Periodic");
    reportParams.interval(reportParams.interval() - 1ms);

    auto [ec, path] = addReport(reportParams);

    EXPECT_THAT(ec.value(), Eq(boost::system::errc::invalid_argument));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestReportManager, DISABLED_failToAddReportWithInvalidReportingType)
{
    reportFactoryMock.expectMake(std::nullopt, Ref(*sut), Ref(storageMock))
        .Times(0);

    reportParams.reportingType("Invalid");

    auto [ec, path] = addReport(reportParams);

    EXPECT_THAT(ec.value(), Eq(boost::system::errc::invalid_argument));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestReportManager, DISABLED_failToAddReportWithMoreSensorsThanExpected)
{
    reportFactoryMock.expectMake(std::nullopt, Ref(*sut), Ref(storageMock))
        .Times(0);

    auto metricParams = reportParams.metricParameters();
    for (size_t i = 0; i < ReportManager::maxReadingParams + 1; i++)
    {
        metricParams.push_back(metricParams.front());
    }
    reportParams.metricParameters(std::move(metricParams));

    auto [ec, path] = addReport(reportParams);

    EXPECT_THAT(ec.value(), Eq(boost::system::errc::argument_list_too_long));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestReportManager, DISABLED_failToAddReportWhenMaxReportIsReached)
{
    reportFactoryMock.expectMake(std::nullopt, Ref(*sut), Ref(storageMock))
        .Times(ReportManager::maxReports);

    for (size_t i = 0; i < ReportManager::maxReports; i++)
    {
        reportParams.reportName(reportParams.reportName() + std::to_string(i));

        auto [ec, path] = addReport(reportParams);
        EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    }

    reportParams.reportName(reportParams.reportName() +
                            std::to_string(ReportManager::maxReports));
    auto [ec, path] = addReport(reportParams);

    EXPECT_THAT(ec.value(), Eq(boost::system::errc::too_many_files_open));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestReportManager, removeReport)
{
    {
        InSequence seq;
        EXPECT_CALL(reportFactoryMock, convertMetricParams(_, _));
        reportFactoryMock.expectMake(reportParams, Ref(*sut), Ref(storageMock))
            .WillOnce(Return(ByMove(std::move(reportMockPtr))));
        EXPECT_CALL(reportMock, Die());
        EXPECT_CALL(checkPoint, Call("end"));
    }

    addReport(reportParams);
    sut->removeReport(&reportMock);
    checkPoint.Call("end");
}

TEST_F(TestReportManager, removingReportThatIsNotInContainerHasNoEffect)
{
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
    {
        InSequence seq;
        EXPECT_CALL(reportFactoryMock, convertMetricParams(_, _));
        reportFactoryMock.expectMake(reportParams, Ref(*sut), Ref(storageMock))
            .WillOnce(Return(ByMove(std::move(reportMockPtr))));
        EXPECT_CALL(reportMock, Die());
        EXPECT_CALL(checkPoint, Call("end"));
    }

    addReport(reportParams);
    sut->removeReport(&reportMock);
    sut->removeReport(&reportMock);
    checkPoint.Call("end");
}

TEST_F(TestReportManager, updateReportCallsUpdateReadingsForExistReport)
{
    reportFactoryMock.expectMake(reportParams, Ref(*sut), Ref(storageMock))
        .WillOnce(Return(ByMove(std::move(reportMockPtr))));
    EXPECT_CALL(reportMock, updateReadings());

    addReport(reportParams);
    sut->updateReport(reportParams.reportName());
}

TEST_F(TestReportManager, updateReportDoNothingIfReportDoesNotExist)
{
    reportFactoryMock.expectMake(reportParams, Ref(*sut), Ref(storageMock))
        .WillOnce(Return(ByMove(std::move(reportMockPtr))));
    EXPECT_CALL(reportMock, updateReadings()).Times(0);

    addReport(reportParams);
    sut->updateReport("NotAReport");
}

class TestReportManagerWithAggregationOperationType :
    public TestReportManager,
    public WithParamInterface<OperationType>
{
  public:
    OperationType operationType = GetParam();
};

INSTANTIATE_TEST_SUITE_P(_, TestReportManagerWithAggregationOperationType,
                         Values(OperationType::single, OperationType::max,
                                OperationType::min, OperationType::avg,
                                OperationType::sum));

TEST_P(TestReportManagerWithAggregationOperationType,
       addReportWithDifferentOperationTypes)
{
    reportParams.metricParameters(
        std::vector<LabeledMetricParameters>{{LabeledMetricParameters{
            {LabeledSensorParameters{"Service",
                                     "/xyz/openbmc_project/sensors/power/p1"}},
            operationType,
            "MetricId1",
            "Metadata1",
            CollectionTimeScope::point,
            CollectionDuration(Milliseconds(0u))}}});

    reportFactoryMock.expectMake(reportParams, Ref(*sut), Ref(storageMock))
        .WillOnce(Return(ByMove(std::move(reportMockPtr))));

    auto [ec, path] = addReport(reportParams);

    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    EXPECT_THAT(path, Eq("/"s + reportParams.reportName()));
}

class TestReportManagerStorage : public TestReportManager
{
  public:
    using FilePath = interfaces::JsonStorage::FilePath;
    using DirectoryPath = interfaces::JsonStorage::DirectoryPath;

    void SetUp() override
    {
        EXPECT_CALL(reportFactoryMock, convertMetricParams(_, _)).Times(0);

        ON_CALL(storageMock, list())
            .WillByDefault(Return(std::vector<FilePath>{FilePath("report1")}));
        ON_CALL(storageMock, load(FilePath("report1")))
            .WillByDefault(InvokeWithoutArgs([this] { return data; }));
    }

    void makeReportManager()
    {
        sut = std::make_unique<ReportManager>(std::move(reportFactoryMockPtr),
                                              std::move(storageMockPtr),
                                              DbusEnvironment::getObjServer());
    }

    nlohmann::json data = nlohmann::json{
        {"Version", Report::reportVersion},
        {"Name", reportParams.reportName()},
        {"ReportingType", reportParams.reportingType()},
        {"EmitsReadingsUpdate", reportParams.emitReadingUpdate()},
        {"LogToMetricReportsCollection",
         reportParams.logToMetricReportCollection()},
        {"Interval", reportParams.interval().count()},
        {"ReportUpdates", reportParams.reportUpdates()},
        {"AppendLimit", reportParams.appendLimit()},
        {"ReadingParameters", reportParams.metricParameters()}};
};

TEST_F(TestReportManagerStorage, reportManagerCtorAddReportFromStorage)
{
    reportFactoryMock.expectMake(reportParams, _, Ref(storageMock));

    makeReportManager();
}

TEST_F(TestReportManagerStorage,
       reportManagerCtorRemoveFileIfVersionDoesNotMatch)
{
    data["Version"] = Report::reportVersion - 1;

    EXPECT_CALL(storageMock, remove(FilePath("report1")));

    makeReportManager();
}

TEST_F(TestReportManagerStorage,
       reportManagerCtorRemoveFileIfIntervalHasWrongType)
{
    data["Interval"] = "1000";

    EXPECT_CALL(storageMock, remove(FilePath("report1")));

    makeReportManager();
}
