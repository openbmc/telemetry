#include "fakes/clock_fake.hpp"
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
            p.collectionDuration(), std::move(clockFakePtr));
    }

    MetricParams params = MetricParams()
                              .id("id")
                              .metadata("metadata")
                              .operationType(OperationType::avg)
                              .collectionTimeScope(CollectionTimeScope::point)
                              .collectionDuration(CollectionDuration(0ms));
    std::vector<std::shared_ptr<SensorMock>> sensorMocks = makeSensorMocks(1u);
    std::unique_ptr<ClockFake> clockFakePtr = std::make_unique<ClockFake>();
    ClockFake& clockFake = *clockFakePtr;
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

    EXPECT_THAT(sut->getReadings(),
                ElementsAre(MetricValue{"id", "sensor1", 0., 0u},
                            MetricValue{"id", "sensor2", 0., 0u}));
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
    expected.at_label<ts::Id>() = params.id();
    expected.at_label<ts::MetricMetadata>() = params.metadata();
    expected.at_label<ts::OperationType>() = params.operationType();
    expected.at_label<ts::CollectionTimeScope>() = params.collectionTimeScope();
    expected.at_label<ts::CollectionDuration>() = params.collectionDuration();
    expected.at_label<ts::SensorPath>() = {
        LabeledSensorParameters("service1", "path1")};

    EXPECT_THAT(conf, Eq(expected));
}

class TestMetricCalculationFunctions :
    public TestMetric,
    public WithParamInterface<MetricParams>
{
  public:
    void SetUp() override
    {
        clockFakePtr->set(0ms);

        sut = makeSut(params.operationType(GetParam().operationType())
                          .collectionTimeScope(GetParam().collectionTimeScope())
                          .collectionDuration(GetParam().collectionDuration()));
    }

    static std::vector<std::pair<Milliseconds, double>> defaultReadings()
    {
        std::vector<std::pair<Milliseconds, double>> ret;
        ret.emplace_back(10ms, 14.);
        ret.emplace_back(1ms, 3.);
        ret.emplace_back(5ms, 7.);
        return ret;
    }
};

MetricParams defaultSingleParams()
{
    return MetricParams()
        .operationType(OperationType::single)
        .readings(TestMetricCalculationFunctions::defaultReadings())
        .expectedReading(11ms, 7.0);
}

INSTANTIATE_TEST_SUITE_P(
    OperationSingleReturnsLastReading, TestMetricCalculationFunctions,
    Values(
        defaultSingleParams().collectionTimeScope(CollectionTimeScope::point),
        defaultSingleParams()
            .collectionTimeScope(CollectionTimeScope::interval)
            .collectionDuration(CollectionDuration(100ms)),
        defaultSingleParams().collectionTimeScope(
            CollectionTimeScope::startup)));

MetricParams defaultPointParams()
{
    return defaultSingleParams().collectionTimeScope(
        CollectionTimeScope::point);
}

INSTANTIATE_TEST_SUITE_P(
    TimeScopePointReturnsLastReading, TestMetricCalculationFunctions,
    Values(defaultPointParams().operationType(OperationType::single),
           defaultPointParams().operationType(OperationType::min),
           defaultPointParams().operationType(OperationType::max),
           defaultPointParams().operationType(OperationType::sum),
           defaultPointParams().operationType(OperationType::avg)));

MetricParams defaultMinParams()
{
    return defaultSingleParams().operationType(OperationType::min);
}

INSTANTIATE_TEST_SUITE_P(
    ReturnsMinForGivenTimeScope, TestMetricCalculationFunctions,
    Values(defaultMinParams()
               .collectionTimeScope(CollectionTimeScope::interval)
               .collectionDuration(CollectionDuration(100ms))
               .expectedReading(10ms, 3.0),
           defaultMinParams()
               .collectionTimeScope(CollectionTimeScope::interval)
               .collectionDuration(CollectionDuration(3ms))
               .expectedReading(13ms, 7.0),
           defaultMinParams()
               .collectionTimeScope(CollectionTimeScope::startup)
               .expectedReading(10ms, 3.0)));

MetricParams defaultMaxParams()
{
    return defaultSingleParams().operationType(OperationType::max);
}

INSTANTIATE_TEST_SUITE_P(
    ReturnsMaxForGivenTimeScope, TestMetricCalculationFunctions,
    Values(defaultMaxParams()
               .collectionTimeScope(CollectionTimeScope::interval)
               .collectionDuration(CollectionDuration(100ms))
               .expectedReading(0ms, 14.0),
           defaultMaxParams()
               .collectionTimeScope(CollectionTimeScope::interval)
               .collectionDuration(CollectionDuration(6ms))
               .expectedReading(10ms, 14.0),
           defaultMaxParams()
               .collectionTimeScope(CollectionTimeScope::interval)
               .collectionDuration(CollectionDuration(5ms))
               .expectedReading(11ms, 7.0),
           defaultMaxParams()
               .collectionTimeScope(CollectionTimeScope::startup)
               .expectedReading(0ms, 14.0)));

MetricParams defaultSumParams()
{
    return defaultSingleParams().operationType(OperationType::sum);
}

INSTANTIATE_TEST_SUITE_P(
    ReturnsSumForGivenTimeScope, TestMetricCalculationFunctions,
    Values(defaultSumParams()
               .collectionTimeScope(CollectionTimeScope::interval)
               .collectionDuration(CollectionDuration(100ms))
               .expectedReading(16ms, 14. * 10 + 3. * 1 + 7 * 5),
           defaultSumParams()
               .collectionTimeScope(CollectionTimeScope::interval)
               .collectionDuration(CollectionDuration(8ms))
               .expectedReading(16ms, 14. * 2 + 3. * 1 + 7 * 5),
           defaultSumParams()
               .collectionTimeScope(CollectionTimeScope::interval)
               .collectionDuration(CollectionDuration(6ms))
               .expectedReading(16ms, 3. * 1 + 7 * 5),
           defaultSumParams()
               .collectionTimeScope(CollectionTimeScope::startup)
               .expectedReading(16ms, 14. * 10 + 3. * 1 + 7 * 5)));

MetricParams defaultAvgParams()
{
    return defaultSingleParams().operationType(OperationType::avg);
}

INSTANTIATE_TEST_SUITE_P(
    ReturnsAvgForGivenTimeScope, TestMetricCalculationFunctions,
    Values(defaultAvgParams()
               .collectionTimeScope(CollectionTimeScope::interval)
               .collectionDuration(CollectionDuration(100ms))
               .expectedReading(16ms, (14. * 10 + 3. * 1 + 7 * 5) / 16.),
           defaultAvgParams()
               .collectionTimeScope(CollectionTimeScope::interval)
               .collectionDuration(CollectionDuration(8ms))
               .expectedReading(16ms, (14. * 2 + 3. * 1 + 7 * 5) / 8.),
           defaultAvgParams()
               .collectionTimeScope(CollectionTimeScope::interval)
               .collectionDuration(CollectionDuration(6ms))
               .expectedReading(16ms, (3. * 1 + 7 * 5) / 6.),
           defaultAvgParams()
               .collectionTimeScope(CollectionTimeScope::startup)
               .expectedReading(16ms, (14. * 10 + 3. * 1 + 7 * 5) / 16.)));

TEST_P(TestMetricCalculationFunctions, calculatesReadingValue)
{
    for (auto [timestamp, reading] : GetParam().readings())
    {
        sut->sensorUpdated(*sensorMocks.front(), clockFake.timestamp(),
                           reading);
        clockFake.advance(timestamp);
    }

    const auto [expectedTimestamp, expectedReading] =
        GetParam().expectedReading();
    const auto readings = sut->getReadings();

    EXPECT_THAT(readings, ElementsAre(MetricValue{
                              "id", "metadata", expectedReading,
                              ClockFake::toTimestamp(expectedTimestamp)}));
}

TEST_P(TestMetricCalculationFunctions,
       calculatedReadingValueWithIntermediateCalculations)
{
    for (auto [timestamp, reading] : GetParam().readings())
    {
        sut->sensorUpdated(*sensorMocks.front(), clockFake.timestamp(),
                           reading);
        clockFake.advance(timestamp);
        sut->getReadings();
    }

    const auto [expectedTimestamp, expectedReading] =
        GetParam().expectedReading();
    const auto readings = sut->getReadings();

    EXPECT_THAT(readings, ElementsAre(MetricValue{
                              "id", "metadata", expectedReading,
                              ClockFake::toTimestamp(expectedTimestamp)}));
}
