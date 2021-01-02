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

TEST_F(TestNumericThreshold, initializeThresholdExpectAllSensorsAreRegistered)
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

TEST_F(TestNumericThreshold, thresholdIsNotInitializeExpectNoActionCommit)
{
    EXPECT_CALL(actionMock, commit(_, _)).Times(0);
}

TEST_F(TestNumericThreshold,
       thresholdIsNotCrossedByTwoSensorsExpectNoActionCommit)
{
    EXPECT_CALL(actionMock, commit(_, _)).Times(0);

    sut->initialize();
    sut->sensorUpdated(*sensorMocks[0], uint64_t{5}, 80.0);
    sut->sensorUpdated(*sensorMocks[1], uint64_t{5}, 98.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{10}, 88.0);
    sut->sensorUpdated(*sensorMocks[1], uint64_t{10}, 100.0);
}

TEST_F(TestNumericThreshold, thresholdIsCrossedByTwoSensorsExpectNoActionCommit)
{
    InSequence seq;
    EXPECT_CALL(actionMock, commit(uint64_t{5}, 100.0));
    EXPECT_CALL(actionMock, commit(uint64_t{10}, 101.0));

    sut->initialize();
    sut->sensorUpdated(*sensorMocks[0], uint64_t{3}, 80.0);
    sut->sensorUpdated(*sensorMocks[1], uint64_t{5}, 89.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{5}, 100.0);
    sut->sensorUpdated(*sensorMocks[1], uint64_t{10}, 101.0);
}

class TestNumericThresholdIncreasingActivation : public TestNumericThreshold
{
  public:
    void SetUp() override
    {
        makeThreshold(NumericThresholdType::upperCritical, 0ms,
                      NumericActivationType::increasing, 90.0);
        sut->initialize();
    }
};

TEST_F(TestNumericThresholdIncreasingActivation,
       thresholdIsCrossedExpectActionCommit)
{
    EXPECT_CALL(actionMock, commit(uint64_t{10}, 91.0));

    sut->initialize();
    sut->sensorUpdated(*sensorMocks[0], uint64_t{5}, 50.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{10}, 91.0);
}

TEST_F(TestNumericThresholdIncreasingActivation,
       thresholdIsCrossedInOppositeDirectionExpectNoActionCommit)
{
    EXPECT_CALL(actionMock, commit(_, _)).Times(0);

    sut->sensorUpdated(*sensorMocks[0], uint64_t{10}, 91.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{20}, 88.0);
}

class TestNumericThresholdDecreasingActivation : public TestNumericThreshold
{
  public:
    void SetUp() override
    {
        makeThreshold(NumericThresholdType::upperCritical, 0ms,
                      NumericActivationType::decreasing, 90.0);
        sut->initialize();
    }
};

TEST_F(TestNumericThresholdDecreasingActivation,
       thresholdIsCrossedExpectActionCommit)
{
    EXPECT_CALL(actionMock, commit(uint64_t{10}, 80.0));

    sut->sensorUpdated(*sensorMocks[0], uint64_t{1}, 100.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{10}, 80.0);
}

TEST_F(TestNumericThresholdDecreasingActivation,
       thresholdIsCrossedInOppositeDirectionExpectNoActionCommit)
{
    EXPECT_CALL(actionMock, commit(_, _)).Times(0);

    sut->sensorUpdated(*sensorMocks[0], uint64_t{10}, 80.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{20}, 100.0);
}

class TestNumericThresholdEitherActivation : public TestNumericThreshold
{
  public:
    void SetUp() override
    {
        makeThreshold(NumericThresholdType::upperCritical, 0ms,
                      NumericActivationType::either, 90.0);
        sut->initialize();
    }
};

TEST_F(TestNumericThresholdEitherActivation,
       thresholdIsCrossedByIncreasingExpectActionCommit)
{
    EXPECT_CALL(actionMock, commit(uint64_t{10}, 91.0));

    sut->sensorUpdated(*sensorMocks[0], uint64_t{1}, 50.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{10}, 91.0);
}

TEST_F(TestNumericThresholdEitherActivation,
       thresholdIsCrossedByDecreasingExpectActionCommit)
{
    EXPECT_CALL(actionMock, commit(uint64_t{10}, 89.0));
    sut->sensorUpdated(*sensorMocks[0], uint64_t{1}, 99.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{10}, 89.0);
}

TEST_F(TestNumericThresholdEitherActivation,
       thresholdIsCrossedBothWaysExpectTwoActionCommit)
{
    InSequence seq;
    EXPECT_CALL(actionMock, commit(uint64_t{10}, 91.0));
    EXPECT_CALL(actionMock, commit(uint64_t{20}, 89.0));

    sut->sensorUpdated(*sensorMocks[0], uint64_t{1}, 50.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{10}, 91.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{20}, 89.0);
}

class TestNumericThresholdActivationType :
    public TestNumericThreshold,
    public WithParamInterface<NumericActivationType>
{
  public:
    void SetUp() override
    {
        makeThreshold(NumericThresholdType::upperCritical, 0ms, GetParam(),
                      90.0);
        sut->initialize();
    }
};

INSTANTIATE_TEST_SUITE_P(_, TestNumericThresholdActivationType,
                         Values(NumericActivationType::increasing,
                                NumericActivationType::decreasing,
                                NumericActivationType::either));

TEST_P(TestNumericThresholdActivationType,
       thresholdIsNotCrossedExpectNoActionCommit)
{
    EXPECT_CALL(actionMock, commit(_, _)).Times(0);

    sut->sensorUpdated(*sensorMocks[0], uint64_t{10}, 50.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{20}, 60.0);
}

TEST_P(TestNumericThresholdActivationType,
       thresholdIsNotCrossedWhenTwoSensorsAreUpdatedExpectNoActionCommit)
{
    EXPECT_CALL(actionMock, commit(_, _)).Times(0);

    sut->sensorUpdated(*sensorMocks[0], uint64_t{10}, 50.0);
    sut->sensorUpdated(*sensorMocks[1], uint64_t{10}, 100.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{20}, 60.0);
    sut->sensorUpdated(*sensorMocks[1], uint64_t{20}, 95.0);
}

TEST_P(TestNumericThresholdActivationType,
       thresholdIsNotCrossedValueIsDecreasingExpectNoActionCommit)
{
    EXPECT_CALL(actionMock, commit(_, _)).Times(0);

    sut->sensorUpdated(*sensorMocks[0], uint64_t{10}, 100.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{20}, 95.0);
}

class TestNumericThresholdIncreasingActivationWithDwellTime :
    public TestNumericThreshold
{
  public:
    void SetUp() override
    {
        makeThreshold(NumericThresholdType::upperCritical, 2ms,
                      NumericActivationType::increasing, 90.0);
        sut->initialize();
    }
};

TEST_F(TestNumericThresholdIncreasingActivationWithDwellTime,
       thresholdIsNotCrossedExpectNoActionCommit)
{
    EXPECT_CALL(actionMock, commit(_, _)).Times(0);

    sut->sensorUpdated(*sensorMocks[0], uint64_t{1}, 50.0);
    DbusEnvironment::sleepFor(4ms);
}

TEST_F(TestNumericThresholdIncreasingActivationWithDwellTime,
       thresholdIsCrossedExpectActionCommit)
{
    EXPECT_CALL(actionMock, commit(uint64_t{2}, 91.0));

    sut->sensorUpdated(*sensorMocks[0], uint64_t{1}, 50.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{2}, 91.0);
    DbusEnvironment::sleepFor(4ms);
}

TEST_F(TestNumericThresholdIncreasingActivationWithDwellTime,
       thresholdIsCrossedButShorterThanDwellTimeExpectNoActionCommit)
{
    EXPECT_CALL(actionMock, commit(_, _)).Times(0);

    sut->sensorUpdated(*sensorMocks[0], uint64_t{1}, 50.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{2}, 91.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{3}, 50.0);
    DbusEnvironment::sleepFor(4ms);
}

TEST_F(TestNumericThresholdIncreasingActivationWithDwellTime,
       thesholdIsCrossedThresholdIsDestoryedBeforeDwellTimeExpired)
{
    EXPECT_CALL(actionMock, commit(_, _)).Times(0);

    sut->sensorUpdated(*sensorMocks[0], uint64_t{1}, 50.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{2}, 91.0);
}

TEST_F(TestNumericThresholdIncreasingActivationWithDwellTime,
       tresholdIsCrossedOnTwoSensorsExpectTwoActionCommits)
{
    EXPECT_CALL(actionMock, commit(uint64_t{2}, 91.0));
    EXPECT_CALL(actionMock, commit(uint64_t{3}, 95.0));

    sut->sensorUpdated(*sensorMocks[0], uint64_t{0}, 50.0);
    sut->sensorUpdated(*sensorMocks[1], uint64_t{1}, 60.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{2}, 91.0);
    sut->sensorUpdated(*sensorMocks[1], uint64_t{3}, 95.0);
    DbusEnvironment::sleepFor(4ms);
}

TEST_F(
    TestNumericThresholdIncreasingActivationWithDwellTime,
    tresholdIsCrossedOnTwoSensorsButOneIsShorterThanDwellTimeExpectActionCommit)
{
    EXPECT_CALL(actionMock, commit(uint64_t{3}, 95.0));

    sut->sensorUpdated(*sensorMocks[0], uint64_t{0}, 50.0);
    sut->sensorUpdated(*sensorMocks[1], uint64_t{1}, 60.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{2}, 91.0);
    sut->sensorUpdated(*sensorMocks[1], uint64_t{3}, 95.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{4}, 75.0);
    DbusEnvironment::sleepFor(4ms);
}

class TestNumericThresholdDecreasingActivationWithDwellTime :
    public TestNumericThreshold
{
  public:
    void SetUp() override
    {
        makeThreshold(NumericThresholdType::upperCritical, 2ms,
                      NumericActivationType::decreasing, 90.0);
        sut->initialize();
    }
};

TEST_F(TestNumericThresholdDecreasingActivationWithDwellTime,
       thresholdIsCrossedExpectActionCommit)
{
    EXPECT_CALL(actionMock, commit(uint64_t{2}, 80.0));

    sut->sensorUpdated(*sensorMocks[0], uint64_t{1}, 100.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{2}, 80.0);
    DbusEnvironment::sleepFor(4ms);
}

TEST_F(TestNumericThresholdDecreasingActivationWithDwellTime,
       thresholdIsCrossedButShorterThanDwellTimeExpectNoActionCommit)
{
    EXPECT_CALL(actionMock, commit(_, _)).Times(0);

    sut->sensorUpdated(*sensorMocks[0], uint64_t{1}, 100.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{10}, 80.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{20}, 99.0);
    DbusEnvironment::sleepFor(4ms);
}

class TestNumericThresholdEitherActivationWithDwellTime :
    public TestNumericThreshold
{
  public:
    void SetUp() override
    {
        makeThreshold(NumericThresholdType::upperCritical, 2ms,
                      NumericActivationType::either, 90.0);
        sut->initialize();
    }
};

TEST_F(TestNumericThresholdEitherActivationWithDwellTime,
       thresholdIsCrossedBothWasyExpectTwoActionCommit)
{
    InSequence seq;
    EXPECT_CALL(actionMock, commit(uint64_t{10}, 91.0));
    EXPECT_CALL(actionMock, commit(uint64_t{20}, 89.0));

    sut->sensorUpdated(*sensorMocks[0], uint64_t{1}, 50.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{10}, 91.0);
    DbusEnvironment::sleepFor(4ms);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{20}, 89.0);
    DbusEnvironment::sleepFor(4ms);
}

TEST_F(
    TestNumericThresholdEitherActivationWithDwellTime,
    thresholdIsCrossedBothWaysButFirstCrossIsShorterThanDwellTimeExpectOneActionCommit)
{
    EXPECT_CALL(actionMock, commit(uint64_t{20}, 89.0));

    sut->sensorUpdated(*sensorMocks[0], uint64_t{1}, 50.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{10}, 91.0);
    sut->sensorUpdated(*sensorMocks[0], uint64_t{20}, 89.0);
    DbusEnvironment::sleepFor(4ms);
}
