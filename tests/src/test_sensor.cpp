#include "dbus_environment.hpp"
#include "mocks/sensor_listener_mock.hpp"
#include "sensor.hpp"
#include "sensor_cache.hpp"
#include "stubs/dbus_sensor_object.hpp"
#include "utils/measure_elapsed_time.hpp"
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
        sensorObject.value = 42.7;
    }

    void TearDown() override
    {
        DbusEnvironment::synchronizeIoc();
    }

    void completeAsyncRead()
    {
        std::shared_ptr<SensorListenerMock> mock =
            std::make_shared<NiceMock<SensorListenerMock>>();
        mock->delegateIgnoringArgs();

        Expectation seq = EXPECT_CALL(*mock, sensorUpdated());
        sut->registerForUpdates(mock);

        EXPECT_CALL(*mock, sensorUpdated())
            .After(seq)
            .WillOnce(Invoke(DbusEnvironment::setPromise("async_read")));
        sut->async_read();

        DbusEnvironment::waitForFuture("async_read");
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
};

TEST_F(TestSensor, createsCorretlyViaSensorCache)
{
    ASSERT_THAT(sut->id(),
                sensorIdEq(Sensor::Id("Sensor", DbusEnvironment::serviceName(),
                                      sensorObject.path())));
}

TEST_F(TestSensor, notifiesWithoutValueWhenThereIsNoValueDuringRegister)
{
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut)));

    sut->registerForUpdates(listenerMock);
}

TEST_F(TestSensor, notifiesWithValueWhenThereIsValueDuringRegister)
{
    completeAsyncRead();

    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut), 42.7));
    sut->registerForUpdates(listenerMock);
}

TEST_F(TestSensor, notifiesListenerWithValueWhenChangeOccurs)
{
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut)));
    sut->registerForUpdates(listenerMock);

    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut), 42.7));
    completeAsyncRead();
}

TEST_F(TestSensor, notifiesListenerWithoutValueWhenNoChangeOccurs)
{
    Sequence seq;
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut))).InSequence(seq);
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut), 42.7)).InSequence(seq);
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut))).InSequence(seq);

    sut->registerForUpdates(listenerMock);

    completeAsyncRead();
    completeAsyncRead();
}

TEST_F(TestSensor, doesntNotifyExpiredListener)
{
    {
        auto listener = std::make_shared<StrictMock<SensorListenerMock>>();
        EXPECT_CALL(*listener, sensorUpdated(Ref(*sut)));
        sut->registerForUpdates(listener);
    }

    completeAsyncRead();
}

std::chrono::milliseconds TestSensor::notifiesInGivenIntervalAfterSchedule(
    std::chrono::milliseconds interval)
{
    auto listener = std::make_shared<NiceMock<SensorListenerMock>>();
    listener->delegateIgnoringArgs();

    sut->registerForUpdates(listener);

    {
        InSequence seq;
        EXPECT_CALL(*listener, sensorUpdated()).Times(4);
        EXPECT_CALL(*listener, sensorUpdated())
            .WillOnce(Invoke(DbusEnvironment::setPromise("5th_async_read")));
    }

    sut->schedule(interval);

    auto elapsed = utils::measureElapsedTime<std::chrono::milliseconds>(
        [] { DbusEnvironment::waitForFuture("5th_async_read"); });

    sut->stop();

    return elapsed;
}

TEST_F(TestSensor, notifiesInGivenIntervalAfterSchedule)
{
    // When interval is too small update might be ommited (performance
    // dependency). With interval big enough all updates should happen in given
    // time frame.
    for (auto interval : {5, 10, 50, 100})
    {
        auto elapsed = notifiesInGivenIntervalAfterSchedule(
                           std::chrono::milliseconds(interval))
                           .count();
        if (elapsed >= interval * 5 && elapsed < interval * 6)
        {
            return;
        }
    }

    ASSERT_THAT(notifiesInGivenIntervalAfterSchedule(750ms).count(),
                AllOf(Ge(750 * 5), Lt(750 * 6)));
}
