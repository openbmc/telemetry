#include "helpers.hpp"
#include "metric.hpp"
#include "mocks/sensor_mock.hpp"
#include "utils/conv_container.hpp"
#include "utils/conversion.hpp"
#include "utils/tstring.hpp"

#include <gmock/gmock.h>

using namespace testing;
namespace tstring = utils::tstring;

using Timestamp = uint64_t;

class TestMetric : public Test
{
  public:
    std::shared_ptr<SensorMock> sensorMock =
        std::make_shared<NiceMock<SensorMock>>();

    std::shared_ptr<Metric> sut = std::make_shared<Metric>(
        sensorMock, OperationType::avg, "id", "metadata");
};

TEST_F(TestMetric, subscribesForSensorDuringInitialization)
{
    EXPECT_CALL(*sensorMock,
                registerForUpdates(Truly([sut = sut.get()](const auto& a0) {
                    return a0.lock().get() == sut;
                })));

    sut->initialize();
}

TEST_F(TestMetric, containsEmptyReadingAfterCreated)
{
    ASSERT_THAT(sut->getReading(), MetricValue({"id", "metadata", 0., 0u}));
}

class TestMetricAfterInitialization : public TestMetric
{
  public:
    TestMetricAfterInitialization()
    {
        sut->initialize();
    }
};

TEST_F(TestMetricAfterInitialization, containsEmptyReading)
{
    ASSERT_THAT(sut->getReading(), MetricValue({"id", "metadata", 0., 0u}));
}

TEST_F(TestMetricAfterInitialization, updatesMetricValuesOnSensorUpdate)
{
    sut->sensorUpdated(*sensorMock, Timestamp{18}, 31.2);

    ASSERT_THAT(sut->getReading(),
                Eq(MetricValue{"id", "metadata", 31.2, 18u}));
}

TEST_F(TestMetricAfterInitialization,
       throwsWhenUpdateIsPerformedOnUnknownSensor)
{
    auto sensor = std::make_shared<StrictMock<SensorMock>>();
    EXPECT_THROW(sut->sensorUpdated(*sensor, Timestamp{10}), std::out_of_range);
    EXPECT_THROW(sut->sensorUpdated(*sensor, Timestamp{10}, 20.0),
                 std::out_of_range);
}

TEST_F(TestMetricAfterInitialization, containsIdInConfigurationDump)
{
    const auto conf = sut->dumpConfiguration();

    EXPECT_THAT(conf.at_label<utils::tstring::Id>(), Eq("id"));
    EXPECT_THAT(conf.to_json().at(tstring::Id::str()),
                Eq(nlohmann::json("id")));
}

TEST_F(TestMetricAfterInitialization, containsOpInJsonDump)
{
    const auto conf = sut->dumpConfiguration();

    EXPECT_THAT(conf.at_label<utils::tstring::OperationType>(),
                Eq(OperationType::avg));
    EXPECT_THAT(conf.to_json().at(tstring::OperationType::str()),
                Eq(nlohmann::json(utils::toUnderlying(OperationType::avg))));
}

TEST_F(TestMetricAfterInitialization, containsMetadataInJsonDump)
{
    const auto conf = sut->dumpConfiguration();

    EXPECT_THAT(conf.at_label<utils::tstring::MetricMetadata>(),
                Eq("metadata"));
    EXPECT_THAT(conf.to_json().at(tstring::MetricMetadata::str()),
                Eq(nlohmann::json("metadata")));
}

TEST_F(TestMetricAfterInitialization, containsSensorPathInJsonDump)
{
    ON_CALL(*sensorMock, id())
        .WillByDefault(Return(SensorMock::makeId("service1", "path1")));

    const auto conf = sut->dumpConfiguration();

    EXPECT_THAT(conf.at_label<utils::tstring::SensorPath>(),
                Eq(LabeledSensorParameters("service1", "path1")));
    EXPECT_THAT(
        conf.to_json().at(tstring::SensorPath::str()),
        Eq(nlohmann::json({{"service", "service1"}, {"path", "path1"}})));
}
