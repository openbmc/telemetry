#include "dbus_environment.hpp"
#include "helpers.hpp"
#include "interfaces/trigger_manager.hpp"
#include "mocks/json_storage_mock.hpp"
#include "mocks/report_factory_mock.hpp"
#include "mocks/trigger_manager_mock.hpp"
#include "params/report_params.hpp"
#include "report.hpp"
#include "report_manager.hpp"
#include "utils/conversion.hpp"
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
        std::make_unique<NiceMock<ReportMock>>(reportParams.reportId());
    ReportMock& reportMock = *reportMockPtr;

    std::unique_ptr<interfaces::TriggerManager> triggerManagerMockPtr =
        std::make_unique<NiceMock<TriggerManagerMock>>();

    std::unique_ptr<ReportManager> sut;

    MockFunction<void(std::string)> checkPoint;

    void SetUp() override
    {
        EXPECT_CALL(reportFactoryMock, convertMetricParams(_, _))
            .Times(AnyNumber());

        sut = std::make_unique<ReportManager>(
            std::move(reportFactoryMockPtr), std::move(storageMockPtr),
            DbusEnvironment::getObjServer(), triggerManagerMockPtr);
    }

    void TearDown() override
    {
        DbusEnvironment::synchronizeIoc();
    }

    template <class... Args>
    requires(sizeof...(Args) > 1)
        std::pair<boost::system::error_code, std::string> addReport(
            Args&&... args)
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
            std::forward<Args>(args)...);
        return DbusEnvironment::waitForFuture(addReportPromise.get_future());
    }

    auto addReport(const ReportParams& params)
    {
        return addReport(params.reportId(), params.reportName(),
                         utils::enumToString(params.reportingType()),
                         utils::enumToString(params.reportUpdates()),
                         params.appendLimit(),
                         utils::transform(params.reportActions(),
                                          [](const auto v) {
                                              return utils::enumToString(v);
                                          }),
                         params.interval().count(),
                         toReadingParameters(params.metricParameters()));
    }

    template <class T>
    static T getProperty(const std::string& property)
    {
        return DbusEnvironment::getProperty<T>(
            ReportManager::reportManagerPath,
            ReportManager::reportManagerIfaceName, property);
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

TEST_F(TestReportManager, nameIsUsedToGenerateIdWhenIdIsEmptyInAddReport)
{
    reportParams.reportId("ReportName");
    reportParams.reportName("ReportName");

    reportFactoryMock.expectMake(reportParams, Ref(*sut), Ref(storageMock));

    auto [ec, path] = addReport(reportParams.reportId(""));

    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    EXPECT_THAT(path, Eq("/ReportName"));
}

TEST_F(TestReportManager, nameIsUsedToGenerateIdWhenIdIsNamespace)
{
    reportParams.reportId("Prefix/ReportName");
    reportParams.reportName("ReportName");

    reportFactoryMock.expectMake(reportParams, Ref(*sut), Ref(storageMock));

    auto [ec, path] = addReport(reportParams.reportId("Prefix/"));

    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    EXPECT_THAT(path, Eq("/Prefix/ReportName"));
}

TEST_F(TestReportManager, addReportWithMaxLengthId)
{
    std::string reportId(ReportManager::maxReportIdLength, 'z');
    reportParams.reportId(reportId);
    reportFactoryMock.expectMake(reportParams, Ref(*sut), Ref(storageMock));

    auto [ec, path] = addReport(reportParams);

    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    EXPECT_THAT(path, Eq("/"s + reportId));
}

TEST_F(TestReportManager, DISABLED_failToAddReportWithTooLongName)
{
    reportFactoryMock.expectMake(std::nullopt, Ref(*sut), Ref(storageMock))
        .Times(0);

    reportParams.reportId(
        std::string(ReportManager::maxReportIdLength + 1, 'z'));

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

    reportParams.reportingType(ReportingType::periodic);
    reportParams.interval(reportParams.interval() - 1ms);

    auto [ec, path] = addReport(reportParams);

    EXPECT_THAT(ec.value(), Eq(boost::system::errc::invalid_argument));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestReportManager, DISABLED_failToAddReportWithInvalidReportingType)
{
    reportFactoryMock.expectMake(std::nullopt, Ref(*sut), Ref(storageMock))
        .Times(0);

    auto [ec, path] = addReport(
        reportParams.reportName(), "InvalidReportingType",
        utils::transform(reportParams.reportActions(),
                         [](const auto v) { return utils::enumToString(v); }),
        reportParams.interval().count(),
        toReadingParameters(reportParams.metricParameters()));

    EXPECT_THAT(ec.value(), Eq(boost::system::errc::invalid_argument));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestReportManager,
       DISABLED_failToAddReportWithMoreMetricPropertiesThanExpected)
{
    reportFactoryMock.expectMake(std::nullopt, Ref(*sut), Ref(storageMock))
        .Times(0);

    reportParams.metricParameters(
        std::vector<LabeledMetricParameters>{{LabeledMetricParameters{
            {LabeledSensorInfo{"Service",
                               "/xyz/openbmc_project/sensors/power/p1",
                               "Metadata1"}},
            OperationType::single,
            "MetricId1",
            CollectionTimeScope::point,
            CollectionDuration(Milliseconds(0u))}}});

    auto metricParams = reportParams.metricParameters();
    auto& metricParamsVec =
        metricParams[0].at_label<utils::tstring::SensorPath>();

    for (size_t i = 0; i < ReportManager::maxNumberMetrics; i++)
    {
        metricParamsVec.emplace_back(LabeledSensorInfo{
            "Service", "/xyz/openbmc_project/sensors/power/p1", "Metadata1"});
    }

    reportParams.metricParameters(std::move(metricParams));

    auto [ec, path] = addReport(reportParams);

    EXPECT_THAT(ec.value(), Eq(boost::system::errc::argument_list_too_long));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestReportManager, DISABLED_failToAddReportWithMoreMetricsThanExpected)
{
    reportFactoryMock.expectMake(std::nullopt, Ref(*sut), Ref(storageMock))
        .Times(0);

    auto metricParams = std::vector<LabeledMetricParameters>{};

    for (size_t i = 0; i < ReportManager::maxNumberMetrics + 1; i++)
    {
        metricParams.emplace_back(
            LabeledMetricParameters{{},
                                    OperationType::single,
                                    "MetricId1",
                                    CollectionTimeScope::point,
                                    CollectionDuration(Milliseconds(0u))});
    }

    reportParams.metricParameters(std::move(metricParams));

    auto [ec, path] = addReport(reportParams);

    EXPECT_THAT(ec.value(), Eq(boost::system::errc::argument_list_too_long));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestReportManager, DISABLED_failToAddReportWithAppendLimitGreaterThanMax)
{
    reportFactoryMock.expectMake(std::nullopt, Ref(*sut), Ref(storageMock))
        .Times(0);

    reportParams.appendLimit(ReportManager::maxAppendLimit + 1);

    auto [ec, path] = addReport(reportParams);

    EXPECT_THAT(ec.value(), Eq(boost::system::errc::invalid_argument));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestReportManager, addReportWithAppendLimitEqualToUint64MaxIsAllowed)
{
    reportParams.appendLimit(std::numeric_limits<uint64_t>::max());

    EXPECT_CALL(reportFactoryMock, convertMetricParams(_, _));
    reportFactoryMock.expectMake(reportParams, Ref(*sut), Ref(storageMock))
        .WillOnce(Return(ByMove(std::move(reportMockPtr))));

    auto [ec, path] = addReport(reportParams);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    EXPECT_THAT(path, Eq(reportMock.getPath()));
}

TEST_F(TestReportManager, DISABLED_failToAddReportWhenMaxReportIsReached)
{
    reportFactoryMock.expectMake(std::nullopt, Ref(*sut), Ref(storageMock))
        .Times(ReportManager::maxReports);

    for (size_t i = 0; i < ReportManager::maxReports; i++)
    {
        reportParams.reportId(reportParams.reportName() + std::to_string(i));

        auto [ec, path] = addReport(reportParams);
        EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    }

    reportParams.reportId(reportParams.reportName() +
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
    sut->updateReport(reportParams.reportId());
}

TEST_F(TestReportManager, updateReportDoNothingIfReportDoesNotExist)
{
    reportFactoryMock.expectMake(reportParams, Ref(*sut), Ref(storageMock))
        .WillOnce(Return(ByMove(std::move(reportMockPtr))));
    EXPECT_CALL(reportMock, updateReadings()).Times(0);

    addReport(reportParams);
    sut->updateReport("NotAReport");
}

TEST_F(TestReportManager, updateTriggerIdsUpdatesThemForExistReport)
{
    reportFactoryMock.expectMake(reportParams, Ref(*sut), Ref(storageMock))
        .WillOnce(Return(ByMove(std::move(reportMockPtr))));
    EXPECT_CALL(reportMock, updateTriggerIds("Trigger1", TriggerIdUpdate::Add));
    EXPECT_CALL(reportMock,
                updateTriggerIds("Trigger2", TriggerIdUpdate::Remove));

    addReport(reportParams);
    sut->updateTriggerIds(reportParams.reportId(), "Trigger1",
                          TriggerIdUpdate::Add);

    sut->updateTriggerIds(reportParams.reportId(), "Trigger2",
                          TriggerIdUpdate::Remove);
}

TEST_F(TestReportManager, updateTriggerIdsDoNothingIfReportDoesNotExist)
{
    reportFactoryMock.expectMake(reportParams, Ref(*sut), Ref(storageMock))
        .WillOnce(Return(ByMove(std::move(reportMockPtr))));
    EXPECT_CALL(reportMock, updateTriggerIds(_, _)).Times(0);

    addReport(reportParams);
    sut->updateTriggerIds("NotAReport", "Trigger1", TriggerIdUpdate::Add);
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
            {LabeledSensorInfo{"Service",
                               "/xyz/openbmc_project/sensors/power/p1",
                               "Metadata1"}},
            operationType,
            "MetricId1",
            CollectionTimeScope::point,
            CollectionDuration(Milliseconds(0u))}}});

    reportFactoryMock.expectMake(reportParams, Ref(*sut), Ref(storageMock))
        .WillOnce(Return(ByMove(std::move(reportMockPtr))));

    auto [ec, path] = addReport(reportParams);

    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    EXPECT_THAT(path, Eq("/"s + reportParams.reportId()));
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
        sut = std::make_unique<ReportManager>(
            std::move(reportFactoryMockPtr), std::move(storageMockPtr),
            DbusEnvironment::getObjServer(), triggerManagerMockPtr);
    }

    nlohmann::json data = nlohmann::json{
        {"Enabled", reportParams.enabled()},
        {"Version", Report::reportVersion},
        {"Id", reportParams.reportId()},
        {"Name", reportParams.reportName()},
        {"ReportingType", utils::toUnderlying(reportParams.reportingType())},
        {"ReportActions", reportParams.reportActions()},
        {"Interval", reportParams.interval().count()},
        {"ReportUpdates", utils::toUnderlying(reportParams.reportUpdates())},
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
