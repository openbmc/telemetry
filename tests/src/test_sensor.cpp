#include "dbus_environment.hpp"
#include "mocks/sensor_listener_mock.hpp"
#include "sensor.hpp"
#include "sensor_cache.hpp"
#include "stubs/dbus_sensor_object.hpp"
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

    void TearDown() override
    {
        DbusEnvironment::synchronizeIoc();
    }

    void
        registerForUpdates(std::shared_ptr<interfaces::SensorListener> listener)
    {
        DbusEnvironment::synchronizedPost(
            [this, listener] { sut->registerForUpdates(listener); });
    }

    std::chrono::milliseconds notifiesInGivenIntervalAfterSchedule(
        std::chrono::milliseconds interval);

    stubs::DbusSensorObject sensorObject{DbusEnvironment::getIoc(),
                                         DbusEnvironment::getBus(),
                                         DbusEnvironment::getObjServer()};

    SensorCache sensorCache;
    uint64_t timestamp = std::time(0);
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
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut), Ge(timestamp), 42.7))
        .WillOnce(Invoke([] { DbusEnvironment::setPromise("async_read"); }));

    registerForUpdates(listenerMock);

    ASSERT_TRUE(DbusEnvironment::waitForFuture("async_read"));
}

TEST_F(TestSensor, notifiesOnceWithValueAfterRegister)
{
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut), Ge(timestamp), 42.7))
        .WillOnce(Invoke([] { DbusEnvironment::setPromise("async_read"); }));
    EXPECT_CALL(*listenerMock2, sensorUpdated(Ref(*sut), Ge(timestamp), 42.7))
        .WillOnce(Invoke([] { DbusEnvironment::setPromise("async_read2"); }));

    DbusEnvironment::synchronizedPost([this] {
        sut->registerForUpdates(listenerMock);
        sut->registerForUpdates(listenerMock2);
    });

    ASSERT_TRUE(DbusEnvironment::waitForFuture("async_read"));
    ASSERT_TRUE(DbusEnvironment::waitForFuture("async_read2"));
}

class TestSensorNotification : public TestSensor
{
  public:
    void SetUp() override
    {
        EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut), Ge(timestamp), 0.))
            .WillOnce(
                Invoke([] { DbusEnvironment::setPromise("async_read"); }));

        registerForUpdates(listenerMock);

        ASSERT_TRUE(DbusEnvironment::waitForFuture("async_read"));
    }

    std::shared_ptr<SensorListenerMock> listenerMock2 =
        std::make_shared<StrictMock<SensorListenerMock>>();
};

TEST_F(TestSensorNotification, notifiesListenerWithValueWhenChangeOccurs)
{
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut), Ge(timestamp), 42.7))
        .WillOnce(Invoke([] { DbusEnvironment::setPromise("notify"); }));

    sensorObject.setValue(42.7);

    ASSERT_TRUE(DbusEnvironment::waitForFuture("notify"));
}

TEST_F(TestSensorNotification, notifiesListenerWithValueWhenNoChangeOccurs)
{
    Sequence seq;

    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut), Ge(timestamp), 42.7))
        .InSequence(seq);
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut), Ge(timestamp)))
        .InSequence(seq)
        .WillOnce(Invoke([] { DbusEnvironment::setPromise("notify"); }));

    sensorObject.setValue(42.7);
    sensorObject.setValue(42.7);

    ASSERT_TRUE(DbusEnvironment::waitForFuture("notify"));
}

TEST_F(TestSensorNotification, doesntNotifyExpiredListener)
{
    Sequence seq;
    EXPECT_CALL(*listenerMock2, sensorUpdated(Ref(*sut), Ge(timestamp), 0.))
        .InSequence(seq);
    EXPECT_CALL(*listenerMock2, sensorUpdated(Ref(*sut), Ge(timestamp), 42.7))
        .InSequence(seq)
        .WillOnce(Invoke([] { DbusEnvironment::setPromise("notify"); }));

    registerForUpdates(listenerMock2);
    listenerMock = nullptr;

    sensorObject.setValue(42.7);

    ASSERT_TRUE(DbusEnvironment::waitForFuture("notify"));
}

TEST_F(TestSensorNotification, notifiesWithValueDuringRegister)
{
    EXPECT_CALL(*listenerMock2, sensorUpdated(Ref(*sut), Ge(timestamp), 0.));

    registerForUpdates(listenerMock2);
}
