#include "dbus_environment.hpp"
#include "telemetry.hpp"

#include <future>
#include <iostream>

#include <gmock/gmock.h>

using namespace testing;
using namespace std::chrono_literals;

class TestReportManager : public Test, public Telemetry
{
  public:
    TestReportManager() : Telemetry(DbusEnvironment::getBus())
    {}

    struct AddReportArgs
    {
        std::string name;
        std::string reportingType;
        bool emitsReadingsUpdate;
        bool logToMetricReportCollection;
        uint64_t interval;
        ReadingParameters readingParams;

        std::string getReportPath()
        {
            return reportPath + name;
        }
    };

    AddReportArgs basicArgs = {"TestReport",
                               "Periodic",
                               true,
                               true,
                               ReportManager::minInterval.count(),
                               ReadingParameters{}};

    using AddReportRet = std::pair<boost::system::error_code, std::string>;

    AddReportRet addReport(const AddReportArgs& args)
    {
        std::promise<AddReportRet> addReportPromise;
        DbusEnvironment::getBus()->async_method_call(
            [&addReportPromise](boost::system::error_code ec,
                                const std::string& path) {
                addReportPromise.set_value({ec, path});
            },
            DbusEnvironment::serviceName(), reportManagerPath,
            reportManagerIfaceName, "AddReport", args.name, args.reportingType,
            args.emitsReadingsUpdate, args.logToMetricReportCollection,
            args.interval, args.readingParams);
        return addReportPromise.get_future().get();
    }

    boost::system::error_code deleteReport(const std::string& path)
    {
        std::promise<boost::system::error_code> deleteReportPromise;
        DbusEnvironment::getBus()->async_method_call(
            [&deleteReportPromise](boost::system::error_code ec) {
                deleteReportPromise.set_value(ec);
            },
            DbusEnvironment::serviceName(), path, deleteIfaceName, "Delete");
        return deleteReportPromise.get_future().get();
    }

    template <class T>
    auto getProperty(const std::string property) -> T
    {
        std::promise<T> propertyPromise;
        DbusEnvironment::getBus()->async_method_call(
            [&propertyPromise](boost::system::error_code ec,
                               const std::variant<T>& property) {
                EXPECT_THAT(static_cast<bool>(ec), Eq(false));
                if (!ec)
                {
                    propertyPromise.set_value(std::get<T>(property));
                }
                else
                {
                    propertyPromise.set_value(T{});
                }
            },
            DbusEnvironment::serviceName(), reportManagerPath,
            "org.freedesktop.DBus.Properties", "Get", reportManagerIfaceName,
            property);
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
    auto [ec, path] = addReport(basicArgs);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    EXPECT_THAT(path, Eq(basicArgs.getReportPath()));
    EXPECT_THAT(reportManager.getReportsSize(), Eq(1));
}

TEST_F(TestReportManager, addSameReportTwoTimes)
{
    addReport(basicArgs);

    auto [ec, path] = addReport(basicArgs);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::file_exists));
    EXPECT_THAT(path, Eq(std::string()));
    EXPECT_THAT(reportManager.getReportsSize(), Eq(1));
}

TEST_F(TestReportManager, addReportWithInvalidInterval)
{
    AddReportArgs args = basicArgs;
    args.interval = ReportManager::minInterval.count() - 1;

    auto [ec, path] = addReport(args);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::invalid_argument));
    EXPECT_THAT(path, Eq(std::string()));
    EXPECT_THAT(reportManager.getReportsSize(), Eq(0));
}

TEST_F(TestReportManager, addManyReportUpToMaxReportsAndOneMore)
{
    AddReportArgs args = basicArgs;
    for (size_t i = 0; i < ReportManager::maxReports; i++)
    {
        args.name = "TestReport" + std::to_string(i);

        auto [ec, path] = addReport(args);
        EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
        EXPECT_THAT(path, Eq(args.getReportPath()));
        EXPECT_THAT(reportManager.getReportsSize(), Eq(i + 1));
    }

    args.name = "TestReport" + std::to_string(ReportManager::maxReports);
    auto [ec, path] = addReport(args);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::too_many_files_open));
    EXPECT_THAT(path, Eq(std::string()));
    EXPECT_THAT(reportManager.getReportsSize(), Eq(ReportManager::maxReports));
}

TEST_F(TestReportManager, addReportAndDeleteIt)
{
    addReport(basicArgs);
    auto ec = deleteReport(basicArgs.getReportPath());
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
}

TEST_F(TestReportManager, deleteReportThatDoesNotExist)
{
    auto ec = deleteReport(basicArgs.getReportPath());
    EXPECT_THAT(ec.value(), Eq(EBADR));
}

TEST_F(TestReportManager, addReportAndDeleteItTwice)
{
    addReport(basicArgs);
    auto ec = deleteReport(basicArgs.getReportPath());
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    ec = deleteReport(basicArgs.getReportPath());
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
    AddReportArgs args = basicArgs;
    args.name = GetParam();
    auto [ec, path] = addReport(args);
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
    AddReportArgs args = basicArgs;
    args.name = GetParam();
    auto [ec, path] = addReport(args);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::invalid_argument));
    EXPECT_THAT(path, Eq(std::string()));
}
