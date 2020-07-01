#include "../actions.hpp"
#include "../core/mocks/metric.hpp"
#include "../core/mocks/report.hpp"
#include "../core/mocks/sensor.hpp"
#include "../core/mocks/storage.hpp"
#include "../utils/transform_to.hpp"
#include "../utils/tstring.hpp"
#include "../utils/tuple_wrapper.hpp"
#include "dbus/dbus.hpp"
#include "dbus/report.hpp"
#include "dbus/report_configuration.hpp"

#include <gmock/gmock.h>

namespace dbus
{
using namespace testing;
using namespace std::chrono_literals;
using namespace std::string_literals;

using MetricJson =
    utils::TupleWrapper<dbus::ReadingParam, utils::tstring::SensorPaths,
                        utils::tstring::OperationType, utils::tstring::Id,
                        utils::tstring::MetricMetadata>;
using ObjPath = sdbusplus::message::object_path;

class DBusReportTest : public Test
{
  public:
    DBusReportTest()
    {
        EXPECT_CALL(*storage,
                    store(std::filesystem::path(
                              "Report/Domain/Name/configuration.json"),
                          _));

        sut = std::make_shared<typeof(*sut)>(bus, objServer, coreReport,
                                             core::ReportName("Domain", "Name"),
                                             storage);
    }

    void SetUp() override
    {
        coreMetrics =
            utils::transformTo<std::shared_ptr<core::interfaces::Metric>>(
                metrics);

        for (size_t i = 0; i < metrics.size(); ++i)
        {
            std::vector<std::shared_ptr<core::SensorMock>> sensors;

            for (size_t k = 0; k < 2; ++k)
            {
                auto sensor = std::make_shared<NiceMock<core::SensorMock>>();
                ON_CALL(*sensor->id_, str())
                    .WillByDefault(ReturnStrView("id/"s + std::to_string(i) +
                                                 "."s + std::to_string(k)));
                sensors.emplace_back(std::move(sensor));
            }

            ON_CALL(*metrics[i], id())
                .WillByDefault(ReturnStrView("Metric-"s + std::to_string(i)));
            ON_CALL(*metrics[i], type())
                .WillByDefault(Return(core::OperationType::average));
            ON_CALL(*metrics[i], metadata())
                .WillByDefault(ReturnStrView("metadata-"s + std::to_string(i)));
            ON_CALL(*metrics[i], sensors())
                .WillByDefault(ReturnRefOfCopy(
                    utils::transformTo<
                        std::shared_ptr<core::interfaces::Sensor>>(sensors)));
        }

        ON_CALL(*coreReport, reportingType())
            .WillByDefault(Return(core::ReportingType::onChange));
        ON_CALL(*coreReport, scanPeriod())
            .WillByDefault(Return(std::chrono::milliseconds(1042u)));
        ON_CALL(*coreReport, reportAction())
            .WillByDefault(ReturnRefOfCopy(std::vector<core::ReportAction>(
                {core::ReportAction::log, core::ReportAction::event,
                 core::ReportAction::event})));
        ON_CALL(*coreReport, metrics())
            .WillByDefault(
                ReturnSpan<const std::shared_ptr<core::interfaces::Metric>>(
                    metrics));
    }

    void TearDown() override
    {
        run(10ms);
    }

    void run(std::chrono::milliseconds timeout)
    {
        if (runExecuted)
        {
            return;
        }

        auto t = std::thread([this, timeout] {
            std::this_thread::sleep_for(timeout);
            ioc.stop();
        });

        ioc.run();
        t.join();

        runExecuted = true;
    }

    static const std::string path;

    bool runExecuted = false;

    boost::asio::io_context ioc;
    std::shared_ptr<sdbusplus::asio::connection> bus =
        std::make_shared<typeof(*bus)>(ioc);
    std::shared_ptr<sdbusplus::asio::object_server> objServer =
        std::make_shared<typeof(*objServer)>(bus);

    std::shared_ptr<core::ReportMock> coreReport =
        std::make_shared<NiceMock<core::ReportMock>>();
    std::shared_ptr<core::StorageMock> storage =
        std::make_shared<StrictMock<core::StorageMock>>();
    std::vector<std::shared_ptr<core::MetricMock>> metrics = {
        std::make_shared<NiceMock<core::MetricMock>>(ioc),
        std::make_shared<NiceMock<core::MetricMock>>(ioc)};

    std::vector<std::shared_ptr<core::interfaces::Metric>> coreMetrics;

    std::shared_ptr<dbus::Report> sut;
};

const std::string DBusReportTest::path =
    "/xyz/openbmc_project/MonitoringService_ut";

TEST_F(DBusReportTest, returnListOfReports)
{
    EXPECT_CALL(*storage, list(std::filesystem::path("Report")))
        .WillRepeatedly(Return(std::vector<std::filesystem::path>(
            {"Report/Domain/Name1", "Report/Domain/Name2"})));

    ASSERT_THAT(ReportConfiguration::list(*storage),
                ElementsAre("Report/Domain/Name1", "Report/Domain/Name2"));
}

TEST_F(DBusReportTest, returnPathToConfigurationFileFromFilesystemPath)
{
    ASSERT_THAT(ReportConfiguration::path(
                    std::filesystem::path("tmp/Report/Domain/Name")),
                Eq("tmp/Report/Domain/Name/configuration.json"));
}

TEST_F(DBusReportTest, returnPathToConfigurationFile)
{
    ASSERT_THAT(ReportConfiguration::path(core::ReportName("Domain", "Name")),
                Eq("Report/Domain/Name/configuration.json"));
}

TEST_F(DBusReportTest, removesConfigurationWhenPersistencyIsChangedToNone)
{
    EXPECT_CALL(*storage, remove(std::filesystem::path(
                              "Report/Domain/Name/configuration.json")));

    sut->setPersistency(core::Persistency::none);
}

TEST_F(DBusReportTest, removesConfigurationWhenStoreIsCallenWithNonePersistency)
{
    EXPECT_CALL(*storage, remove(std::filesystem::path(
                              "Report/Domain/Name/configuration.json")))
        .Times(2);

    sut->setPersistency(core::Persistency::none);
    sut->store();
}

TEST_F(DBusReportTest, storesConfigurationWhenPersistencyIsChanged)
{
    EXPECT_CALL(*storage, store(std::filesystem::path(
                                    "Report/Domain/Name/configuration.json"),
                                _));

    sut->setPersistency(core::Persistency::configurationAndData);
}

TEST_F(DBusReportTest, noActionWhenPersistencyStaysTheSame)
{
    sut->setPersistency(core::Persistency::configurationOnly);
}

TEST_F(DBusReportTest, createsReportConfigurationFromGeneratedJson)
{
    auto actual = nlohmann::json();

    EXPECT_CALL(*storage, store(std::filesystem::path(
                                    "Report/Domain/Name/configuration.json"),
                                _))
        .WillOnce(SaveArg<1>(&actual));

    sut->store();

    auto reportConfiguration = ReportConfiguration(actual);

    EXPECT_THAT(reportConfiguration.domain, Eq("Domain"));
    EXPECT_THAT(reportConfiguration.name, Eq("Name"));
    EXPECT_THAT(reportConfiguration.reportingType, Eq("OnChange"));
    EXPECT_THAT(reportConfiguration.reportAction,
                ElementsAre("Log", "Event", "Event"));
    EXPECT_THAT(reportConfiguration.scanPeriod, Eq(1042u));
    EXPECT_THAT(reportConfiguration.persistency, Eq("ConfigurationOnly"));
    EXPECT_THAT(reportConfiguration.metricParams,
                ElementsAre(MetricParams(api::SensorPaths({ObjPath("id/0.0"),
                                                           ObjPath("id/0.1")}),
                                         "AVERAGE", "Metric-0", "metadata-0"),
                            MetricParams(api::SensorPaths({ObjPath("id/1.0"),
                                                           ObjPath("id/1.1")}),
                                         "AVERAGE", "Metric-1", "metadata-1")));
}

struct StoreParams
{
    std::string key;
    nlohmann::json expected;

    friend std::ostream& operator<<(std::ostream& os, const StoreParams& o)
    {
        return os << "{" << o.key << ": " << o.expected << "}";
    }
};

struct DBusReportTestStore :
    public DBusReportTest,
    public WithParamInterface<StoreParams>
{};

INSTANTIATE_TEST_SUITE_P(
    StoresKey, DBusReportTestStore,
    Values(StoreParams{"name", nlohmann::json("Name")},
           StoreParams{"domain", nlohmann::json("Domain")},
           StoreParams{"reportingType", nlohmann::json("OnChange")},
           StoreParams{"reportAction", nlohmann::json(std::vector<std::string>(
                                           {"Log", "Event", "Event"}))},
           StoreParams{"scanPeriod", nlohmann::json(1042u)},
           StoreParams{"persistency", nlohmann::json("ConfigurationOnly")},
           StoreParams{"version", nlohmann::json(1u)},
           StoreParams{"metricParams",
                       nlohmann::json(
                           {MetricJson({ObjPath("id/0.0"), ObjPath("id/0.1")},
                                       "AVERAGE"s, "Metric-0"s, "metadata-0"s),
                            MetricJson({ObjPath("id/1.0"), ObjPath("id/1.1")},
                                       "AVERAGE"s, "Metric-1"s, "metadata-1"s)})

           }));

TEST_P(DBusReportTestStore, storesConfiguration)
{
    auto actual = nlohmann::json();

    EXPECT_CALL(*storage, store(std::filesystem::path(
                                    "Report/Domain/Name/configuration.json"),
                                _))
        .WillOnce(SaveArg<1>(&actual));

    sut->store();

    auto it = actual.find(GetParam().key);
    ASSERT_THAT(it, Ne(std::end(actual)));
    ASSERT_THAT(*it, Eq(GetParam().expected)) << "key: " << GetParam().key;
}

} // namespace dbus
