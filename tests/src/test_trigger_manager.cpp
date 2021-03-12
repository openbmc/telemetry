#include "dbus_environment.hpp"
#include "helpers.hpp"
#include "mocks/trigger_factory_mock.hpp"
#include "mocks/trigger_mock.hpp"
#include "params/trigger_params.hpp"
#include "trigger_manager.hpp"

using namespace testing;

class TestTriggerManager : public Test
{
  public:
    std::pair<boost::system::error_code, std::string>
        addTrigger(const TriggerParams& params)
    {
        std::promise<std::pair<boost::system::error_code, std::string>>
            addTriggerPromise;
        DbusEnvironment::getBus()->async_method_call(
            [&addTriggerPromise](boost::system::error_code ec,
                                 const std::string& path) {
                addTriggerPromise.set_value({ec, path});
            },
            DbusEnvironment::serviceName(), TriggerManager::triggerManagerPath,
            TriggerManager::triggerManagerIfaceName, "AddTrigger",
            params.name(), params.isDiscrete(), params.logToJournal(),
            params.logToRedfish(), params.updateReport(), params.sensors(),
            params.reportNames(), params.thresholdParams());
        return DbusEnvironment::waitForFuture(addTriggerPromise.get_future());
    }

    TriggerParams triggerParams;
    std::unique_ptr<TriggerFactoryMock> triggerFactoryMockPtr =
        std::make_unique<NiceMock<TriggerFactoryMock>>();
    TriggerFactoryMock& triggerFactoryMock = *triggerFactoryMockPtr;
    std::unique_ptr<TriggerMock> triggerMockPtr =
        std::make_unique<NiceMock<TriggerMock>>(triggerParams.name());
    TriggerMock& triggerMock = *triggerMockPtr;
    std::unique_ptr<TriggerManager> sut = std::make_unique<TriggerManager>(
        std::move(triggerFactoryMockPtr),
        std::move(DbusEnvironment::getObjServer()));
    MockFunction<void(std::string)> checkPoint;
};

TEST_F(TestTriggerManager, addTrigger)
{
    triggerFactoryMock.expectMake(triggerParams, Ref(*sut))
        .WillOnce(Return(ByMove(std::move(triggerMockPtr))));

    auto [ec, path] = addTrigger(triggerParams);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    EXPECT_THAT(path, Eq(triggerMock.getPath()));
}

TEST_F(TestTriggerManager, addTriggerWithDiscreteThresholds)
{
    TriggerParams triggerParamsDiscrete;
    auto thresholds = std::vector<discrete::ThresholdParam>{
        {"discrete_threshold1",
         discrete::severityToString(discrete::Severity::ok), 10, false, 11.0},
        {"discrete_threshold2",
         discrete::severityToString(discrete::Severity::warning), 10, false,
         12.0},
        {"discrete_threshold3",
         discrete::severityToString(discrete::Severity::critical), 10, false,
         13.0},
        {"discrete_threshold4",
         discrete::severityToString(discrete::Severity::ok), 10, true, 0.0}};

    triggerParamsDiscrete.thresholdParams(thresholds).isDiscrete(true);

    auto [ec, path] = addTrigger(triggerParamsDiscrete);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    EXPECT_THAT(path, Eq(triggerMock.getPath()));
}

TEST_F(TestTriggerManager,
       DISABLED_failToAddTriggerWithDuplicateDiscreteThresholdName)
{
    TriggerParams triggerParamsDiscrete;
    auto thresholds = std::vector<discrete::ThresholdParam>{
        {"discrete_threshold1",
         discrete::severityToString(discrete::Severity::ok), 10, false, 11.0},
        {"discrete_threshold2",
         discrete::severityToString(discrete::Severity::warning), 10, false,
         12.0},
        {"discrete_threshold1",
         discrete::severityToString(discrete::Severity::critical), 10, false,
         13.0},
        {"discrete_threshold4",
         discrete::severityToString(discrete::Severity::ok), 10, true, 0.0}};
    triggerParamsDiscrete.thresholdParams(thresholds).isDiscrete(true);

    auto [ec, path] = addTrigger(triggerParamsDiscrete);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::file_exists));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestTriggerManager, DISABLED_failToAddTriggerTwice)
{
    triggerFactoryMock.expectMake(triggerParams, Ref(*sut))
        .WillOnce(Return(ByMove(std::move(triggerMockPtr))));

    addTrigger(triggerParams);

    auto [ec, path] = addTrigger(triggerParams);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::file_exists));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestTriggerManager, DISABLED_failToAddTriggerWhenMaxTriggerIsReached)
{
    triggerFactoryMock.expectMake(std::nullopt, Ref(*sut))
        .Times(TriggerManager::maxTriggers);

    for (size_t i = 0; i < TriggerManager::maxTriggers; i++)
    {
        triggerParams.name(triggerParams.name() + std::to_string(i));

        auto [ec, path] = addTrigger(triggerParams);
        EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    }

    triggerParams.name(triggerParams.name() +
                       std::to_string(TriggerManager::maxTriggers));
    auto [ec, path] = addTrigger(triggerParams);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::too_many_files_open));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestTriggerManager, removeTrigger)
{
    {
        InSequence seq;
        triggerFactoryMock.expectMake(triggerParams, Ref(*sut))
            .WillOnce(Return(ByMove(std::move(triggerMockPtr))));
        EXPECT_CALL(triggerMock, Die());
        EXPECT_CALL(checkPoint, Call("end"));
    }

    addTrigger(triggerParams);
    sut->removeTrigger(&triggerMock);
    checkPoint.Call("end");
}

TEST_F(TestTriggerManager, removingTriggerThatIsNotInContainerHasNoEffect)
{
    {
        InSequence seq;
        EXPECT_CALL(checkPoint, Call("end"));
        EXPECT_CALL(triggerMock, Die());
    }

    sut->removeTrigger(&triggerMock);
    checkPoint.Call("end");
}

TEST_F(TestTriggerManager, removingSameTriggerTwiceHasNoSideEffect)
{
    {
        InSequence seq;
        triggerFactoryMock.expectMake(triggerParams, Ref(*sut))
            .WillOnce(Return(ByMove(std::move(triggerMockPtr))));
        EXPECT_CALL(triggerMock, Die());
        EXPECT_CALL(checkPoint, Call("end"));
    }

    addTrigger(triggerParams);
    sut->removeTrigger(&triggerMock);
    sut->removeTrigger(&triggerMock);
    checkPoint.Call("end");
}
