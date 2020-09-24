#include "mocks/sensor_mock.hpp"
#include "sensor_cache.hpp"

#include <gmock/gmock.h>

using namespace testing;

class TestSensorCache : public Test
{
  public:
    SensorCache sut;
};

TEST_F(TestSensorCache, shouldReturnDifferentSensorWhenIdsAreDifferent)
{
    auto sensor1 = sut.makeSensor<NiceMock<SensorMock>>("sensor-name-a");
    auto sensor2 = sut.makeSensor<NiceMock<SensorMock>>("sensor-name-b");

    ASSERT_THAT(sensor1.get(), Not(Eq(sensor2.get())));
    ASSERT_THAT(sensor1->mockId, Not(Eq(sensor2->mockId)));
}

TEST_F(TestSensorCache, shouldReturnSameSensorWhenSensorWithSameIdStillExists)
{
    auto sensor1 = sut.makeSensor<NiceMock<SensorMock>>("sensor-name");
    auto sensor2 = sut.makeSensor<NiceMock<SensorMock>>("sensor-name");

    ASSERT_THAT(sensor1.get(), Eq(sensor2.get()));
    ASSERT_THAT(sensor1->mockId, Eq(sensor2->mockId));
}

TEST_F(TestSensorCache, shouldReturnDifferentSensorWhenPreviousSensorExpired)
{
    auto mockId1 = sut.makeSensor<NiceMock<SensorMock>>("sensor-name")->mockId;
    auto mockId2 = sut.makeSensor<NiceMock<SensorMock>>("sensor-name")->mockId;

    ASSERT_THAT(mockId2, Not(Eq(mockId1)));
}

TEST_F(TestSensorCache, shouldCreateSensorWithCorrespondingId)
{
    auto id = sut.makeSensor<NiceMock<SensorMock>>("sensor-name")->id();

    auto expected = SensorMock::makeId("sensor-name");

    ASSERT_THAT(expected.type, Eq(id.type));
    ASSERT_THAT(expected.name, Eq(id.name));
}
