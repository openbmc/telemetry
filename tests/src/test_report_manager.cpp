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
            ReportManager::reportManagerIfaceName, "AddReport",
            params.reportName(), params.reportingType(),
            params.emitReadingUpdate(), params.logToMetricReportCollection(),
            static_cast<uint64_t>(params.interval().count()),
            params.readingParameters());
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
            [&propertyPromise](boost::system::error_code ec, T t) {
                if (ec)
                {
                    utils::setException(propertyPromise, "GetProperty failed");
                    return;
                }
                propertyPromise.set_value(t);
            });
        return DbusEnvironment::waitForFuture(std::move(propertyFuture));
    }
};

TEST_F(TestReportManager, minInterval)
{
    EXPECT_THAT(getProperty<uint64_t>("MinInterval"),
                Eq(static_cast<uint64_t>(ReportManager::minInterval.count())));
}

TEST_F(TestReportManager, maxReports)
{
    EXPECT_THAT(getProperty<size_t>("MaxReports"),
                Eq(ReportManager::maxReports));
}

TEST_F(TestReportManager, addReport)
{
    reportFactoryMock.expectMake(_, reportParams, Ref(*sut), Ref(storageMock))
        .WillOnce(Return(ByMove(std::move(reportMockPtr))));

    auto [ec, path] = addReport(reportParams);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    EXPECT_THAT(path, Eq(reportMock.getPath()));
}

TEST_F(TestReportManager, addReportWithMaxLengthName)
{
    std::stringstream reportNameStream;
    std::stringstream pathStream;

    for (size_t i = 0; i < ReportManager::maxReportNameLength; ++i)
    {
        reportNameStream << "z";
    }
    reportParams.reportName(reportNameStream.str());
    reportFactoryMock.expectMake(_, reportParams, Ref(*sut), Ref(storageMock));

    auto [ec, path] = addReport(reportParams);

    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    EXPECT_THAT(path, Eq("/"s + reportNameStream.str()));
}

TEST_F(TestReportManager, DISABLED_failToAddReportWithTooLongName)
{
    reportFactoryMock.expectMake(_, std::nullopt, Ref(*sut), Ref(storageMock))
        .Times(0);
    reportFactoryMock.expectMake(std::nullopt, Ref(*sut), Ref(storageMock), _)
        .Times(0);
    std::stringstream reportNameStream;

    for (size_t i = 0; i < ReportManager::maxReportNameLength + 1; ++i)
    {
        reportNameStream << "z";
    }

    reportParams.reportName(reportNameStream.str());

    auto [ec, path] = addReport(reportParams);

    EXPECT_THAT(ec.value(), Eq(boost::system::errc::invalid_argument));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestReportManager, DISABLED_failToAddReportTwice)
{
    reportFactoryMock.expectMake(_, reportParams, Ref(*sut), Ref(storageMock))
        .WillOnce(Return(ByMove(std::move(reportMockPtr))));

    addReport(reportParams);

    auto [ec, path] = addReport(reportParams);

    EXPECT_THAT(ec.value(), Eq(boost::system::errc::file_exists));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestReportManager, DISABLED_failToAddReportWithInvalidInterval)
{
    reportFactoryMock.expectMake(_, std::nullopt, Ref(*sut), Ref(storageMock))
        .Times(0);
    reportFactoryMock.expectMake(std::nullopt, Ref(*sut), Ref(storageMock), _)
        .Times(0);

    reportParams.reportingType("Periodic");
    reportParams.interval(reportParams.interval() - 1ms);

    auto [ec, path] = addReport(reportParams);

    EXPECT_THAT(ec.value(), Eq(boost::system::errc::invalid_argument));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestReportManager, DISABLED_failToAddReportWithInvalidReportingType)
{
    reportFactoryMock.expectMake(_, std::nullopt, Ref(*sut), Ref(storageMock))
        .Times(0);
    reportFactoryMock.expectMake(std::nullopt, Ref(*sut), Ref(storageMock), _)
        .Times(0);

    reportParams.reportingType("Invalid");

    auto [ec, path] = addReport(reportParams);

    EXPECT_THAT(ec.value(), Eq(boost::system::errc::invalid_argument));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestReportManager, DISABLED_failToAddReportWithMoreSensorsThanExpected)
{
    reportFactoryMock.expectMake(_, std::nullopt, Ref(*sut), Ref(storageMock))
        .Times(0);
    reportFactoryMock.expectMake(std::nullopt, Ref(*sut), Ref(storageMock), _)
        .Times(0);

    auto readingParams = reportParams.readingParameters();
    for (size_t i = 0; i < ReportManager::maxReadingParams + 1; i++)
    {
        readingParams.push_back(readingParams.front());
    }
    reportParams.readingParameters(std::move(readingParams));

    auto [ec, path] = addReport(reportParams);

    EXPECT_THAT(ec.value(), Eq(boost::system::errc::argument_list_too_long));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestReportManager, DISABLED_failToAddReportWhenMaxReportIsReached)
{
    reportFactoryMock.expectMake(_, std::nullopt, Ref(*sut), Ref(storageMock))
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
        reportFactoryMock
            .expectMake(_, reportParams, Ref(*sut), Ref(storageMock))
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
        reportFactoryMock
            .expectMake(_, reportParams, Ref(*sut), Ref(storageMock))
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
    reportFactoryMock.expectMake(_, reportParams, Ref(*sut), Ref(storageMock))
        .WillOnce(Return(ByMove(std::move(reportMockPtr))));
    EXPECT_CALL(reportMock, updateReadings());

    addReport(reportParams);
    sut->updateReport(reportParams.reportName());
}

TEST_F(TestReportManager, updateReportDoNothingIfReportDoesNotExist)
{
    reportFactoryMock.expectMake(_, reportParams, Ref(*sut), Ref(storageMock))
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
    reportParams.readingParameters(
        {{{sdbusplus::message::object_path(
              "/xyz/openbmc_project/sensors/power/p1")},
          utils::enumToString(operationType),
          "MetricId1",
          "Metadata1"}});

    reportFactoryMock.expectMake(_, reportParams, Ref(*sut), Ref(storageMock))
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

    static std::vector<LabeledMetricParameters>
        convertToLabeled(const ReadingParameters& params)
    {
        return utils::transform(params, [](const auto& item) {
            return LabeledMetricParameters(
                LabeledSensorParameters("service", std::get<0>(item)),
                utils::stringToOperationType(std::get<1>(item)),
                std::get<2>(item), std::get<3>(item));
        });
    }

    nlohmann::json data = nlohmann::json{
        {"Version", Report::reportVersion},
        {"Name", reportParams.reportName()},
        {"ReportingType", reportParams.reportingType()},
        {"EmitsReadingsUpdate", reportParams.emitReadingUpdate()},
        {"LogToMetricReportsCollection",
         reportParams.logToMetricReportCollection()},
        {"Interval", reportParams.interval().count()},
        {"ReadingParameters",
         convertToLabeled(reportParams.readingParameters())}};
};

TEST_F(TestReportManagerStorage, reportManagerCtorAddReportFromStorage)
{
    reportFactoryMock.expectMake(
        reportParams, _, Ref(storageMock),
        ElementsAreArray(convertToLabeled(reportParams.readingParameters())));

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
