#include "helpers.hpp"
#include "mocks/sensor_mock.hpp"
#include "sensor_cache.hpp"

#include <initializer_list>

#include <gmock/gmock.h>

using namespace testing;

class TestSensorCache : public Test
{
  public:
    SensorCache sut;
};

struct IdParam
{
    IdParam() = default;
    IdParam(std::string_view service, std::string_view path) :
        service(service), path(path)
    {}

    std::string service;
    std::string path;
};

class TestSensorCacheP :
    public TestSensorCache,
    public WithParamInterface<std::vector<IdParam>>
{
  public:
    void SetUp() override
    {
        auto vec = GetParam();
        ASSERT_THAT(vec, SizeIs(param.size()));
        std::copy(vec.begin(), vec.end(), param.begin());
    }

    template <size_t index>
    const IdParam& id() const
    {
        static_assert(index < std::tuple_size_v<decltype(param)>);
        return param[index];
    }

  private:
    std::array<IdParam, 2> param;
};

INSTANTIATE_TEST_SUITE_P(
    UniqueIds, TestSensorCacheP,
    Values(std::vector<IdParam>({IdParam("service1", "path1"),
                                 IdParam("service1", "path2")}),
           std::vector<IdParam>({IdParam("service1", "path1"),
                                 IdParam("service2", "path1")}),
           std::vector<IdParam>({IdParam("service1", "path1"),
                                 IdParam("service2", "path2")})));

TEST_P(TestSensorCacheP, shouldReturnDifferentSensorWhenIdsAreDifferent)
{
    auto sensor1 = sut.makeSensor<NiceMock<SensorMock>>(id<0>().service,
                                                        id<0>().path);
    auto sensor2 = sut.makeSensor<NiceMock<SensorMock>>(id<1>().service,
                                                        id<1>().path);

    ASSERT_THAT(sensor1.get(), Not(Eq(sensor2.get())));
    ASSERT_THAT(sensor1->mockId, Not(Eq(sensor2->mockId)));
}

TEST_F(TestSensorCache, shouldReturnSameSensorWhenSensorWithSameIdStillExists)
{
    auto sensor1 = sut.makeSensor<NiceMock<SensorMock>>("sensor-service",
                                                        "sensor-path");
    auto sensor2 = sut.makeSensor<NiceMock<SensorMock>>("sensor-service",
                                                        "sensor-path");

    ASSERT_THAT(sensor1.get(), Eq(sensor2.get()));
    ASSERT_THAT(sensor1->mockId, Eq(sensor2->mockId));
}

TEST_F(TestSensorCache, shouldReturnDifferentSensorWhenPreviousSensorExpired)
{
    auto mockId1 = sut.makeSensor<NiceMock<SensorMock>>("sensor-service",
                                                        "sensor-path")
                       ->mockId;
    auto mockId2 = sut.makeSensor<NiceMock<SensorMock>>("sensor-service",
                                                        "sensor-path")
                       ->mockId;

    ASSERT_THAT(mockId2, Not(Eq(mockId1)));
}

TEST_F(TestSensorCache, shouldCreateSensorWithCorrespondingId)
{
    auto id = sut.makeSensor<NiceMock<SensorMock>>("sensor-service",
                                                   "sensor-path")
                  ->id();

    auto expected = SensorMock::makeId("sensor-service", "sensor-path");

    ASSERT_THAT(id, Eq(expected));
}
