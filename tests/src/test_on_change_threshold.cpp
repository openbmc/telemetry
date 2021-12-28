#include "dbus_environment.hpp"
#include "helpers.hpp"
#include "mocks/sensor_mock.hpp"
#include "mocks/trigger_action_mock.hpp"
#include "on_change_threshold.hpp"
#include "utils/conv_container.hpp"

#include <gmock/gmock.h>

using namespace testing;
using namespace std::chrono_literals;

class TestOnChangeThreshold : public Test
{
  public:
    std::vector<std::shared_ptr<SensorMock>> sensorMocks = {
        std::make_shared<NiceMock<SensorMock>>(),
        std::make_shared<NiceMock<SensorMock>>()};
    std::vector<std::string> sensorNames = {"Sensor1", "Sensor2"};
    std::unique_ptr<TriggerActionMock> actionMockPtr =
        std::make_unique<StrictMock<TriggerActionMock>>();
    TriggerActionMock& actionMock = *actionMockPtr;
    std::shared_ptr<OnChangeThreshold> sut;

    void SetUp() override
    {
        std::vector<std::unique_ptr<interfaces::TriggerAction>> actions;
        actions.push_back(std::move(actionMockPtr));

        sut = std::make_shared<OnChangeThreshold>(
            utils::convContainer<std::shared_ptr<interfaces::Sensor>>(
                sensorMocks),
            sensorNames, std::move(actions));
    }
};

TEST_F(TestOnChangeThreshold, initializeThresholdExpectAllSensorsAreRegistered)
{
    for (auto& sensor : sensorMocks)
    {
        EXPECT_CALL(*sensor,
                    registerForUpdates(Truly([sut = sut.get()](const auto& x) {
                        return x.lock().get() == sut;
                    })));
    }

    sut->initialize();
}

TEST_F(TestOnChangeThreshold, thresholdIsNotInitializeExpectNoActionCommit)
{
    EXPECT_CALL(actionMock, commit(_, _, _)).Times(0);
}

struct OnChangeParams
{
    using UpdateParams = std::tuple<size_t, Milliseconds, double>;
    using ExpectedParams = std::tuple<size_t, Milliseconds, double>;

    OnChangeParams& Updates(std::vector<UpdateParams> val)
    {
        updates = std::move(val);
        return *this;
    }

    OnChangeParams& Expected(std::vector<ExpectedParams> val)
    {
        expected = std::move(val);
        return *this;
    }

    friend void PrintTo(const OnChangeParams& o, std::ostream* os)
    {
        *os << "{ Updates: ";
        for (const auto& [index, timestamp, value] : o.updates)
        {
            *os << "{ SensorIndex: " << index
                << ", Timestamp: " << timestamp.count() << ", Value: " << value
                << " }, ";
        }
        *os << "Expected: ";
        for (const auto& [index, timestamp, value] : o.expected)
        {
            *os << "{ SensorIndex: " << index
                << ", Timestamp: " << timestamp.count() << ", Value: " << value
                << " }, ";
        }
        *os << " }";
    }

    std::vector<UpdateParams> updates;
    std::vector<ExpectedParams> expected;
};

class TestOnChangeThresholdUpdates :
    public TestOnChangeThreshold,
    public WithParamInterface<OnChangeParams>
{};

INSTANTIATE_TEST_SUITE_P(
    _, TestOnChangeThresholdUpdates,
    Values(
        OnChangeParams().Updates({{0, 1ms, 80.0}}).Expected({{0, 1ms, 80.0}}),
        OnChangeParams()
            .Updates({{0, 1ms, 80.0}, {1, 2ms, 81.0}})
            .Expected({{0, 1ms, 80.0}, {1, 2ms, 81.0}}),
        OnChangeParams()
            .Updates({{0, 1ms, 80.0}, {0, 2ms, 90.0}})
            .Expected({{0, 1ms, 80.0}, {0, 2ms, 90.0}}),
        OnChangeParams()
            .Updates({{0, 1ms, 80.0}, {1, 2ms, 90.0}, {0, 3ms, 90.0}})
            .Expected({{0, 1ms, 80.0}, {1, 2ms, 90.0}, {0, 3ms, 90.0}}),
        OnChangeParams()
            .Updates({{0, 1ms, 80.0},
                      {1, 2ms, 80.0},
                      {1, 3ms, 90.0},
                      {0, 4ms, 90.0}})
            .Expected({{0, 1ms, 80.0},
                       {1, 2ms, 80.0},
                       {1, 3ms, 90.0},
                       {0, 4ms, 90.0}})));

TEST_P(TestOnChangeThresholdUpdates, senorsIsUpdatedMultipleTimes)
{
    InSequence seq;
    for (const auto& [index, timestamp, value] : GetParam().expected)
    {
        EXPECT_CALL(actionMock, commit(sensorNames[index], timestamp, value));
    }

    sut->initialize();
    for (const auto& [index, timestamp, value] : GetParam().updates)
    {
        sut->sensorUpdated(*sensorMocks[index], timestamp, value);
    }
}
