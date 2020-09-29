#include "dbus_environment.hpp"
#include "mocks/sensor_listener_mock.hpp"
#include "sensor.hpp"
#include "sensor_cache.hpp"
#include "stubs/sensor_service.hpp"
#include "utils/sensor_id_eq.hpp"

#include <thread>

#include <gmock/gmock.h>

using namespace testing;
using namespace std::chrono_literals;

class TestSensor : public Test
{
  public:
    void SetUp() override
    {
        sensorService.value = 42.7;
    }

    stubs::SensorService sensorService{DbusEnvironment::getIoc(),
                                       DbusEnvironment::getBus(),
                                       DbusEnvironment::getObjServer()};

    SensorCache sensorCache;
    std::shared_ptr<Sensor> sut = sensorCache.makeSensor<Sensor>(
        DbusEnvironment::serviceName(), sensorService.path(),
        DbusEnvironment::getIoc(), DbusEnvironment::getBus());
    std::shared_ptr<SensorListenerMock> listenerMock =
        std::make_shared<NiceMock<SensorListenerMock>>();
};

TEST_F(TestSensor, createsCorretlyViaSensorCache)
{
    ASSERT_THAT(sut->id(),
                sensorIdEq(Sensor::Id("Sensor", DbusEnvironment::serviceName(),
                                      sensorService.path())));
}

TEST_F(TestSensor, notifiesWithoutValueWhenThereIsNoValueDuringRegister)
{
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut)));

    sut->registerForUpdates(listenerMock);

    DbusEnvironment::sleep(10ms);
}

TEST_F(TestSensor, notifiesWithValueWhenThereIsValueDuringRegister)
{
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut), 42.7));

    sut->async_read();

    DbusEnvironment::sleep(10ms);

    sut->registerForUpdates(listenerMock);
}

TEST_F(TestSensor, notifiesListenerWithValueWhenChangeOccurs)
{
    InSequence _;
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut)));
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut), 42.7));

    sut->registerForUpdates(listenerMock);
    sut->async_read();

    DbusEnvironment::sleep(10ms);
}

TEST_F(TestSensor, notifiesListenerWithoutValueWhenNoChangeOccurs)
{
    InSequence _;
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut)));
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut), 42.7));
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut)));

    sut->registerForUpdates(listenerMock);
    sut->async_read();
    sut->async_read();

    DbusEnvironment::sleep(10ms);
}

TEST_F(TestSensor, doesntNotifyExpiredListener)
{
    {
        auto listener = std::make_shared<StrictMock<SensorListenerMock>>();
        EXPECT_CALL(*listener, sensorUpdated(Ref(*sut)));
        sut->registerForUpdates(listener);
    }

    sut->async_read();

    DbusEnvironment::sleep(10ms);
}

TEST_F(TestSensor, notifiesInGivenIntervalAfterSchedule)
{
    sut->async_read();

    DbusEnvironment::sleep(5ms);

    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut), 42.7));
    sut->registerForUpdates(listenerMock);

    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut))).Times(5);
    sut->schedule(10ms);

    DbusEnvironment::sleep(51ms);

    sut->stop();
}
