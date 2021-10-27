#include "dbus_environment.hpp"
#include "helpers.hpp"
#include "mocks/json_storage_mock.hpp"
#include "mocks/trigger_factory_mock.hpp"
#include "mocks/trigger_mock.hpp"
#include "params/trigger_params.hpp"
#include "trigger.hpp"
#include "trigger_manager.hpp"
#include "utils/conversion_trigger.hpp"
#include "utils/transform.hpp"

using namespace testing;

class TestTriggerManager : public Test
{
  public:
    std::pair<boost::system::error_code, std::string>
        addTrigger(const TriggerParams& params)
    {
        const auto sensorInfos =
            utils::fromLabeledSensorsInfo(params.sensors());

        std::promise<std::pair<boost::system::error_code, std::string>>
            addTriggerPromise;
        DbusEnvironment::getBus()->async_method_call(
            [&addTriggerPromise](boost::system::error_code ec,
                                 const std::string& path) {
                addTriggerPromise.set_value({ec, path});
            },
            DbusEnvironment::serviceName(), TriggerManager::triggerManagerPath,
            TriggerManager::triggerManagerIfaceName, "AddTrigger", params.id(),
            params.name(), params.triggerActions(), sensorInfos,
            params.reportNames(),
            std::visit(utils::FromLabeledThresholdParamConversion(),
                       params.thresholdParams()));
        return DbusEnvironment::waitForFuture(addTriggerPromise.get_future());
    }

    std::unique_ptr<TriggerManager> makeTriggerManager()
    {
        return std::make_unique<TriggerManager>(
            std::move(triggerFactoryMockPtr), std::move(storageMockPtr),
            DbusEnvironment::getObjServer());
    }

    void SetUp() override
    {
        sut = makeTriggerManager();
    }

    std::unique_ptr<StorageMock> storageMockPtr =
        std::make_unique<NiceMock<StorageMock>>();
    StorageMock& storageMock = *storageMockPtr;
    std::unique_ptr<TriggerFactoryMock> triggerFactoryMockPtr =
        std::make_unique<NiceMock<TriggerFactoryMock>>();
    TriggerFactoryMock& triggerFactoryMock = *triggerFactoryMockPtr;
    std::unique_ptr<TriggerMock> triggerMockPtr =
        std::make_unique<NiceMock<TriggerMock>>(TriggerParams().id());
    TriggerMock& triggerMock = *triggerMockPtr;
    std::unique_ptr<TriggerManager> sut;
    MockFunction<void(std::string)> checkPoint;
};

TEST_F(TestTriggerManager, addTrigger)
{
    triggerFactoryMock.expectMake(TriggerParams(), Ref(*sut), Ref(storageMock))
        .WillOnce(Return(ByMove(std::move(triggerMockPtr))));

    auto [ec, path] = addTrigger(TriggerParams());
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    EXPECT_THAT(path, Eq(triggerMock.getPath()));
}

TEST_F(TestTriggerManager, addTriggerWithDiscreteThresholds)
{
    TriggerParams triggerParamsDiscrete;
    auto thresholds = std::vector<discrete::LabeledThresholdParam>{
        {"discrete_threshold1", discrete::Severity::ok, 10, "11.0"},
        {"discrete_threshold2", discrete::Severity::warning, 10, "12.0"},
        {"discrete_threshold3", discrete::Severity::critical, 10, "13.0"}};

    triggerParamsDiscrete.thresholdParams(thresholds);

    auto [ec, path] = addTrigger(triggerParamsDiscrete);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    EXPECT_THAT(path, Eq(triggerMock.getPath()));
}

TEST_F(TestTriggerManager, addDiscreteTriggerWithoutThresholds)
{
    TriggerParams triggerParamsDiscrete;
    auto thresholds = std::vector<discrete::LabeledThresholdParam>();

    triggerParamsDiscrete.thresholdParams(thresholds);

    auto [ec, path] = addTrigger(triggerParamsDiscrete);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    EXPECT_THAT(path, Eq(triggerMock.getPath()));
}

TEST_F(TestTriggerManager, DISABLED_failToAddTriggerTwice)
{
    triggerFactoryMock.expectMake(TriggerParams(), Ref(*sut), Ref(storageMock))
        .WillOnce(Return(ByMove(std::move(triggerMockPtr))));

    addTrigger(TriggerParams());

    auto [ec, path] = addTrigger(TriggerParams());
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::file_exists));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestTriggerManager, DISABLED_failToAddTriggerWithInvalidId)
{
    triggerFactoryMock.expectMake(std::nullopt, Ref(*sut), Ref(storageMock))
        .Times(0);

    auto [ec, path] = addTrigger(TriggerParams().id("not valid?"));
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::invalid_argument));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestTriggerManager, addTriggerWithoutIdAndName)
{
    triggerFactoryMock
        .expectMake(TriggerParams()
                        .id(TriggerManager::triggerNameDefault)
                        .name(TriggerManager::triggerNameDefault),
                    Ref(*sut), Ref(storageMock))
        .WillOnce(Return(ByMove(std::move(triggerMockPtr))));

    auto [ec, path] = addTrigger(TriggerParams().id("").name(""));
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    EXPECT_THAT(path, Not(Eq("")));
}

TEST_F(TestTriggerManager, addTriggerWithPrefixId)
{
    triggerFactoryMock
        .expectMake(TriggerParams().id("TelemetryService/Trigger1"), Ref(*sut),
                    Ref(storageMock))
        .WillOnce(Return(ByMove(std::move(triggerMockPtr))));

    auto [ec, path] = addTrigger(TriggerParams().id("TelemetryService/"));
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    EXPECT_THAT(path, Not(Eq("")));
}

TEST_F(TestTriggerManager, addTriggerWithoutIdTwice)
{
    addTrigger(TriggerParams().id(""));

    auto [ec, path] = addTrigger(TriggerParams().id(""));
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    EXPECT_THAT(path, Not(Eq("")));
}

TEST_F(TestTriggerManager, addTriggerWithoutIdAndWithLongNameTwice)
{
    addTrigger(TriggerParams().id("").name(
        std::string(2 * TriggerManager::maxTriggerIdLength, 'z')));

    auto [ec, path] = addTrigger(TriggerParams().id("").name(
        std::string(2 * TriggerManager::maxTriggerIdLength, 'z')));
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    EXPECT_THAT(path, Not(Eq("")));
}

TEST_F(TestTriggerManager, addTriggerWithMaxLengthId)
{
    auto triggerId = std::string(TriggerManager::maxTriggerIdLength, 'z');
    auto triggerParams = TriggerParams().id(triggerId);

    triggerFactoryMock.expectMake(triggerParams, Ref(*sut), Ref(storageMock))
        .WillOnce(Return(ByMove(std::move(triggerMockPtr))));

    auto [ec, path] = addTrigger(triggerParams);

    EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    EXPECT_THAT(path, Eq(triggerMock.getPath()));
}

TEST_F(TestTriggerManager, DISABLED_failToAddTriggerWithTooLongId)
{
    auto triggerId = std::string(TriggerManager::maxTriggerIdLength + 1, 'z');
    auto triggerParams = TriggerParams().id(triggerId);

    triggerFactoryMock.expectMake(std::nullopt, Ref(*sut), Ref(storageMock))
        .Times(0);

    auto [ec, path] = addTrigger(triggerParams);

    EXPECT_THAT(ec.value(), Eq(boost::system::errc::invalid_argument));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestTriggerManager, DISABLED_failToAddTriggerWhenMaxTriggerIsReached)
{
    auto triggerParams = TriggerParams();

    triggerFactoryMock.expectMake(std::nullopt, Ref(*sut), Ref(storageMock))
        .Times(TriggerManager::maxTriggers);

    for (size_t i = 0; i < TriggerManager::maxTriggers; i++)
    {
        triggerParams.id(TriggerParams().id() + std::to_string(i));

        auto [ec, path] = addTrigger(triggerParams);
        EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    }

    triggerParams.id(TriggerParams().id() +
                     std::to_string(TriggerManager::maxTriggers));
    auto [ec, path] = addTrigger(triggerParams);
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::too_many_files_open));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestTriggerManager, removeTrigger)
{
    {
        InSequence seq;
        triggerFactoryMock
            .expectMake(TriggerParams(), Ref(*sut), Ref(storageMock))
            .WillOnce(Return(ByMove(std::move(triggerMockPtr))));
        EXPECT_CALL(triggerMock, Die());
        EXPECT_CALL(checkPoint, Call("end"));
    }

    addTrigger(TriggerParams());
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
        triggerFactoryMock
            .expectMake(TriggerParams(), Ref(*sut), Ref(storageMock))
            .WillOnce(Return(ByMove(std::move(triggerMockPtr))));
        EXPECT_CALL(triggerMock, Die());
        EXPECT_CALL(checkPoint, Call("end"));
    }

    addTrigger(TriggerParams());
    sut->removeTrigger(&triggerMock);
    sut->removeTrigger(&triggerMock);
    checkPoint.Call("end");
}

class TestTriggerManagerStorage : public TestTriggerManager
{
  public:
    using FilePath = interfaces::JsonStorage::FilePath;
    using DirectoryPath = interfaces::JsonStorage::DirectoryPath;

    void SetUp() override
    {
        ON_CALL(storageMock, list())
            .WillByDefault(Return(std::vector<FilePath>{
                {FilePath("trigger1")}, {FilePath("trigger2")}}));

        ON_CALL(storageMock, load(FilePath("trigger1")))
            .WillByDefault(InvokeWithoutArgs([this] { return data1; }));

        data2["Id"] = "Trigger2";
        data2["Name"] = "Trigger 2";
        ON_CALL(storageMock, load(FilePath("trigger2")))
            .WillByDefault(InvokeWithoutArgs([this] { return data2; }));
    }

    nlohmann::json data1 = nlohmann::json{
        {"Version", Trigger::triggerVersion},
        {"Id", TriggerParams().id()},
        {"Name", TriggerParams().name()},
        {"ThresholdParamsDiscriminator",
         TriggerParams().thresholdParams().index()},
        {"TriggerActions", TriggerParams().triggerActions()},
        {"ThresholdParams", utils::labeledThresholdParamsToJson(
                                TriggerParams().thresholdParams())},
        {"ReportNames", TriggerParams().reportNames()},
        {"Sensors", TriggerParams().sensors()}};

    nlohmann::json data2 = data1;
};

TEST_F(TestTriggerManagerStorage, triggerManagerCtorAddTriggerFromStorage)
{
    triggerFactoryMock.expectMake(TriggerParams(), _, Ref(storageMock));
    triggerFactoryMock.expectMake(
        TriggerParams().id("Trigger2").name("Trigger 2"), _, Ref(storageMock));
    EXPECT_CALL(storageMock, remove(_)).Times(0);

    sut = makeTriggerManager();
}

TEST_F(TestTriggerManagerStorage,
       triggerManagerCtorRemoveDiscreteTriggerFromStorage)
{
    LabeledTriggerThresholdParams thresholdParams =
        std::vector<discrete::LabeledThresholdParam>{
            {"userId1", discrete::Severity::warning, 15, "10.0"},
            {"userId2", discrete::Severity::critical, 5, "20.0"}};

    data1["ThresholdParamsDiscriminator"] = thresholdParams.index();

    data1["ThresholdParams"] =
        utils::labeledThresholdParamsToJson(thresholdParams);

    EXPECT_CALL(storageMock, remove(FilePath("trigger1"))).Times(0);

    sut = makeTriggerManager();
}

TEST_F(TestTriggerManagerStorage,
       triggerManagerCtorRemoveDiscreteTriggerFromStorage2)
{
    data1["IsDiscrete"] = true;

    EXPECT_CALL(storageMock, remove(FilePath("trigger1"))).Times(0);

    sut = makeTriggerManager();
}

TEST_F(TestTriggerManagerStorage,
       triggerManagerCtorAddProperRemoveInvalidTriggerFromStorage)
{
    data1["Version"] = Trigger::triggerVersion - 1;

    triggerFactoryMock.expectMake(
        TriggerParams().id("Trigger2").name("Trigger 2"), _, Ref(storageMock));
    EXPECT_CALL(storageMock, remove(FilePath("trigger1")));

    sut = makeTriggerManager();
}
