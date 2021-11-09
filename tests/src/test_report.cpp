#include "dbus_environment.hpp"
#include "helpers.hpp"
#include "mocks/json_storage_mock.hpp"
#include "mocks/metric_mock.hpp"
#include "mocks/report_manager_mock.hpp"
#include "params/report_params.hpp"
#include "report.hpp"
#include "report_manager.hpp"
#include "utils/contains.hpp"
#include "utils/conv_container.hpp"
#include "utils/transform.hpp"
#include "utils/tstring.hpp"

#include <sdbusplus/exception.hpp>

using namespace testing;
using namespace std::literals::string_literals;
using namespace std::chrono_literals;
namespace tstring = utils::tstring;

class TestReport : public Test
{
  public:
    ReportParams defaultParams;

    std::unique_ptr<ReportManagerMock> reportManagerMock =
        std::make_unique<NiceMock<ReportManagerMock>>();
    testing::NiceMock<StorageMock> storageMock;
    std::vector<std::shared_ptr<MetricMock>> metricMocks;
    std::unique_ptr<Report> sut;

    MockFunction<void()> checkPoint;

    void initMetricMocks(
        const std::vector<LabeledMetricParameters>& metricParameters)
    {
        for (auto i = metricMocks.size(); i < metricParameters.size(); ++i)
        {
            metricMocks.emplace_back(std::make_shared<NiceMock<MetricMock>>());
        }
        metricMocks.resize(metricParameters.size());

        std::vector<MetricValue> readings{{MetricValue{"a", "b", 17.1, 114},
                                           MetricValue{"aa", "bb", 42.0, 74}}};
        readings.resize(metricParameters.size());

        for (size_t i = 0; i < metricParameters.size(); ++i)
        {
            ON_CALL(*metricMocks[i], getReadings())
                .WillByDefault(Return(std::vector({readings[i]})));
            ON_CALL(*metricMocks[i], dumpConfiguration())
                .WillByDefault(Return(metricParameters[i]));
        }
    }

    void SetUp() override
    {
        sut = makeReport(ReportParams());
    }

    static interfaces::JsonStorage::FilePath to_file_path(std::string name)
    {
        return interfaces::JsonStorage::FilePath(
            std::to_string(std::hash<std::string>{}(name)));
    }

    std::unique_ptr<Report> makeReport(const ReportParams& params)
    {
        initMetricMocks(params.metricParameters());

        return std::make_unique<Report>(
            DbusEnvironment::getIoc(), DbusEnvironment::getObjServer(),
            params.reportName(), params.reportingType(), params.reportActions(),
            params.interval(), params.appendLimit(), params.reportUpdates(),
            *reportManagerMock, storageMock,
            utils::convContainer<std::shared_ptr<interfaces::Metric>>(
                metricMocks),
            params.enabled());
    }

    template <class T>
    static T getProperty(const std::string& path, const std::string& property)
    {
        return DbusEnvironment::getProperty<T>(path, Report::reportIfaceName,
                                               property);
    }

    template <class T>
    static boost::system::error_code setProperty(const std::string& path,
                                                 const std::string& property,
                                                 const T& newValue)
    {
        return DbusEnvironment::setProperty<T>(path, Report::reportIfaceName,
                                               property, newValue);
    }

    boost::system::error_code call(const std::string& path,
                                   const std::string& interface,
                                   const std::string& method)
    {
        std::promise<boost::system::error_code> methodPromise;
        DbusEnvironment::getBus()->async_method_call(
            [&methodPromise](boost::system::error_code ec) {
                methodPromise.set_value(ec);
            },
            DbusEnvironment::serviceName(), path, interface, method);
        return DbusEnvironment::waitForFuture(methodPromise.get_future());
    }

    boost::system::error_code update(const std::string& path)
    {
        return call(path, Report::reportIfaceName, "Update");
    }

    boost::system::error_code deleteReport(const std::string& path)
    {
        return call(path, Report::deleteIfaceName, "Delete");
    }
};

TEST_F(TestReport, verifyIfPropertiesHaveValidValue)
{
    EXPECT_THAT(getProperty<bool>(sut->getPath(), "Enabled"),
                Eq(defaultParams.enabled()));
    EXPECT_THAT(getProperty<uint64_t>(sut->getPath(), "Interval"),
                Eq(defaultParams.interval().count()));
    EXPECT_THAT(getProperty<bool>(sut->getPath(), "Persistency"), Eq(true));
    EXPECT_THAT(
        getProperty<std::vector<std::string>>(sut->getPath(), "ReportActions"),
        Eq(utils::transform(defaultParams.reportActions(), [](const auto v) {
            return utils::enumToString(v);
        })));
    EXPECT_THAT(getProperty<bool>(sut->getPath(), "EmitsReadingsUpdate"),
                Eq(utils::contains(defaultParams.reportActions(),
                                   ReportAction::emitsReadingsUpdate)));
    EXPECT_THAT(getProperty<uint64_t>(sut->getPath(), "AppendLimit"),
                Eq(defaultParams.appendLimit()));
    EXPECT_THAT(getProperty<std::string>(sut->getPath(), "ReportingType"),
                Eq(utils::enumToString(defaultParams.reportingType())));
    EXPECT_THAT(getProperty<std::string>(sut->getPath(), "ReportUpdates"),
                Eq(utils::enumToString(defaultParams.reportUpdates())));
    EXPECT_THAT(
        getProperty<bool>(sut->getPath(), "LogToMetricReportsCollection"),
        Eq(utils::contains(defaultParams.reportActions(),
                           ReportAction::logToMetricReportsCollection)));
    EXPECT_THAT(getProperty<ReadingParameters>(
                    sut->getPath(), "ReadingParametersFutureVersion"),
                Eq(toReadingParameters(defaultParams.metricParameters())));
}

TEST_F(TestReport, readingsAreInitialyEmpty)
{
    EXPECT_THAT(getProperty<Readings>(sut->getPath(), "Readings"),
                Eq(Readings{}));
}

TEST_F(TestReport, setEnabledWithNewValue)
{
    bool newValue = !defaultParams.enabled();
    EXPECT_THAT(setProperty(sut->getPath(), "Enabled", newValue).value(),
                Eq(boost::system::errc::success));
    EXPECT_THAT(getProperty<bool>(sut->getPath(), "Enabled"), Eq(newValue));
}

TEST_F(TestReport, setIntervalWithValidValue)
{
    uint64_t newValue = defaultParams.interval().count() + 1;
    EXPECT_THAT(setProperty(sut->getPath(), "Interval", newValue).value(),
                Eq(boost::system::errc::success));
    EXPECT_THAT(getProperty<uint64_t>(sut->getPath(), "Interval"),
                Eq(newValue));
}

TEST_F(TestReport, settingIntervalWithInvalidValueDoesNotChangeProperty)
{
    uint64_t newValue = defaultParams.interval().count() - 1;
    EXPECT_THAT(setProperty(sut->getPath(), "Interval", newValue).value(),
                Eq(boost::system::errc::success));
    EXPECT_THAT(getProperty<uint64_t>(sut->getPath(), "Interval"),
                Eq(defaultParams.interval().count()));
}

TEST_F(TestReport, settingEmitsReadingsUpdateHaveNoEffect)
{
    EXPECT_THAT(
        setProperty(sut->getPath(), "EmitsReadingsUpdate", true).value(),
        Eq(boost::system::errc::read_only_file_system));
    EXPECT_THAT(getProperty<bool>(sut->getPath(), "EmitsReadingsUpdate"),
                Eq(utils::contains(defaultParams.reportActions(),
                                   ReportAction::emitsReadingsUpdate)));
}

TEST_F(TestReport, settingLogToMetricReportCollectionHaveNoEffect)
{
    EXPECT_THAT(
        setProperty(sut->getPath(), "LogToMetricReportsCollection", true)
            .value(),
        Eq(boost::system::errc::read_only_file_system));
    EXPECT_THAT(
        getProperty<bool>(sut->getPath(), "LogToMetricReportsCollection"),
        Eq(utils::contains(defaultParams.reportActions(),
                           ReportAction::logToMetricReportsCollection)));
}

TEST_F(TestReport, settingPersistencyToFalseRemovesReportFromStorage)
{
    EXPECT_CALL(storageMock, remove(to_file_path(sut->getName())));

    bool persistency = false;
    EXPECT_THAT(setProperty(sut->getPath(), "Persistency", persistency).value(),
                Eq(boost::system::errc::success));
    EXPECT_THAT(getProperty<bool>(sut->getPath(), "Persistency"),
                Eq(persistency));
}

TEST_F(TestReport, deleteReport)
{
    EXPECT_CALL(*reportManagerMock, removeReport(sut.get()));
    auto ec = deleteReport(sut->getPath());
    EXPECT_THAT(ec, Eq(boost::system::errc::success));
}

TEST_F(TestReport, deletingNonExistingReportReturnInvalidRequestDescriptor)
{
    auto ec = deleteReport(Report::reportDir + "NonExisting"s);
    EXPECT_THAT(ec.value(), Eq(EBADR));
}

TEST_F(TestReport, deleteReportExpectThatFileIsRemoveFromStorage)
{
    EXPECT_CALL(storageMock, remove(to_file_path(sut->getName())));
    auto ec = deleteReport(sut->getPath());
    EXPECT_THAT(ec, Eq(boost::system::errc::success));
}

class TestReportStore :
    public TestReport,
    public WithParamInterface<std::pair<std::string, nlohmann::json>>
{
  public:
    void SetUp() override
    {}

    nlohmann::json storedConfiguration;
};

INSTANTIATE_TEST_SUITE_P(
    _, TestReportStore,
    Values(std::make_pair("Enabled"s, nlohmann::json(ReportParams().enabled())),
           std::make_pair("Version"s, nlohmann::json(6)),
           std::make_pair("Name"s, nlohmann::json(ReportParams().reportName())),
           std::make_pair("ReportingType",
                          nlohmann::json(ReportParams().reportingType())),
           std::make_pair("ReportActions", nlohmann::json(utils::transform(
                                               ReportParams().reportActions(),
                                               [](const auto v) {
                                                   return utils::toUnderlying(
                                                       v);
                                               }))),
           std::make_pair("Interval",
                          nlohmann::json(ReportParams().interval().count())),
           std::make_pair(
               "ReadingParameters",
               nlohmann::json(
                   {{{tstring::SensorPath::str(),
                      {{{tstring::Service::str(), "Service"},
                        {tstring::Path::str(),
                         "/xyz/openbmc_project/sensors/power/p1"}}}},
                     {tstring::OperationType::str(), OperationType::single},
                     {tstring::Id::str(), "MetricId1"},
                     {tstring::MetricMetadata::str(), "Metadata1"},
                     {tstring::CollectionTimeScope::str(),
                      CollectionTimeScope::point},
                     {tstring::CollectionDuration::str(), 0}},
                    {{tstring::SensorPath::str(),
                      {{{tstring::Service::str(), "Service"},
                        {tstring::Path::str(),
                         "/xyz/openbmc_project/sensors/power/p2"}}}},
                     {tstring::OperationType::str(), OperationType::single},
                     {tstring::Id::str(), "MetricId2"},
                     {tstring::MetricMetadata::str(), "Metadata2"},
                     {tstring::CollectionTimeScope::str(),
                      CollectionTimeScope::point},
                     {tstring::CollectionDuration::str(), 0}}}))));

TEST_P(TestReportStore, settingPersistencyToTrueStoresReport)
{
    sut = makeReport(ReportParams());

    {
        InSequence seq;
        EXPECT_CALL(storageMock, remove(to_file_path(sut->getName())));
        EXPECT_CALL(checkPoint, Call());
        EXPECT_CALL(storageMock, store(to_file_path(sut->getName()), _))
            .WillOnce(SaveArg<1>(&storedConfiguration));
    }

    setProperty(sut->getPath(), "Persistency", false);
    checkPoint.Call();
    setProperty(sut->getPath(), "Persistency", true);

    const auto& [key, value] = GetParam();

    ASSERT_THAT(storedConfiguration.at(key), Eq(value));
}

TEST_P(TestReportStore, reportIsSavedToStorageAfterCreated)
{
    EXPECT_CALL(storageMock,
                store(to_file_path(ReportParams().reportName()), _))
        .WillOnce(SaveArg<1>(&storedConfiguration));

    sut = makeReport(ReportParams());

    const auto& [key, value] = GetParam();

    ASSERT_THAT(storedConfiguration.at(key), Eq(value));
}

class TestReportValidNames :
    public TestReport,
    public WithParamInterface<ReportParams>
{
  public:
    void SetUp() override
    {}
};

INSTANTIATE_TEST_SUITE_P(
    ValidNames, TestReportValidNames,
    Values(ReportParams().reportName("Valid_1"),
           ReportParams().reportName("Valid_1/Valid_2"),
           ReportParams().reportName("Valid_1/Valid_2/Valid_3")));

TEST_P(TestReportValidNames, reportCtorDoesNotThrowOnValidName)
{
    EXPECT_NO_THROW(makeReport(GetParam()));
}

class TestReportInvalidNames :
    public TestReport,
    public WithParamInterface<ReportParams>
{
  public:
    void SetUp() override
    {}
};

INSTANTIATE_TEST_SUITE_P(InvalidNames, TestReportInvalidNames,
                         Values(ReportParams().reportName("/"),
                                ReportParams().reportName("/Invalid"),
                                ReportParams().reportName("Invalid/"),
                                ReportParams().reportName("Invalid/Invalid/"),
                                ReportParams().reportName("Invalid?")));

TEST_P(TestReportInvalidNames, reportCtorThrowOnInvalidName)
{
    EXPECT_THROW(makeReport(GetParam()), sdbusplus::exception::SdBusError);
}

TEST_F(TestReportInvalidNames, reportCtorThrowOnInvalidNameAndNoStoreIsCalled)
{
    EXPECT_CALL(storageMock, store).Times(0);
    EXPECT_THROW(makeReport(ReportParams().reportName("/Invalid")),
                 sdbusplus::exception::SdBusError);
}

class TestReportAllReportTypes :
    public TestReport,
    public WithParamInterface<ReportParams>
{
  public:
    void SetUp() override
    {
        sut = makeReport(GetParam());
    }
};

INSTANTIATE_TEST_SUITE_P(
    _, TestReportAllReportTypes,
    Values(ReportParams().reportingType(ReportingType::onRequest),
           ReportParams().reportingType(ReportingType::onChange),
           ReportParams().reportingType(ReportingType::periodic)));

TEST_P(TestReportAllReportTypes, returnPropertValueOfReportType)
{
    EXPECT_THAT(utils::toReportingType(
                    getProperty<std::string>(sut->getPath(), "ReportingType")),
                Eq(GetParam().reportingType()));
}

TEST_P(TestReportAllReportTypes, updateReadingsCallEnabledPropertyOff)
{
    const uint64_t expectedTime = std::time(0);

    setProperty(sut->getPath(), "Enabled", false);
    sut->updateReadings();
    const auto [timestamp, readings] =
        getProperty<Readings>(sut->getPath(), "Readings");

    EXPECT_THAT(getProperty<bool>(sut->getPath(), "Enabled"), Eq(false));
    EXPECT_THAT(timestamp, Lt(expectedTime));
}

TEST_P(TestReportAllReportTypes, updateReadingsCallEnabledPropertyOn)
{
    const uint64_t expectedTime = std::time(0);

    sut->updateReadings();
    const auto [timestamp, readings] =
        getProperty<Readings>(sut->getPath(), "Readings");

    EXPECT_THAT(getProperty<bool>(sut->getPath(), "Enabled"), Eq(true));
    EXPECT_THAT(timestamp, Ge(expectedTime));
}

class TestReportOnRequestType : public TestReport
{
    void SetUp() override
    {
        sut =
            makeReport(ReportParams().reportingType(ReportingType::onRequest));
    }
};

TEST_F(TestReportOnRequestType, updatesReadingTimestamp)
{
    const uint64_t expectedTime = std::time(0);

    ASSERT_THAT(update(sut->getPath()), Eq(boost::system::errc::success));

    const auto [timestamp, readings] =
        getProperty<Readings>(sut->getPath(), "Readings");

    EXPECT_THAT(timestamp, Ge(expectedTime));
}

TEST_F(TestReportOnRequestType, updatesReadingWhenUpdateIsCalled)
{
    ASSERT_THAT(update(sut->getPath()), Eq(boost::system::errc::success));

    const auto [timestamp, readings] =
        getProperty<Readings>(sut->getPath(), "Readings");

    EXPECT_THAT(readings,
                ElementsAre(std::make_tuple("a"s, "b"s, 17.1, 114u),
                            std::make_tuple("aa"s, "bb"s, 42.0, 74u)));
}

class TestReportNonOnRequestType :
    public TestReport,
    public WithParamInterface<ReportParams>
{
    void SetUp() override
    {
        sut = makeReport(GetParam());
    }
};

INSTANTIATE_TEST_SUITE_P(
    _, TestReportNonOnRequestType,
    Values(ReportParams().reportingType(ReportingType::periodic),
           ReportParams().reportingType(ReportingType::onChange)));

TEST_P(TestReportNonOnRequestType, readingsAreNotUpdateOnUpdateCall)
{
    ASSERT_THAT(update(sut->getPath()), Eq(boost::system::errc::success));

    EXPECT_THAT(getProperty<Readings>(sut->getPath(), "Readings"),
                Eq(Readings{}));
}

class TestReportNonPeriodicReport :
    public TestReport,
    public WithParamInterface<ReportParams>
{
  public:
    void SetUp() override
    {
        sut = makeReport(GetParam());
    }
};

INSTANTIATE_TEST_SUITE_P(
    _, TestReportNonPeriodicReport,
    Values(ReportParams().reportingType(ReportingType::onRequest),
           ReportParams().reportingType(ReportingType::onChange)));

TEST_P(TestReportNonPeriodicReport, readingsAreNotUpdatedAfterIntervalExpires)
{
    DbusEnvironment::sleepFor(ReportManager::minInterval + 1ms);

    EXPECT_THAT(getProperty<Readings>(sut->getPath(), "Readings"),
                Eq(Readings{}));
}

class TestReportPeriodicReport : public TestReport
{
    void SetUp() override
    {
        sut = makeReport(ReportParams().reportingType(ReportingType::periodic));
    }
};

TEST_F(TestReportPeriodicReport, readingTimestampIsUpdatedAfterIntervalExpires)
{
    const uint64_t expectedTime = std::time(0);
    DbusEnvironment::sleepFor(ReportManager::minInterval + 1ms);

    const auto [timestamp, readings] =
        getProperty<Readings>(sut->getPath(), "Readings");

    EXPECT_THAT(timestamp, Ge(expectedTime));
}

TEST_F(TestReportPeriodicReport, readingsAreUpdatedAfterIntervalExpires)
{
    DbusEnvironment::sleepFor(ReportManager::minInterval + 1ms);

    const auto [timestamp, readings] =
        getProperty<Readings>(sut->getPath(), "Readings");

    EXPECT_THAT(readings,
                ElementsAre(std::make_tuple("a"s, "b"s, 17.1, 114u),
                            std::make_tuple("aa"s, "bb"s, 42.0, 74u)));
}

struct ReportUpdatesReportParams
{
    ReportParams reportParams;
    std::vector<ReadingData> expectedReadings;
    bool expectedEnabled;
};

class TestReportWithReportUpdatesAndLimit :
    public TestReport,
    public WithParamInterface<ReportUpdatesReportParams>
{
    void SetUp() override
    {
        sut = makeReport(ReportParams(GetParam().reportParams)
                             .reportingType(ReportingType::periodic)
                             .interval(std::chrono::hours(1000)));
    }
};

INSTANTIATE_TEST_SUITE_P(
    _, TestReportWithReportUpdatesAndLimit,
    Values(
        ReportUpdatesReportParams{
            ReportParams()
                .reportUpdates(ReportUpdates::appendWrapsWhenFull)
                .appendLimit(5),
            std::vector<ReadingData>{{std::make_tuple("aa"s, "bb"s, 42.0, 74u),
                                      std::make_tuple("a"s, "b"s, 17.1, 114u),
                                      std::make_tuple("aa"s, "bb"s, 42.0, 74u),
                                      std::make_tuple("aa"s, "bb"s, 42.0, 74u),
                                      std::make_tuple("a"s, "b"s, 17.1, 114u)}},
            true},
        ReportUpdatesReportParams{
            ReportParams()
                .reportUpdates(ReportUpdates::appendWrapsWhenFull)
                .appendLimit(4),
            std::vector<ReadingData>{
                {std::make_tuple("a"s, "b"s, 17.1, 114u),
                 std::make_tuple("aa"s, "bb"s, 42.0, 74u),
                 std::make_tuple("a"s, "b"s, 17.1, 114u),
                 std::make_tuple("aa"s, "bb"s, 42.0, 74u)}},
            true},
        ReportUpdatesReportParams{
            ReportParams()
                .reportUpdates(ReportUpdates::appendWrapsWhenFull)
                .appendLimit(0),
            std::vector<ReadingData>{}, true},
        ReportUpdatesReportParams{
            ReportParams()
                .reportUpdates(ReportUpdates::appendStopsWhenFull)
                .appendLimit(10),
            std::vector<ReadingData>{
                {std::make_tuple("a"s, "b"s, 17.1, 114u),
                 std::make_tuple("aa"s, "bb"s, 42.0, 74u),
                 std::make_tuple("a"s, "b"s, 17.1, 114u),
                 std::make_tuple("aa"s, "bb"s, 42.0, 74u),
                 std::make_tuple("a"s, "b"s, 17.1, 114u),
                 std::make_tuple("aa"s, "bb"s, 42.0, 74u),
                 std::make_tuple("a"s, "b"s, 17.1, 114u),
                 std::make_tuple("aa"s, "bb"s, 42.0, 74u)}},
            true},
        ReportUpdatesReportParams{
            ReportParams()
                .reportUpdates(ReportUpdates::appendStopsWhenFull)
                .appendLimit(5),
            std::vector<ReadingData>{{std::make_tuple("a"s, "b"s, 17.1, 114u),
                                      std::make_tuple("aa"s, "bb"s, 42.0, 74u),
                                      std::make_tuple("a"s, "b"s, 17.1, 114u),
                                      std::make_tuple("aa"s, "bb"s, 42.0, 74u),
                                      std::make_tuple("a"s, "b"s, 17.1, 114u)}},
            false},
        ReportUpdatesReportParams{
            ReportParams()
                .reportUpdates(ReportUpdates::appendStopsWhenFull)
                .appendLimit(4),
            std::vector<ReadingData>{
                {std::make_tuple("a"s, "b"s, 17.1, 114u),
                 std::make_tuple("aa"s, "bb"s, 42.0, 74u),
                 std::make_tuple("a"s, "b"s, 17.1, 114u),
                 std::make_tuple("aa"s, "bb"s, 42.0, 74u)}},
            false},
        ReportUpdatesReportParams{
            ReportParams()
                .reportUpdates(ReportUpdates::appendStopsWhenFull)
                .appendLimit(0),
            std::vector<ReadingData>{}, false},
        ReportUpdatesReportParams{
            ReportParams()
                .reportUpdates(ReportUpdates::overwrite)
                .appendLimit(500),
            std::vector<ReadingData>{
                {std::make_tuple("a"s, "b"s, 17.1, 114u),
                 std::make_tuple("aa"s, "bb"s, 42.0, 74u)}},
            true},
        ReportUpdatesReportParams{
            ReportParams()
                .reportUpdates(ReportUpdates::overwrite)
                .appendLimit(1),
            std::vector<ReadingData>{
                {std::make_tuple("a"s, "b"s, 17.1, 114u),
                 std::make_tuple("aa"s, "bb"s, 42.0, 74u)}},
            true},
        ReportUpdatesReportParams{
            ReportParams()
                .reportUpdates(ReportUpdates::overwrite)
                .appendLimit(0),
            std::vector<ReadingData>{
                {std::make_tuple("a"s, "b"s, 17.1, 114u),
                 std::make_tuple("aa"s, "bb"s, 42.0, 74u)}},
            true}));

TEST_P(TestReportWithReportUpdatesAndLimit,
       readingsAreUpdatedAfterIntervalExpires)
{
    for (int i = 0; i < 4; i++)
    {
        sut->updateReadings();
    }

    const auto [timestamp, readings] =
        getProperty<Readings>(sut->getPath(), "Readings");
    const auto enabled = getProperty<bool>(sut->getPath(), "Enabled");

    EXPECT_THAT(readings, ElementsAreArray(GetParam().expectedReadings));
    EXPECT_EQ(enabled, GetParam().expectedEnabled);
}

class TestReportInitialization : public TestReport
{
  public:
    void SetUp() override
    {}

    void monitorProc(sdbusplus::message::message& msg)
    {
        std::string iface;
        std::vector<std::pair<std::string, std::variant<Readings>>>
            changed_properties;
        std::vector<std::string> invalidated_properties;

        msg.read(iface, changed_properties, invalidated_properties);

        if (iface == Report::reportIfaceName)
        {
            for (const auto& [name, value] : changed_properties)
            {
                if (name == "Readings")
                {
                    readingsUpdated.Call();
                }
            }
        }
    }

    void makeMonitor()
    {
        monitor = std::make_unique<sdbusplus::bus::match_t>(
            *DbusEnvironment::getBus(),
            sdbusplus::bus::match::rules::propertiesChanged(
                sut->getPath(), Report::reportIfaceName),
            [this](auto& msg) { monitorProc(msg); });
    }

    std::unique_ptr<sdbusplus::bus::match_t> monitor;
    MockFunction<void()> readingsUpdated;
};

TEST_F(TestReportInitialization,
       metricsAreInitializedWhenEnabledReportConstructed)
{
    initMetricMocks(defaultParams.metricParameters());
    for (auto& metric : metricMocks)
    {
        EXPECT_CALL(*metric, initialize()).Times(1);
    }
    sut = makeReport(defaultParams.enabled(true));
}

TEST_F(TestReportInitialization,
       metricsAreNotInitializedWhenDisabledReportConstructed)
{
    initMetricMocks(defaultParams.metricParameters());
    for (auto& metric : metricMocks)
    {
        EXPECT_CALL(*metric, initialize()).Times(0);
    }
    sut = makeReport(defaultParams.enabled(false));
}

TEST_F(TestReportInitialization,
       emitReadingsUpdateIsTrueReadingsPropertiesChangedSingalEmits)
{
    EXPECT_CALL(readingsUpdated, Call())
        .WillOnce(
            InvokeWithoutArgs(DbusEnvironment::setPromise("readingsUpdated")));

    const auto elapsed = DbusEnvironment::measureTime([this] {
        sut =
            makeReport(defaultParams.reportingType(ReportingType::periodic)
                           .reportActions({ReportAction::emitsReadingsUpdate}));
        makeMonitor();
        EXPECT_TRUE(DbusEnvironment::waitForFuture("readingsUpdated"));
    });

    EXPECT_THAT(elapsed, AllOf(Ge(defaultParams.interval()),
                               Lt(defaultParams.interval() * 2)));
}

TEST_F(TestReportInitialization,
       emitReadingsUpdateIsFalseReadingsPropertiesChangesSigalDoesNotEmits)
{
    EXPECT_CALL(readingsUpdated, Call()).Times(0);

    sut = makeReport(
        defaultParams.reportingType(ReportingType::periodic).reportActions({}));
    makeMonitor();
    DbusEnvironment::sleepFor(defaultParams.interval() * 2);
}

TEST_F(TestReportInitialization, appendLimitDeducedProperly)
{
    sut = makeReport(
        ReportParams().appendLimit(std::numeric_limits<uint64_t>::max()));
    auto appendLimit = getProperty<uint64_t>(sut->getPath(), "AppendLimit");
    EXPECT_EQ(appendLimit, 2ull);
}
