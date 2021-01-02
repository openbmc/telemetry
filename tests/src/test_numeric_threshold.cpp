#include "dbus_environment.hpp"
#include "helpers.hpp"
#include "mocks/sensor_mock.hpp"
#include "mocks/trigger_action_mock.hpp"
#include "numeric_threshold.hpp"
#include "utils/conv_container.hpp"

#include <gmock/gmock.h>

using namespace testing;
using namespace std::chrono_literals;

class TestNumericThreshold : public Test
{
  public:
    std::vector<std::shared_ptr<SensorMock>> sensorMocks = {
        std::make_shared<NiceMock<SensorMock>>(),
        std::make_shared<NiceMock<SensorMock>>()};
    std::unique_ptr<TriggerActionMock> actionMockPtr =
        std::make_unique<NiceMock<TriggerActionMock>>();
    TriggerActionMock& actionMock = *actionMockPtr;
    std::shared_ptr<NumericThreshold> sut;

    void makeThreshold(NumericThresholdType thresholdType,
                       std::chrono::milliseconds dwellTime,
                       NumericActivationType activationType,
                       double thresholdValue)
    {
        std::vector<std::unique_ptr<interfaces::TriggerAction>> actions;
        actions.push_back(std::move(actionMockPtr));

        sut = std::make_shared<NumericThreshold>(
            DbusEnvironment::getIoc(),
            utils::convContainer<std::shared_ptr<interfaces::Sensor>>(
                sensorMocks),
            std::move(actions), thresholdType, dwellTime, activationType,
            thresholdValue);
    }

    void SetUp() override
    {
        makeThreshold(NumericThresholdType::upperCritical, 0ms,
                      NumericActivationType::increasing, 90.0);
    }
};

TEST_F(TestNumericThreshold, subscribesForSensorDuringInitialization)
{
    for (auto& sensor : sensorMocks)
    {
        EXPECT_CALL(*sensor,
                    registerForUpdates(Truly([sut = sut.get()](const auto& a0) {
                        return a0.lock().get() == sut;
                    })));
    }

    sut->initialize();
}

TEST_F(TestNumericThreshold, actionIsNotCommitedIfThresholdIsNotInitialize)
{
    EXPECT_CALL(actionMock, commit(_, _)).Times(0);
}

TEST_F(TestNumericThreshold, commitActionIfThresholdIsCrossed)
{
    EXPECT_CALL(actionMock, commit(uint64_t{10}, 91.0));

    sut->initialize();
    sut->sensorUpdated(*sensorMocks[0], uint64_t{5}, 50.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{10}, 91.0);
}

TEST_F(TestNumericThreshold,
       actionIsNotCommitedIfThresholdIsCrossedInOppositeDirection)
{
    EXPECT_CALL(actionMock, commit(_, _)).Times(0);

    sut->initialize();
    sut->sensorUpdated(*sensorMocks[0], uint64_t{10}, 91.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{20}, 88.0);
}

TEST_F(TestNumericThreshold, actionIsNotCommitedIfThresholdIsNotCrossed)
{
    EXPECT_CALL(actionMock, commit(_, _)).Times(0);

    sut->initialize();
    sut->sensorUpdated(*sensorMocks[0], uint64_t{5}, 50.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{10}, 89.0);
}

TEST_F(TestNumericThreshold, actionIsNotCommitedIfThreeSensorDoesNotCross)
{
    EXPECT_CALL(actionMock, commit(_, _)).Times(0);

    sut->initialize();
    sut->sensorUpdated(*sensorMocks[0], uint64_t{5}, 80.0);
    sut->sensorUpdated(*sensorMocks[1], uint64_t{5}, 98.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{10}, 88.0);
    sut->sensorUpdated(*sensorMocks[1], uint64_t{10}, 100.0);
}

class TestNumericThresholdDwellTime :
    public TestNumericThreshold,
    public WithParamInterface<std::chrono::milliseconds>
{
  public:
    void SetUp() override
    {
        makeThreshold(NumericThresholdType::upperCritical, GetParam(),
                      NumericActivationType::increasing, 90.0);
        sut->initialize();
        sut->sensorUpdated(*sensorMocks[0], uint64_t{1}, 50.0);
    }
};

INSTANTIATE_TEST_SUITE_P(_, TestNumericThresholdDwellTime,
                         Values(1ms, 100ms, 500ms));

TEST_P(TestNumericThresholdDwellTime, thresholdValueIsNotCrossedExpectNoEffect)
{
    EXPECT_CALL(actionMock, commit(_, _)).Times(0);

    DbusEnvironment::sleepFor(GetParam() + 1ms);
}

TEST_P(TestNumericThresholdDwellTime, thresholdValueIsCrossedExpectActionCommit)
{
    EXPECT_CALL(actionMock, commit(uint64_t{2}, 91.0));

    sut->sensorUpdated(*sensorMocks[0], uint64_t{2}, 91.0);
    DbusEnvironment::sleepFor(GetParam() + 1ms);
}

TEST_P(TestNumericThresholdDwellTime,
       thresholdValueIsCrossedNotInDwellTimeExpectNoEffect)
{
    EXPECT_CALL(actionMock, commit(_, _)).Times(0);

    sut->sensorUpdated(*sensorMocks[0], uint64_t{2}, 91.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{3}, 50.0);
    DbusEnvironment::sleepFor(GetParam() + 1ms);
}

TEST_P(TestNumericThresholdDwellTime,
       thesholdValueIsCrossedThresholdIsDestoryedBeforeTimeoutExpired)
{
    EXPECT_CALL(actionMock, commit(_, _)).Times(0);

    sut->sensorUpdated(*sensorMocks[0], uint64_t{2}, 91.0);
}

class TestNumericThresholdActivationType :
    public TestNumericThreshold,
    public WithParamInterface<NumericActivationType>
{
  public:
    void SetUp() override
    {
        makeThreshold(NumericThresholdType::upperCritical, 1ms, GetParam(),
                      90.0);
        sut->initialize();
        sut->sensorUpdated(*sensorMocks[0], uint64_t{1}, 90.0);
    }
};

INSTANTIATE_TEST_SUITE_P(_, TestNumericThresholdActivationType,
                         Values(NumericActivationType::increasing,
                                NumericActivationType::decreasing,
                                NumericActivationType::either));

TEST_P(TestNumericThresholdActivationType, thresholdIsNotCrossedExpectNoEffect)
{
    EXPECT_CALL(actionMock, commit(_, _)).Times(0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{10}, 1.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{20}, 2.0);
    DbusEnvironment::sleepFor(2ms);
}

class TestNumericThresholdEitherActivationType : public TestNumericThreshold
{
  public:
    void SetUp() override
    {
        makeThreshold(NumericThresholdType::upperCritical, 1ms,
                      NumericActivationType::either, 90.0);
        sut->initialize();
        sut->sensorUpdated(*sensorMocks[0], uint64_t{1}, 50.0);
    }
};

TEST_F(TestNumericThresholdEitherActivationType,
       thresholdIsCrossedExpectActionCommit)
{
    EXPECT_CALL(actionMock, commit(uint64_t{10}, 91.0));
    sut->sensorUpdated(*sensorMocks[0], uint64_t{10}, 91.0);
    DbusEnvironment::sleepFor(2ms);
}

TEST_F(TestNumericThresholdEitherActivationType,
       thresholdIsCrossedBothWaysButDwellIsTooShortForOneExpectOneActionCommit)
{
    EXPECT_CALL(actionMock, commit(uint64_t{20}, 89.0));
    sut->sensorUpdated(*sensorMocks[0], uint64_t{10}, 91.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{20}, 89.0);
    DbusEnvironment::sleepFor(2ms);
}

TEST_F(TestNumericThresholdEitherActivationType,
       thresholdIsCrossedBothWaysExpectTwoActionCommit)
{
    InSequence seq;
    EXPECT_CALL(actionMock, commit(uint64_t{10}, 91.0));
    EXPECT_CALL(actionMock, commit(uint64_t{20}, 89.0));

    sut->sensorUpdated(*sensorMocks[0], uint64_t{10}, 91.0);
    DbusEnvironment::sleepFor(2ms);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{20}, 89.0);
    DbusEnvironment::sleepFor(2ms);
}

class TestNumericThresholdDecreasingActivationType : public TestNumericThreshold
{
  public:
    void SetUp() override
    {
        makeThreshold(NumericThresholdType::upperCritical, 1ms,
                      NumericActivationType::decreasing, 90.0);
        sut->initialize();
        sut->sensorUpdated(*sensorMocks[0], uint64_t{1}, 100.0);
    }
};

TEST_F(TestNumericThresholdDecreasingActivationType,
       thresholdIsCrossedExpectActionCommit)
{
    EXPECT_CALL(actionMock, commit(uint64_t{10}, 80.0));
    sut->sensorUpdated(*sensorMocks[0], uint64_t{10}, 80.0);
    DbusEnvironment::sleepFor(2ms);
}

TEST_F(TestNumericThresholdDecreasingActivationType,
       thresholdIsCrossedButDwellNotEnoughExpectNoEffect)
{
    EXPECT_CALL(actionMock, commit(_, _)).Times(0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{10}, 80.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{20}, 100.0);
    DbusEnvironment::sleepFor(2ms);
}
