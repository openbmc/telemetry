#include "dbus_environment.hpp"
#include "mocks/report_manager_mock.hpp"
#include "report.hpp"
#include "report_manager.hpp"

#include <sdbusplus/exception.hpp>

using namespace testing;
using namespace std::literals::string_literals;

class TestReport : public Test
{
  public:
    std::string defaultReportName = "TestReport";
    std::string defaultReportType = "Periodic";
    bool defaultEmitReadingSignal = true;
    bool defaultLogToMetricReportCollection = true;
    uint64_t defaultInterval = ReportManager::minInterval.count();
    ReadingParameters defaultReadingParams = {};

    std::unique_ptr<ReportManagerMock> reportManagerMock =
        std::make_unique<StrictMock<ReportManagerMock>>();
    Report sut =
        Report(DbusEnvironment::getIoc(), DbusEnvironment::getObjServer(),
               defaultReportName, defaultReportType, defaultEmitReadingSignal,
               defaultLogToMetricReportCollection,
               std::chrono::milliseconds{defaultInterval}, defaultReadingParams,
               *reportManagerMock);

    template <class T>
    static T getProperty(const std::string& path, const std::string& property)
    {
        std::promise<T> propertyPromise;
        sdbusplus::asio::getProperty<T>(
            *DbusEnvironment::getBus(), DbusEnvironment::serviceName(), path,
            Report::reportIfaceName, property,
            [&propertyPromise](boost::system::error_code ec) {
                EXPECT_THAT(static_cast<bool>(ec), ::testing::Eq(false));
                propertyPromise.set_value(T{});
            },
            [&propertyPromise](T t) { propertyPromise.set_value(t); });
        return propertyPromise.get_future().get();
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
        return setPromise.get_future().get();
    }

    boost::system::error_code deleteReport(const std::string& path)
    {
        std::promise<boost::system::error_code> deleteReportPromise;
        DbusEnvironment::getBus()->async_method_call(
            [&deleteReportPromise](boost::system::error_code ec) {
                deleteReportPromise.set_value(ec);
            },
            DbusEnvironment::serviceName(), path, Report::deleteIfaceName,
            "Delete");
        return deleteReportPromise.get_future().get();
    }
};

TEST_F(TestReport, verifyIfPropertiesHaveValidValue)
{
    EXPECT_THAT(getProperty<uint64_t>(sut.getPath(), "Interval"),
                Eq(defaultInterval));
    EXPECT_THAT(getProperty<bool>(sut.getPath(), "Persistency"), Eq(false));
    EXPECT_THAT(getProperty<std::string>(sut.getPath(), "ReportingType"),
                Eq(defaultReportType));
    EXPECT_THAT(getProperty<bool>(sut.getPath(), "EmitsReadingsUpdate"),
                Eq(defaultEmitReadingSignal));
    EXPECT_THAT(
        getProperty<bool>(sut.getPath(), "LogToMetricReportsCollection"),
        Eq(defaultLogToMetricReportCollection));
    EXPECT_THAT(
        getProperty<ReadingParameters>(sut.getPath(), "ReadingParameters"),
        Eq(defaultReadingParams));
}

TEST_F(TestReport, setIntervalWithValidValue)
{
    uint64_t newValue = defaultInterval + 1;
    EXPECT_THAT(setProperty(sut.getPath(), "Interval", newValue).value(),
                Eq(boost::system::errc::success));
    EXPECT_THAT(getProperty<uint64_t>(sut.getPath(), "Interval"), Eq(newValue));
}

TEST_F(TestReport, settingIntervalWithInvalidValueDoesNotChangeProperty)
{
    uint64_t newValue = defaultInterval - 1;
    EXPECT_THAT(setProperty(sut.getPath(), "Interval", newValue).value(),
                Eq(boost::system::errc::success));
    EXPECT_THAT(getProperty<uint64_t>(sut.getPath(), "Interval"),
                Eq(defaultInterval));
}

TEST_F(TestReport, deleteReport)
{
    EXPECT_CALL(*reportManagerMock, removeReport(&sut));
    auto ec = deleteReport(sut.getPath());
    EXPECT_THAT(ec, Eq(boost::system::errc::success));
}

TEST_F(TestReport, deletingNonExistingReportReturnInvalidRequestDescriptor)
{
    auto ec = deleteReport(Report::reportDir + "NonExisting"s);
    EXPECT_THAT(ec.value(), Eq(EBADR));
}

class TestReportCreation : public Test
{
  public:
    std::unique_ptr<ReportManagerMock> reportManagerMock =
        std::make_unique<StrictMock<ReportManagerMock>>();

    std::unique_ptr<Report> createReportWithName(std::string name)
    {
        return std::make_unique<Report>(
            DbusEnvironment::getIoc(), DbusEnvironment::getObjServer(), name,
            "", true, true,
            std::chrono::milliseconds{ReportManager::minInterval.count()},
            ReadingParameters{}, *reportManagerMock);
    }
};

class TestReportValidNames :
    public TestReportCreation,
    public WithParamInterface<const char*>
{};

INSTANTIATE_TEST_SUITE_P(ValidNames, TestReportValidNames,
                         ValuesIn({"Valid_1", "Valid_1/Valid_2",
                                   "Valid_1/Valid_2/Valid_3"}));

TEST_P(TestReportValidNames, reportCtorDoesNotThrowOnValidName)
{
    EXPECT_NO_THROW(createReportWithName(GetParam()));
}

class TestReportInvalidNames :
    public TestReportCreation,
    public WithParamInterface<const char*>
{};

INSTANTIATE_TEST_SUITE_P(InvalidNames, TestReportInvalidNames,
                         ValuesIn({"/", "/Invalid", "Invalid/",
                                   "Invalid/Invalid/", "Invalid?", "?"}));

TEST_P(TestReportInvalidNames, reportCtorThrowOnInvalidName)
{
    EXPECT_THROW(createReportWithName(GetParam()),
                 sdbusplus::exception::SdBusError);
}
