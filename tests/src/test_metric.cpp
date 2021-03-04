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
    void SetUp() override
    {
        sut = std::make_shared<Metric>(sensorMock, OperationType::avg, "id",
                                       "metadata", CollectionTimeScope::point,
                                       CollectionDuration(0ms),
                                       std::move(clockFakePtr));
    }

    std::unique_ptr<ClockFake> clockFakePtr = std::make_unique<ClockFake>();
    ClockFake& clockFake = *clockFakePtr;
    std::shared_ptr<SensorMock> sensorMock =
        std::make_shared<NiceMock<SensorMock>>();

    std::shared_ptr<Metric> sut;
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
    void SetUp() override
    {
        TestMetric::SetUp();
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

class TestMetricCalculationFunctions :
    public TestMetric,
    public WithParamInterface<MetricParams>
{
  public:
    void SetUp() override
    {
        clockFakePtr->set(0ms);

        sut = std::make_shared<Metric>(
            sensorMock, GetParam().operationType(), "id", "metadata",
            GetParam().collectionTimeScope(), GetParam().collectionDuration(),
            std::move(clockFakePtr));
    }

    static std::vector<std::pair<std::chrono::milliseconds, double>>
        defaultReadings()
    {
        std::vector<std::pair<std::chrono::milliseconds, double>> ret;
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
        sut->sensorUpdated(*sensorMock, clockFake.timestamp(), reading);
        clockFake.advance(timestamp);
    }

    const auto [expectedTimestamp, expectedReading] =
        GetParam().expectedReading();
    const auto [id, metadata, reading, timestamp] = sut->getReading();

    EXPECT_THAT(id, Eq("id"));
    EXPECT_THAT(metadata, Eq("metadata"));
    EXPECT_THAT(reading, DoubleEq(expectedReading));
    EXPECT_THAT(timestamp, Eq(ClockFake::toTimestamp(expectedTimestamp)));
}

TEST_P(TestMetricCalculationFunctions,
       calculatedReadingValueWithIntermediateCalculations)
{
    for (auto [timestamp, reading] : GetParam().readings())
    {
        sut->sensorUpdated(*sensorMock, clockFake.timestamp(), reading);
        clockFake.advance(timestamp);
        sut->getReading();
    }

    const auto [expectedTimestamp, expectedReading] =
        GetParam().expectedReading();
    const auto [id, metadata, reading, timestamp] = sut->getReading();

    EXPECT_THAT(id, Eq("id"));
    EXPECT_THAT(metadata, Eq("metadata"));
    EXPECT_THAT(reading, DoubleEq(expectedReading));
    EXPECT_THAT(timestamp, Eq(ClockFake::toTimestamp(expectedTimestamp)));
}
