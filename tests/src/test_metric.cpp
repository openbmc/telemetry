#include "helpers.hpp"
#include "metric.hpp"
#include "mocks/sensor_mock.hpp"
#include "params/metric_params.hpp"
#include "utils/conv_container.hpp"
#include "utils/conversion.hpp"
#include "utils/tstring.hpp"

#include <gmock/gmock.h>

using namespace testing;
using namespace std::chrono_literals;

namespace tstring = utils::tstring;

using Timestamp = uint64_t;

class TestMetric : public Test
{
  public:
    static std::vector<std::shared_ptr<SensorMock>>
        makeSensorMocks(size_t amount)
    {
        std::vector<std::shared_ptr<SensorMock>> result;
        for (size_t i = 0; i < amount; ++i)
        {
            result.emplace_back(std::make_shared<NiceMock<SensorMock>>());
        }
        return result;
    }

    std::shared_ptr<Metric> makeSut(const MetricParams& p)
    {
        return std::make_shared<Metric>(
            utils::convContainer<std::shared_ptr<interfaces::Sensor>>(
                sensorMocks),
            p.operationType(), p.id(), p.metadata(), p.collectionTimeScope(),
            p.collectionDuration());
    }

    MetricParams params =
        MetricParams()
            .id("id")
            .metadata("metadata")
            .operationType(OperationType::avg)
            .collectionTimeScope(CollectionTimeScope::interval)
            .collectionDuration(CollectionDuration(42ms));
    std::vector<std::shared_ptr<SensorMock>> sensorMocks = makeSensorMocks(1u);
    std::shared_ptr<Metric> sut;
};

TEST_F(TestMetric, subscribesForSensorDuringInitialization)
{
    sut = makeSut(params);

    EXPECT_CALL(*sensorMocks.front(),
                registerForUpdates(Truly([sut = sut.get()](const auto& a0) {
                    return a0.lock().get() == sut;
                })));

    sut->initialize();
}

TEST_F(TestMetric, containsEmptyReadingAfterCreated)
{
    sut = makeSut(params);

    ASSERT_THAT(sut->getReadings(),
                ElementsAre(MetricValue({"id", "metadata", 0., 0u})));
}

TEST_F(TestMetric, parsesSensorMetadata)
{
    nlohmann::json metadata;
    metadata["MetricProperties"] = {"sensor1", "sensor2"};

    sensorMocks = makeSensorMocks(2);
    sut = makeSut(params.metadata(metadata.dump()));

    EXPECT_THAT(
        sut->getReadings(),
        ElementsAre(MetricValue{"id",
                                nlohmann::json{{"SensorDbusPath", ""},
                                               {"SensorRedfishUri", "sensor1"}}
                                    .dump(),
                                0., 0u},
                    MetricValue{"id",
                                nlohmann::json{{"SensorDbusPath", ""},
                                               {"SensorRedfishUri", "sensor2"}}
                                    .dump(),
                                0., 0u}));
}

TEST_F(TestMetric, parsesSensorMetadataWhenMoreMetadataThanSensors)
{
    nlohmann::json metadata;
    metadata["MetricProperties"] = {"sensor1", "sensor2"};

    sensorMocks = makeSensorMocks(1);
    sut = makeSut(params.metadata(metadata.dump()));

    EXPECT_THAT(sut->getReadings(),
                ElementsAre(MetricValue{"id", metadata.dump(), 0., 0u}));
}

TEST_F(TestMetric, parsesSensorMetadataWhenMoreSensorsThanMetadata)
{
    nlohmann::json metadata;
    metadata["MetricProperties"] = {"sensor1"};

    sensorMocks = makeSensorMocks(2);
    sut = makeSut(params.metadata(metadata.dump()));

    EXPECT_THAT(sut->getReadings(),
                ElementsAre(MetricValue{"id", metadata.dump(), 0., 0u},
                            MetricValue{"id", metadata.dump(), 0., 0u}));
}

class TestMetricAfterInitialization : public TestMetric
{
  public:
    void SetUp() override
    {
        sut = makeSut(params);
        sut->initialize();
    }
};

TEST_F(TestMetricAfterInitialization, containsEmptyReading)
{
    ASSERT_THAT(sut->getReadings(),
                ElementsAre(MetricValue({"id", "metadata", 0., 0u})));
}

TEST_F(TestMetricAfterInitialization, updatesMetricValuesOnSensorUpdate)
{
    sut->sensorUpdated(*sensorMocks.front(), Timestamp{18}, 31.2);

    ASSERT_THAT(sut->getReadings(),
                ElementsAre(MetricValue{"id", "metadata", 31.2, 18u}));
}

TEST_F(TestMetricAfterInitialization,
       throwsWhenUpdateIsPerformedOnUnknownSensor)
{
    auto sensor = std::make_shared<StrictMock<SensorMock>>();
    EXPECT_THROW(sut->sensorUpdated(*sensor, Timestamp{10}), std::out_of_range);
    EXPECT_THROW(sut->sensorUpdated(*sensor, Timestamp{10}, 20.0),
                 std::out_of_range);
}

TEST_F(TestMetricAfterInitialization, dumpsConfiguration)
{
    namespace ts = utils::tstring;

    ON_CALL(*sensorMocks.front(), id())
        .WillByDefault(Return(SensorMock::makeId("service1", "path1")));

    const auto conf = sut->dumpConfiguration();

    LabeledMetricParameters expected = {};
    expected.at_label<ts::Id>() = "id";
    expected.at_label<ts::MetricMetadata>() = "metadata";
    expected.at_label<ts::OperationType>() = OperationType::avg;
    expected.at_label<ts::CollectionTimeScope>() =
        CollectionTimeScope::interval;
    expected.at_label<ts::CollectionDuration>() = CollectionDuration(42ms);
    expected.at_label<ts::SensorPath>() = {
        LabeledSensorParameters("service1", "path1")};

    EXPECT_THAT(conf, Eq(expected));
}
