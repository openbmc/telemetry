#include "dbus_environment.hpp"
#include "mocks/sensor_listener_mock.hpp"
#include "sensor.hpp"
#include "sensor_cache.hpp"
#include "stubs/dbus_sensor_object.hpp"
#include "utils/measure_elapsed_time.hpp"
#include "utils/sensor_id_eq.hpp"

#include <sdbusplus/asio/property.hpp>

#include <thread>

#include <gmock/gmock.h>

using namespace testing;
using namespace std::chrono_literals;

class TestSensor : public Test
{
  public:
    void SetUp() override
    {
        sensorObject.setValue(42.7);
    }

    ~TestSensor()
    {
        DbusEnvironment::synchronizeIoc();
    }

    std::chrono::milliseconds notifiesInGivenIntervalAfterSchedule(
        std::chrono::milliseconds interval);

    stubs::DbusSensorObject sensorObject{DbusEnvironment::getIoc(),
                                         DbusEnvironment::getBus(),
                                         DbusEnvironment::getObjServer()};

    SensorCache sensorCache;
    std::shared_ptr<Sensor> sut = sensorCache.makeSensor<Sensor>(
        DbusEnvironment::serviceName(), sensorObject.path(),
        DbusEnvironment::getIoc(), DbusEnvironment::getBus());
    std::shared_ptr<SensorListenerMock> listenerMock =
        std::make_shared<StrictMock<SensorListenerMock>>();
    std::shared_ptr<SensorListenerMock> listenerMock2 =
        std::make_shared<StrictMock<SensorListenerMock>>();
};

TEST_F(TestSensor, createsCorretlyViaSensorCache)
{
    ASSERT_THAT(sut->id(),
                sensorIdEq(Sensor::Id("Sensor", DbusEnvironment::serviceName(),
                                      sensorObject.path())));
}

TEST_F(TestSensor, notifiesWithValueAfterRegister)
{
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut), 42.7))
        .WillOnce(Invoke(DbusEnvironment::setPromise("async_read")));

    sut->registerForUpdates(listenerMock);

    ASSERT_TRUE(DbusEnvironment::waitForFuture("async_read"));
}

TEST_F(TestSensor, notifiesOnceWithValueAfterRegister)
{
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut), 42.7))
        .WillOnce(Invoke(DbusEnvironment::setPromise("async_read")));
    EXPECT_CALL(*listenerMock2, sensorUpdated(Ref(*sut), 42.7))
        .WillOnce(Invoke(DbusEnvironment::setPromise("async_read2")));

    sut->registerForUpdates(listenerMock);
    sut->registerForUpdates(listenerMock2);

    ASSERT_TRUE(DbusEnvironment::waitForFuture("async_read"));
    ASSERT_TRUE(DbusEnvironment::waitForFuture("async_read2"));
}

class TestSensorNotification : public TestSensor
{
  public:
    void SetUp() override
    {
        EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut), 0.))
            .WillOnce(Invoke(DbusEnvironment::setPromise("async_read")));

        sut->registerForUpdates(listenerMock);

        ASSERT_TRUE(DbusEnvironment::waitForFuture("async_read"));
    }

    std::shared_ptr<SensorListenerMock> listenerMock2 =
        std::make_shared<StrictMock<SensorListenerMock>>();
};

TEST_F(TestSensorNotification, notifiesListenerWithValueWhenChangeOccurs)
{
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut), 42.7))
        .WillOnce(Invoke(DbusEnvironment::setPromise("notify")));

    sensorObject.setValue(42.7);

    ASSERT_TRUE(DbusEnvironment::waitForFuture("notify"));
}

TEST_F(TestSensorNotification, notifiesListenerWithValueWhenNoChangeOccurs)
{
    Sequence seq;

    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut), 42.7)).InSequence(seq);
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut)))
        .InSequence(seq)
        .WillOnce(Invoke(DbusEnvironment::setPromise("notify")));

    sensorObject.setValue(42.7);
    sensorObject.setValue(42.7);

    ASSERT_TRUE(DbusEnvironment::waitForFuture("notify"));
}

TEST_F(TestSensorNotification, doesntNotifyExpiredListener)
{
    Sequence seq;
    EXPECT_CALL(*listenerMock2, sensorUpdated(Ref(*sut), 0.)).InSequence(seq);
    EXPECT_CALL(*listenerMock2, sensorUpdated(Ref(*sut), 42.7))
        .InSequence(seq)
        .WillOnce(Invoke(DbusEnvironment::setPromise("notify")));

    sut->registerForUpdates(listenerMock2);
    listenerMock = nullptr;

    sensorObject.setValue(42.7);

    ASSERT_TRUE(DbusEnvironment::waitForFuture("notify"));
}

TEST_F(TestSensorNotification, notifiesWithValueDuringRegister)
{
    EXPECT_CALL(*listenerMock2, sensorUpdated(Ref(*sut), 0.));

    sut->registerForUpdates(listenerMock2);
}
