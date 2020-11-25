#include "dbus_environment.hpp"
#include "mocks/json_storage_mock.hpp"
#include "mocks/metric_mock.hpp"
#include "mocks/report_manager_mock.hpp"
#include "params/report_params.hpp"
#include "printers.hpp"
#include "report.hpp"
#include "report_manager.hpp"
#include "utils/conv_container.hpp"
#include "utils/set_exception.hpp"

#include <sdbusplus/exception.hpp>

using namespace testing;
using namespace std::literals::string_literals;
using namespace std::chrono_literals;

class TestReport : public Test
{
  public:
    ReportParams defaultParams;

    std::unique_ptr<ReportManagerMock> reportManagerMock =
        std::make_unique<NiceMock<ReportManagerMock>>();
    testing::NiceMock<StorageMock> storageMock;
    std::vector<std::shared_ptr<MetricMock>> metricMocks = {
        std::make_shared<NiceMock<MetricMock>>(),
        std::make_shared<NiceMock<MetricMock>>(),
        std::make_shared<NiceMock<MetricMock>>()};
    std::unique_ptr<Report> sut;

    MockFunction<void()> checkPoint;

    TestReport()
    {
        ON_CALL(*metricMocks[0], getReadings())
            .WillByDefault(ReturnRefOfCopy(std::vector<MetricValue>(
                {MetricValue{"a", "b", 17.1, 114},
                 MetricValue{"aaa", "bbb", 21.7, 100}})));
        ON_CALL(*metricMocks[1], getReadings())
            .WillByDefault(ReturnRefOfCopy(
                std::vector<MetricValue>({MetricValue{"aa", "bb", 42.0, 74}})));

        for (size_t i = 0; i < metricMocks.size(); ++i)
        {
            ON_CALL(*metricMocks[i], to_json())
                .WillByDefault(
                    Return(nlohmann::json("metric"s + std::to_string(i))));
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
        return std::make_unique<Report>(
            DbusEnvironment::getIoc(), DbusEnvironment::getObjServer(),
            params.reportName(), params.reportingType(),
            params.emitReadingUpdate(), params.logToMetricReportCollection(),
            params.interval(), params.readingParameters(), *reportManagerMock,
            storageMock,
            utils::convContainer<std::shared_ptr<interfaces::Metric>>(
                metricMocks));
    }

    template <class T>
    static T getProperty(const std::string& path, const std::string& property)
    {
        std::promise<T> propertyPromise;
        sdbusplus::asio::getProperty<T>(
            *DbusEnvironment::getBus(), DbusEnvironment::serviceName(), path,
            Report::reportIfaceName, property,
            [&propertyPromise](boost::system::error_code) {
                utils::setException(propertyPromise, "GetProperty failed");
            },
            [&propertyPromise](T t) { propertyPromise.set_value(t); });
        return DbusEnvironment::waitForFuture(propertyPromise.get_future());
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

    template <class T>
    static boost::system::error_code setProperty(const std::string& path,
                                                 const std::string& property,
                                                 const T& newValue)
    {
        std::promise<boost::system::error_code> setPromise;
        sdbusplus::asio::setProperty(
            *DbusEnvironment::getBus(), DbusEnvironment::serviceName(), path,
            Report::reportIfaceName, property, std::move(newValue),
            [&setPromise](boost::system::error_code ec) {
                setPromise.set_value(ec);
            },
            [&setPromise]() {
                setPromise.set_value(boost::system::error_code{});
            });
        return DbusEnvironment::waitForFuture(setPromise.get_future());
    }

    boost::system::error_code deleteReport(const std::string& path)
    {
        return call(path, Report::deleteIfaceName, "Delete");
    }
};

TEST_F(TestReport, verifyIfPropertiesHaveValidValue)
{
    EXPECT_THAT(getProperty<uint64_t>(sut->getPath(), "Interval"),
                Eq(defaultParams.interval().count()));
    EXPECT_THAT(getProperty<bool>(sut->getPath(), "Persistency"), Eq(true));
    EXPECT_THAT(getProperty<bool>(sut->getPath(), "EmitsReadingsUpdate"),
                Eq(defaultParams.emitReadingUpdate()));
    EXPECT_THAT(
        getProperty<bool>(sut->getPath(), "LogToMetricReportsCollection"),
        Eq(defaultParams.logToMetricReportCollection()));
    EXPECT_THAT(
        getProperty<ReadingParameters>(sut->getPath(), "ReadingParameters"),
        Eq(defaultParams.readingParameters()));
}

TEST_F(TestReport, readingsAreInitialyEmpty)
{
    EXPECT_THAT(getProperty<Readings>(sut->getPath(), "Readings"),
                Eq(Readings{}));
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
    Values(std::make_pair("Version"s, nlohmann::json(1)),
           std::make_pair("Name"s, nlohmann::json(ReportParams().reportName())),
           std::make_pair("ReportingType",
                          nlohmann::json(ReportParams().reportingType())),
           std::make_pair("EmitsReadingsUpdate",
                          nlohmann::json(ReportParams().emitReadingUpdate())),
           std::make_pair(
               "LogToMetricReportsCollection",
               nlohmann::json(ReportParams().logToMetricReportCollection())),
           std::make_pair("Interval",
                          nlohmann::json(ReportParams().interval().count())),
           std::make_pair("ReadingParameters",
                          nlohmann::json({"metric0", "metric1", "metric2"}))));

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
    void SetUp() override
    {
        sut = makeReport(GetParam());
    }
};

INSTANTIATE_TEST_SUITE_P(_, TestReportAllReportTypes,
                         Values(ReportParams().reportingType("OnRequest"),
                                ReportParams().reportingType("OnChange"),
                                ReportParams().reportingType("Periodic")));

TEST_P(TestReportAllReportTypes, returnPropertValueOfReportType)
{
    EXPECT_THAT(getProperty<std::string>(sut->getPath(), "ReportingType"),
                Eq(GetParam().reportingType()));
}

class TestReportOnRequestType : public TestReport
{
    void SetUp() override
    {
        sut = makeReport(ReportParams().reportingType("OnRequest"));
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
                            std::make_tuple("aaa"s, "bbb"s, 21.7, 100u),
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

INSTANTIATE_TEST_SUITE_P(_, TestReportNonOnRequestType,
                         Values(ReportParams().reportingType("Periodic"),
                                ReportParams().reportingType("OnChange")));

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
    void SetUp() override
    {
        sut = makeReport(GetParam());
    }
};

INSTANTIATE_TEST_SUITE_P(_, TestReportNonPeriodicReport,
                         Values(ReportParams().reportingType("OnRequest"),
                                ReportParams().reportingType("OnChange")));

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
        sut = makeReport(ReportParams().reportingType("Periodic"));
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
                            std::make_tuple("aaa"s, "bbb"s, 21.7, 100u),
                            std::make_tuple("aa"s, "bb"s, 42.0, 74u)));
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
        monitor = std::make_unique<sdbusplus::bus::match::match>(
            *DbusEnvironment::getBus(),
            sdbusplus::bus::match::rules::propertiesChanged(
                sut->getPath(), Report::reportIfaceName),
            [this](auto& msg) { monitorProc(msg); });
    }

    std::unique_ptr<sdbusplus::bus::match::match> monitor;
    MockFunction<void()> readingsUpdated;
};

TEST_F(TestReportInitialization, metricsAreInitializedWhenConstructed)
{
    for (auto& metric : metricMocks)
    {
        EXPECT_CALL(*metric, initialize());
    }

    sut = makeReport(ReportParams());
}

TEST_F(TestReportInitialization, readingsPropertiesChangedSingalEmits)
{
    sut = makeReport(defaultParams.reportingType("Periodic"));
    EXPECT_CALL(readingsUpdated, Call());
    makeMonitor();
    DbusEnvironment::sleepFor(defaultParams.interval() + 1ms);
}
