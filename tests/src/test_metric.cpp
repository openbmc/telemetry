#include "helpers/metric_value_helpers.hpp"
#include "interfaces/sensor.hpp"
#include "metric.hpp"
#include "mocks/sensor_mock.hpp"
#include "printers.hpp"
#include "utils/conv_container.hpp"

#include <gmock/gmock.h>

using namespace testing;

using Timestamp = uint64_t;

class TestMetric : public Test
{
  public:
    std::vector<std::shared_ptr<SensorMock>> sensorMocks = {
        std::make_shared<NiceMock<SensorMock>>(),
        std::make_shared<NiceMock<SensorMock>>(),
        std::make_shared<NiceMock<SensorMock>>()};

    std::shared_ptr<Metric> sut = std::make_shared<Metric>(
        utils::convContainer<std::shared_ptr<interfaces::Sensor>>(sensorMocks),
        "op", "id", "metadata");
};

TEST_F(TestMetric, subscribesForSensorDuringInitialization)
{
    for (auto& sensor : sensorMocks)
    {
        EXPECT_CALL(*sensor,
                    registerForUpdates(Truly([sut = sut.get()](const auto& a0) {
                        return a0.lock().get() == sut;
                    })));
    }

    sut->initialize();
}

TEST_F(TestMetric, containsNoReadingsWhenNotInitialized)
{
    ASSERT_THAT(sut->getReadings(), ElementsAre());
}

TEST_F(TestMetric, containsEmptyReadingsAfterInitialize)
{
    sut->initialize();

    ASSERT_THAT(sut->getReadings(),
                ElementsAre(MetricValue{"id", "metadata", 0., 0u},
                            MetricValue{"id", "metadata", 0., 0u},
                            MetricValue{"id", "metadata", 0., 0u}));
}

TEST_F(TestMetric, throwsWhenUpdateIsPerformedWhenNotInitialized)
{
    EXPECT_THROW(sut->sensorUpdated(*sensorMocks[0], Timestamp{10}),
                 std::out_of_range);
    EXPECT_THROW(sut->sensorUpdated(*sensorMocks[1], Timestamp{10}, 20.0),
                 std::out_of_range);
}

TEST_F(TestMetric, updatesMetricValuesOnSensorUpdate)
{
    sut->initialize();

    sut->sensorUpdated(*sensorMocks[2], Timestamp{18}, 31.0);
    sut->sensorUpdated(*sensorMocks[0], Timestamp{21});

    ASSERT_THAT(sut->getReadings(),
                ElementsAre(MetricValue{"id", "metadata", 0., 21u},
                            MetricValue{"id", "metadata", 0., 0u},
                            MetricValue{"id", "metadata", 31., 18u}));
}

TEST_F(TestMetric, throwsWhenUpdateIsPerformedOnUnknownSensor)
{
    sut->initialize();

    auto sensor = std::make_shared<StrictMock<SensorMock>>();
    EXPECT_THROW(sut->sensorUpdated(*sensor, Timestamp{10}), std::out_of_range);
    EXPECT_THROW(sut->sensorUpdated(*sensor, Timestamp{10}, 20.0),
                 std::out_of_range);
}

TEST_F(TestMetric, containsIdInJsonDump)
{
    ASSERT_THAT(sut->to_json().at("id"), Eq(nlohmann::json("id")));
}

TEST_F(TestMetric, containsOpInJsonDump)
{
    ASSERT_THAT(sut->to_json().at("operationType"), Eq(nlohmann::json("op")));
}

TEST_F(TestMetric, containsMetadataInJsonDump)
{
    ASSERT_THAT(sut->to_json().at("metricMetadata"),
                Eq(nlohmann::json("metadata")));
}

TEST_F(TestMetric, containsSensorPathsInJsonDump)
{
    for (size_t i = 0; i < sensorMocks.size(); ++i)
    {
        const auto no = std::to_string(i);
        ON_CALL(*sensorMocks[i], id())
            .WillByDefault(
                Return(SensorMock::makeId("service" + no, "path" + no)));
    }

    ASSERT_THAT(sut->to_json().at("sensorPaths"),
                Eq(nlohmann::json(
                    {"service0:path0", "service1:path1", "service2:path2"})));
}
