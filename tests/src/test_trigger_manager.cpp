#include "dbus_environment.hpp"
#include "helpers.hpp"
#include "mocks/json_storage_mock.hpp"
#include "mocks/trigger_factory_mock.hpp"
#include "mocks/trigger_mock.hpp"
#include "params/trigger_params.hpp"
#include "trigger.hpp"
#include "trigger_manager.hpp"
#include "utils/conversion.hpp"
#include "utils/transform.hpp"

using namespace testing;

class TestTriggerManager : public Test
{
  public:
    std::pair<boost::system::error_code, std::string>
        addTrigger(const TriggerParams& params)
    {
        const auto sensorInfos = fromLabeledSensorsInfo(params.sensors());

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
            params.logToRedfish(), params.updateReport(), sensorInfos,
            params.reportNames(),
            std::visit(FromLabeledThresholdParamConversion(),
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
        std::make_unique<NiceMock<TriggerMock>>(TriggerParams().name());
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

TEST_F(TestTriggerManager, DISABLED_failToAddTriggerTwice)
{
    triggerFactoryMock.expectMake(TriggerParams(), Ref(*sut), Ref(storageMock))
        .WillOnce(Return(ByMove(std::move(triggerMockPtr))));

    addTrigger(TriggerParams());

    auto [ec, path] = addTrigger(TriggerParams());
    EXPECT_THAT(ec.value(), Eq(boost::system::errc::file_exists));
    EXPECT_THAT(path, Eq(std::string()));
}

TEST_F(TestTriggerManager, DISABLED_failToAddTriggerWhenMaxTriggerIsReached)
{
    auto triggerParams = TriggerParams();

    triggerFactoryMock.expectMake(std::nullopt, Ref(*sut), Ref(storageMock))
        .Times(TriggerManager::maxTriggers);

    for (size_t i = 0; i < TriggerManager::maxTriggers; i++)
    {
        triggerParams.name(TriggerParams().name() + std::to_string(i));

        auto [ec, path] = addTrigger(triggerParams);
        EXPECT_THAT(ec.value(), Eq(boost::system::errc::success));
    }

    triggerParams.name(TriggerParams().name() +
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

        data2["Name"] = "Trigger2";
        ON_CALL(storageMock, load(FilePath("trigger2")))
            .WillByDefault(InvokeWithoutArgs([this] { return data2; }));
    }

    nlohmann::json data1 = nlohmann::json{
        {"Version", Trigger::triggerVersion},
        {"Name", TriggerParams().name()},
        {"ThresholdParamsDiscriminator",
         TriggerParams().thresholdParams().index()},
        {"IsDiscrete", TriggerParams().isDiscrete()},
        {"LogToJournal", TriggerParams().logToJournal()},
        {"LogToRedfish", TriggerParams().logToRedfish()},
        {"UpdateReport", TriggerParams().updateReport()},
        {"ThresholdParams",
         labeledThresholdParamsToJson(TriggerParams().thresholdParams())},
        {"ReportNames", TriggerParams().reportNames()},
        {"Sensors", TriggerParams().sensors()}};

    nlohmann::json data2 = data1;
};

TEST_F(TestTriggerManagerStorage, triggerManagerCtorAddTriggerFromStorage)
{
    triggerFactoryMock.expectMake(TriggerParams(), _, Ref(storageMock));
    triggerFactoryMock.expectMake(TriggerParams().name("Trigger2"), _,
                                  Ref(storageMock));
    EXPECT_CALL(storageMock, remove(_)).Times(0);

    sut = makeTriggerManager();
}

TEST_F(TestTriggerManagerStorage,
       triggerManagerCtorRemoveFileIfVersionDoesNotMatch)
{
    data1["Version"] = Trigger::triggerVersion - 1;

    EXPECT_CALL(storageMock, remove(FilePath("trigger1")));

    sut = makeTriggerManager();
}

TEST_F(TestTriggerManagerStorage,
       triggerManagerCtorRemoveDiscreteTriggerFromStorage)
{
    LabeledTriggerThresholdParams thresholdParams =
        std::vector<discrete::LabeledThresholdParam>{
            {"1", discrete::Severity::warning, 10.0, 15},
            {"2", discrete::Severity::critical, 20.0, 5}};

    data1["ThresholdParamsDiscriminator"] = thresholdParams.index();

    data1["ThresholdParams"] = labeledThresholdParamsToJson(thresholdParams);

    EXPECT_CALL(storageMock, remove(FilePath("trigger1")));

    sut = makeTriggerManager();
}

TEST_F(TestTriggerManagerStorage,
       triggerManagerCtorRemoveDiscreteTriggerFromStorage2)
{
    data1["IsDiscrete"] = true;

    EXPECT_CALL(storageMock, remove(FilePath("trigger1")));

    sut = makeTriggerManager();
}

TEST_F(TestTriggerManagerStorage,
       triggerManagerCtorAddProperRemoveInvalidTriggerFromStorage)
{
    data1["Version"] = Trigger::triggerVersion - 1;

    triggerFactoryMock.expectMake(TriggerParams().name("Trigger2"), _,
                                  Ref(storageMock));
    EXPECT_CALL(storageMock, remove(FilePath("trigger1")));

    sut = makeTriggerManager();
}