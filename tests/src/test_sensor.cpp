#include "dbus_environment.hpp"
#include "mocks/sensor_listener_mock.hpp"
#include "sensor.hpp"
#include "sensor_cache.hpp"
#include "stubs/sensor_service.hpp"
#include "utils/sensor_id_eq.hpp"

#include <thread>

#include <gmock/gmock.h>

using namespace testing;

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
        DbusEnvironment::service(), sensorService.path(),
        DbusEnvironment::getBus());
    std::shared_ptr<SensorListenerMock> listenerMock =
        std::make_shared<NiceMock<SensorListenerMock>>();
};

TEST_F(TestSensor, shouldBeCreatedCorretlyViaSensorCache)
{
    ASSERT_THAT(sut->id(),
                sensorIdEq(Sensor::Id("Sensor", DbusEnvironment::service(),
                                      sensorService.path())));
}

TEST_F(TestSensor, shouldNotifyListenerWithValueWhenChangeOccurs)
{
    InSequence _;
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut)));
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut), 42.7));

    sut->registerForUpdates(listenerMock);
    sut->async_read();

    using namespace std::chrono_literals;
    DbusEnvironment::sleep(10ms);
}

TEST_F(TestSensor, shouldNotifyListenerWithoutValueWhenNoChangeOccurs)
{
    InSequence _;
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut)));
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut), 42.7));
    EXPECT_CALL(*listenerMock, sensorUpdated(Ref(*sut)));

    sut->registerForUpdates(listenerMock);
    sut->async_read();
    sut->async_read();

    using namespace std::chrono_literals;
    DbusEnvironment::sleep(10ms);
}

TEST_F(TestSensor, shouldNotNotifyExpiredListener)
{
    {
        auto listener = std::make_shared<StrictMock<SensorListenerMock>>();
        EXPECT_CALL(*listener, sensorUpdated(Ref(*sut)));
        sut->registerForUpdates(listener);
    }

    sut->async_read();

    using namespace std::chrono_literals;
    DbusEnvironment::sleep(10ms);
}
