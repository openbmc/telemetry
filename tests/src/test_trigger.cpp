#include "dbus_environment.hpp"
#include "helpers.hpp"
#include "mocks/json_storage_mock.hpp"
#include "mocks/trigger_manager_mock.hpp"
#include "params/trigger_params.hpp"
#include "trigger.hpp"
#include "trigger_manager.hpp"
#include "utils/conversion_trigger.hpp"
#include "utils/transform.hpp"
#include "utils/tstring.hpp"

#include <boost/range/combine.hpp>

using namespace testing;
using namespace std::literals::string_literals;

static constexpr size_t expectedTriggerVersion = 0;

class TestTrigger : public Test
{
  public:
    TriggerParams triggerParams;
    TriggerParams triggerDiscreteParams =
        TriggerParams()
            .id("Trigger2")
            .name("Trigger 2")
            .thresholdParams(std::vector<discrete::LabeledThresholdParam>{
                discrete::LabeledThresholdParam{
                    "userId", discrete::Severity::warning,
                    Milliseconds(10).count(), "15.2"},
                discrete::LabeledThresholdParam{
                    "userId_2", discrete::Severity::critical,
                    Milliseconds(5).count(), "32.7"},
            });

    std::unique_ptr<TriggerManagerMock> triggerManagerMockPtr =
        std::make_unique<NiceMock<TriggerManagerMock>>();
    testing::NiceMock<StorageMock> storageMock;
    std::unique_ptr<Trigger> sut;

    void SetUp() override
    {
        sut = makeTrigger(triggerParams);
    }

    static std::vector<LabeledSensorInfo>
        convertToLabeledSensor(const SensorsInfo& sensorsInfo)
    {
        return utils::transform(sensorsInfo, [](const auto& sensorInfo) {
            const auto& [sensorPath, sensorMetadata] = sensorInfo;
            return LabeledSensorInfo("service1", sensorPath, sensorMetadata);
        });
    }

    std::unique_ptr<Trigger> makeTrigger(const TriggerParams& params)
    {
        return std::make_unique<Trigger>(
            DbusEnvironment::getIoc(), DbusEnvironment::getObjServer(),
            params.id(), params.name(), params.triggerActions(),
            params.reportNames(), params.sensors(), params.thresholdParams(),
            std::vector<std::shared_ptr<interfaces::Threshold>>{},
            *triggerManagerMockPtr, storageMock);
    }

    static interfaces::JsonStorage::FilePath to_file_path(std::string name)
    {
        return interfaces::JsonStorage::FilePath(
            std::to_string(std::hash<std::string>{}(name)));
    }

    template <class T>
    static T getProperty(const std::string& path, const std::string& property)
    {
        return DbusEnvironment::getProperty<T>(path, Trigger::triggerIfaceName,
                                               property);
    }

    template <class T>
    static boost::system::error_code setProperty(const std::string& path,
                                                 const std::string& property,
                                                 const T& newValue)
    {
        return DbusEnvironment::setProperty<T>(path, Trigger::triggerIfaceName,
                                               property, newValue);
    }

    boost::system::error_code deleteTrigger(const std::string& path)
    {
        std::promise<boost::system::error_code> methodPromise;
        DbusEnvironment::getBus()->async_method_call(
            [&methodPromise](boost::system::error_code ec) {
                methodPromise.set_value(ec);
            },
            DbusEnvironment::serviceName(), path, Trigger::deleteIfaceName,
            "Delete");
        return DbusEnvironment::waitForFuture(methodPromise.get_future());
    }
};

TEST_F(TestTrigger, checkIfPropertiesAreSet)
{
    EXPECT_THAT(getProperty<std::string>(sut->getPath(), "Name"),
                Eq(triggerParams.name()));
    EXPECT_THAT(getProperty<bool>(sut->getPath(), "Persistent"), Eq(true));
    EXPECT_THAT(
        getProperty<std::vector<std::string>>(sut->getPath(), "TriggerActions"),
        Eq(triggerParams.triggerActions()));
    EXPECT_THAT((getProperty<SensorsInfo>(sut->getPath(), "Sensors")),
                Eq(utils::fromLabeledSensorsInfo(triggerParams.sensors())));
    EXPECT_THAT(
        getProperty<std::vector<std::string>>(sut->getPath(), "ReportNames"),
        Eq(triggerParams.reportNames()));
    EXPECT_THAT(
        getProperty<TriggerThresholdParams>(sut->getPath(), "Thresholds"),
        Eq(std::visit(utils::FromLabeledThresholdParamConversion(),
                      triggerParams.thresholdParams())));
}

TEST_F(TestTrigger, setPropertyNameToCorrectValue)
{
    std::string name = "custom name 1234 %^#5";
    EXPECT_THAT(setProperty(sut->getPath(), "Name", name),
                Eq(boost::system::errc::success));
    EXPECT_THAT(getProperty<std::string>(sut->getPath(), "Name"), Eq(name));
}

TEST_F(TestTrigger, checkIfNumericCoversionsAreGood)
{
    const auto& labeledParamsBase =
        std::get<std::vector<numeric::LabeledThresholdParam>>(
            triggerParams.thresholdParams());
    const auto paramsToCheck =
        std::visit(utils::FromLabeledThresholdParamConversion(),
                   triggerParams.thresholdParams());
    const auto labeledParamsToCheck =
        std::get<std::vector<numeric::LabeledThresholdParam>>(std::visit(
            utils::ToLabeledThresholdParamConversion(), paramsToCheck));

    for (const auto& [tocheck, base] :
         boost::combine(labeledParamsToCheck, labeledParamsBase))
    {
        EXPECT_THAT(tocheck.at_label<utils::tstring::Type>(),
                    Eq(base.at_label<utils::tstring::Type>()));
        EXPECT_THAT(tocheck.at_label<utils::tstring::Direction>(),
                    Eq(base.at_label<utils::tstring::Direction>()));
        EXPECT_THAT(tocheck.at_label<utils::tstring::DwellTime>(),
                    Eq(base.at_label<utils::tstring::DwellTime>()));
        EXPECT_THAT(tocheck.at_label<utils::tstring::ThresholdValue>(),
                    Eq(base.at_label<utils::tstring::ThresholdValue>()));
    }
}

TEST_F(TestTrigger, checkIfDiscreteCoversionsAreGood)
{
    const auto& labeledParamsBase =
        std::get<std::vector<discrete::LabeledThresholdParam>>(
            triggerDiscreteParams.thresholdParams());
    const auto paramsToCheck =
        std::visit(utils::FromLabeledThresholdParamConversion(),
                   triggerDiscreteParams.thresholdParams());
    const auto labeledParamsToCheck =
        std::get<std::vector<discrete::LabeledThresholdParam>>(std::visit(
            utils::ToLabeledThresholdParamConversion(), paramsToCheck));

    for (const auto& [tocheck, base] :
         boost::combine(labeledParamsToCheck, labeledParamsBase))
    {
        EXPECT_THAT(tocheck.at_label<utils::tstring::UserId>(),
                    Eq(base.at_label<utils::tstring::UserId>()));
        EXPECT_THAT(tocheck.at_label<utils::tstring::Severity>(),
                    Eq(base.at_label<utils::tstring::Severity>()));
        EXPECT_THAT(tocheck.at_label<utils::tstring::DwellTime>(),
                    Eq(base.at_label<utils::tstring::DwellTime>()));
        EXPECT_THAT(tocheck.at_label<utils::tstring::ThresholdValue>(),
                    Eq(base.at_label<utils::tstring::ThresholdValue>()));
    }
}

TEST_F(TestTrigger, deleteTrigger)
{
    EXPECT_CALL(storageMock, remove(to_file_path(sut->getId())));
    EXPECT_CALL(*triggerManagerMockPtr, removeTrigger(sut.get()));
    auto ec = deleteTrigger(sut->getPath());
    EXPECT_THAT(ec, Eq(boost::system::errc::success));
}

TEST_F(TestTrigger, deletingNonExistingTriggerReturnInvalidRequestDescriptor)
{
    auto ec = deleteTrigger(Trigger::triggerDir + "NonExisting"s);
    EXPECT_THAT(ec.value(), Eq(EBADR));
}

TEST_F(TestTrigger, settingPersistencyToFalseRemovesTriggerFromStorage)
{
    EXPECT_CALL(storageMock, remove(to_file_path(sut->getId())));

    bool persistent = false;
    EXPECT_THAT(setProperty(sut->getPath(), "Persistent", persistent),
                Eq(boost::system::errc::success));
    EXPECT_THAT(getProperty<bool>(sut->getPath(), "Persistent"),
                Eq(persistent));
}

class TestTriggerErrors : public TestTrigger
{
  public:
    void SetUp() override
    {}

    nlohmann::json storedConfiguration;
};

TEST_F(TestTriggerErrors, exceptionDuringTriggerStoreDisablesPersistency)
{
    EXPECT_CALL(storageMock, store(_, _))
        .WillOnce(Throw(std::runtime_error("Generic error!")));

    sut = makeTrigger(triggerParams);

    EXPECT_THAT(getProperty<bool>(sut->getPath(), "Persistent"), Eq(false));
}

TEST_F(TestTriggerErrors, creatingTriggerThrowsExceptionWhenIdIsInvalid)
{
    EXPECT_CALL(storageMock, store(_, _)).Times(0);

    EXPECT_THROW(makeTrigger(triggerParams.id("inv?lidId")),
                 sdbusplus::exception::SdBusError);
}

class TestTriggerStore : public TestTrigger
{
  public:
    nlohmann::json storedConfiguration;
    nlohmann::json storedDiscreteConfiguration;
    std::unique_ptr<Trigger> sutDiscrete;

    void SetUp() override
    {
        ON_CALL(storageMock, store(_, _))
            .WillByDefault(SaveArg<1>(&storedConfiguration));
        sut = makeTrigger(triggerParams);

        ON_CALL(storageMock, store(_, _))
            .WillByDefault(SaveArg<1>(&storedDiscreteConfiguration));
        sutDiscrete = makeTrigger(triggerDiscreteParams);
    }
};

TEST_F(TestTriggerStore, settingPersistencyToTrueStoresTriggerVersion)
{
    ASSERT_THAT(storedConfiguration.at("Version"), Eq(expectedTriggerVersion));
}

TEST_F(TestTriggerStore, settingPersistencyToTrueStoresTriggerId)
{
    ASSERT_THAT(storedConfiguration.at("Id"), Eq(triggerParams.id()));
}

TEST_F(TestTriggerStore, settingPersistencyToTrueStoresTriggerName)
{
    ASSERT_THAT(storedConfiguration.at("Name"), Eq(triggerParams.name()));
}

TEST_F(TestTriggerStore, settingPersistencyToTrueStoresTriggerTriggerActions)
{
    ASSERT_THAT(storedConfiguration.at("TriggerActions"),
                Eq(triggerParams.triggerActions()));
}

TEST_F(TestTriggerStore, settingPersistencyToTrueStoresTriggerReportNames)
{
    ASSERT_THAT(storedConfiguration.at("ReportNames"),
                Eq(triggerParams.reportNames()));
}

TEST_F(TestTriggerStore, settingPersistencyToTrueStoresTriggerSensors)
{
    nlohmann::json expectedItem;
    expectedItem["service"] = "service1";
    expectedItem["sensorPath"] =
        "/xyz/openbmc_project/sensors/temperature/BMC_Temp";
    expectedItem["sensorMetadata"] = "metadata1";

    ASSERT_THAT(storedConfiguration.at("Sensors"), ElementsAre(expectedItem));
}

TEST_F(TestTriggerStore, settingPersistencyToTrueStoresTriggerThresholdParams)
{
    nlohmann::json expectedItem0;
    expectedItem0["type"] = 0;
    expectedItem0["dwellTime"] = 10;
    expectedItem0["direction"] = 1;
    expectedItem0["thresholdValue"] = 0.5;

    nlohmann::json expectedItem1;
    expectedItem1["type"] = 3;
    expectedItem1["dwellTime"] = 10;
    expectedItem1["direction"] = 2;
    expectedItem1["thresholdValue"] = 90.2;

    ASSERT_THAT(storedConfiguration.at("ThresholdParamsDiscriminator"), Eq(0));
    ASSERT_THAT(storedConfiguration.at("ThresholdParams"),
                ElementsAre(expectedItem0, expectedItem1));
}

TEST_F(TestTriggerStore,
       settingPersistencyToTrueStoresDiscreteTriggerThresholdParams)
{
    nlohmann::json expectedItem0;
    expectedItem0["userId"] = "userId";
    expectedItem0["severity"] = discrete::Severity::warning;
    expectedItem0["dwellTime"] = 10;
    expectedItem0["thresholdValue"] = "15.2";

    nlohmann::json expectedItem1;
    expectedItem1["userId"] = "userId_2";
    expectedItem1["severity"] = discrete::Severity::critical;
    expectedItem1["dwellTime"] = 5;
    expectedItem1["thresholdValue"] = "32.7";

    ASSERT_THAT(storedDiscreteConfiguration.at("ThresholdParamsDiscriminator"),
                Eq(1));
    ASSERT_THAT(storedDiscreteConfiguration.at("ThresholdParams"),
                ElementsAre(expectedItem0, expectedItem1));
}
