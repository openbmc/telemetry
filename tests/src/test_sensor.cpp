#include "dbus_environment.hpp"
#include "helpers.hpp"
#include "mocks/sensor_listener_mock.hpp"
#include "sensor.hpp"
#include "sensor_cache.hpp"
#include "stubs/dbus_sensor_object.hpp"
#include "utils/clock.hpp"

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
        sensorObject->setValue(42.7);
    }

    void TearDown() override
    {
        DbusEnvironment::synchronizeIoc();
    }

    void
        registerForUpdates(std::shared_ptr<interfaces::SensorListener> listener)
    {
        sut->registerForUpdates(listener);
        DbusEnvironment::synchronizeIoc();
    }

    void unregisterFromUpdates(
        std::shared_ptr<interfaces::SensorListener> listener)
    {
        sut->unregisterFromUpdates(listener);
        DbusEnvironment::synchronizeIoc();
    }

    static std::unique_ptr<stubs::DbusSensorObject> makeSensorObject()
    {
        return std::make_unique<stubs::DbusSensorObject>(
            DbusEnvironment::getIoc(), DbusEnvironment::getBus(),
            DbusEnvironment::getObjServer());
    }

    std::unique_ptr<stubs::DbusSensorObject> sensorObject = makeSensorObject();

    SensorCache sensorCache;
    Milliseconds timestamp = Clock().steadyTimestamp();
    std::shared_ptr<Sensor> sut = sensorCache.makeSensor<Sensor>(
        DbusEnvironment::serviceName(), sensorObject->path(), "metadata",
        DbusEnvironment::getIoc(), DbusEnvironment::getBus());
    std::shared_ptr<SensorListenerMock> listenerMock =
        std::make_shared<StrictMock<SensorListenerMock>>();
    std::shared_ptr<SensorListenerMock> listenerMock2 =
        std::make_shared<StrictMock<SensorListenerMock>>();
    MockFunction<void()> checkPoint;
};

TEST_F(TestSensor, createsCorretlyViaSensorCache)
{
    ASSERT_THAT(sut->id(),
                Eq(Sensor::Id("Sensor", DbusEnvironment::serviceName(),
                              sensorObject->path())));
}

TEST_F(TestSensor, notifiesWithValueAfterRegister)
{
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut), Ge(timestamp), 42.7))
        .WillOnce(InvokeWithoutArgs(DbusEnvironment::setPromise("async_read")));

    registerForUpdates(listenerMock);

    ASSERT_TRUE(DbusEnvironment::waitForFuture("async_read"));
}

TEST_F(TestSensor, notifiesOnceWithValueAfterRegister)
{
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut), Ge(timestamp), 42.7))
        .WillOnce(InvokeWithoutArgs(DbusEnvironment::setPromise("async_read")));
    EXPECT_CALL(*listenerMock2, sensorUpdated(Ref(*sut), Ge(timestamp), 42.7))
        .WillOnce(
            InvokeWithoutArgs(DbusEnvironment::setPromise("async_read2")));

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
                InvokeWithoutArgs(DbusEnvironment::setPromise("async_read")));

        registerForUpdates(listenerMock);

        ASSERT_TRUE(DbusEnvironment::waitForFuture("async_read"));
    }
};

TEST_F(TestSensorNotification, notifiesListenerWithValueWhenChangeOccurs)
{
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut), Ge(timestamp), 42.7))
        .WillOnce(InvokeWithoutArgs(DbusEnvironment::setPromise("notify")));

    sensorObject->setValue(42.7);

    ASSERT_TRUE(DbusEnvironment::waitForFuture("notify"));
}

TEST_F(TestSensorNotification, notifiesListenerWithValueWhenNoChangeOccurs)
{
    InSequence seq;

    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut), Ge(timestamp), 42.7));
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut), Ge(timestamp)))
        .WillOnce(InvokeWithoutArgs(DbusEnvironment::setPromise("notify")));

    sensorObject->setValue(42.7);
    sensorObject->setValue(42.7);

    ASSERT_TRUE(DbusEnvironment::waitForFuture("notify"));
}

TEST_F(TestSensorNotification, doesntNotifyExpiredListener)
{
    InSequence seq;
    EXPECT_CALL(*listenerMock2, sensorUpdated(Ref(*sut), Ge(timestamp), 0.));
    EXPECT_CALL(*listenerMock2, sensorUpdated(Ref(*sut), Ge(timestamp), 42.7))
        .WillOnce(InvokeWithoutArgs(DbusEnvironment::setPromise("notify")));

    registerForUpdates(listenerMock2);
    listenerMock = nullptr;

    sensorObject->setValue(42.7);

    ASSERT_TRUE(DbusEnvironment::waitForFuture("notify"));
}

TEST_F(TestSensorNotification, notifiesWithValueDuringRegister)
{
    EXPECT_CALL(*listenerMock2, sensorUpdated(Ref(*sut), Ge(timestamp), 0.));

    registerForUpdates(listenerMock2);
}

TEST_F(TestSensorNotification, notNotifiesWithValueWhenUnregistered)
{
    EXPECT_CALL(*listenerMock2, sensorUpdated(Ref(*sut), Ge(timestamp), 0.));
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut), Ge(timestamp), 42.7))
        .Times(0);
    EXPECT_CALL(*listenerMock2, sensorUpdated(Ref(*sut), Ge(timestamp), 42.7))
        .WillOnce(InvokeWithoutArgs(DbusEnvironment::setPromise("notify")));

    registerForUpdates(listenerMock2);
    unregisterFromUpdates(listenerMock);

    sensorObject->setValue(42.7);

    ASSERT_TRUE(DbusEnvironment::waitForFuture("notify"));
}

TEST_F(TestSensorNotification,
       dbusSensorIsAddedToSystemAfterSensorIsCreatedThenValueIsUpdated)
{
    InSequence seq;

    EXPECT_CALL(*listenerMock,
                sensorUpdated(Ref(*sut), Ge(timestamp), DoubleEq(42.7)))
        .WillOnce(
            InvokeWithoutArgs(DbusEnvironment::setPromise("notify-change")));
    EXPECT_CALL(checkPoint, Call());
    EXPECT_CALL(*listenerMock,
                sensorUpdated(Ref(*sut), Ge(timestamp), DoubleEq(0.)))
        .WillOnce(
            InvokeWithoutArgs(DbusEnvironment::setPromise("notify-create")));

    sensorObject->setValue(42.7);
    DbusEnvironment::waitForFuture("notify-change");

    checkPoint.Call();

    sensorObject = nullptr;
    sensorObject = makeSensorObject();
    DbusEnvironment::waitForFuture("notify-create");
}
