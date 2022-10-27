#include "dbus_environment.hpp"
#include "helpers.hpp"
#include "messages/collect_trigger_id.hpp"
#include "messages/trigger_presence_changed_ind.hpp"
#include "mocks/json_storage_mock.hpp"
#include "mocks/report_manager_mock.hpp"
#include "mocks/sensor_mock.hpp"
#include "mocks/threshold_mock.hpp"
#include "mocks/trigger_factory_mock.hpp"
#include "mocks/trigger_manager_mock.hpp"
#include "params/trigger_params.hpp"
#include "trigger.hpp"
#include "trigger_manager.hpp"
#include "utils/conversion_trigger.hpp"
#include "utils/dbus_path_utils.hpp"
#include "utils/messanger.hpp"
#include "utils/string_utils.hpp"
#include "utils/transform.hpp"
#include "utils/tstring.hpp"

#include <boost/range/combine.hpp>

using namespace testing;
using namespace std::literals::string_literals;
using sdbusplus::message::object_path;

static constexpr size_t expectedTriggerVersion = 2;

class TestTrigger : public Test
{
  public:
    TriggerParams triggerParams;
    TriggerParams triggerDiscreteParams =
        TriggerParams()
            .id("DiscreteTrigger")
            .name("My Discrete Trigger")
            .thresholdParams(std::vector<discrete::LabeledThresholdParam>{
                discrete::LabeledThresholdParam{
                    "userId", discrete::Severity::warning,
                    Milliseconds(10).count(), "15.2"},
                discrete::LabeledThresholdParam{
                    "userId_2", discrete::Severity::critical,
                    Milliseconds(5).count(), "32.7"},
            });

    std::unique_ptr<ReportManagerMock> reportManagerMockPtr =
        std::make_unique<NiceMock<ReportManagerMock>>();
    std::unique_ptr<TriggerManagerMock> triggerManagerMockPtr =
        std::make_unique<NiceMock<TriggerManagerMock>>();
    std::unique_ptr<TriggerFactoryMock> triggerFactoryMockPtr =
        std::make_unique<NiceMock<TriggerFactoryMock>>();
    testing::NiceMock<StorageMock> storageMock;
    NiceMock<MockFunction<void(const messages::TriggerPresenceChangedInd)>>
        triggerPresenceChanged;
    std::vector<std::shared_ptr<interfaces::Threshold>> thresholdMocks;
    utils::Messanger messanger;
    std::unique_ptr<Trigger> sut;

    TestTrigger() : messanger(DbusEnvironment::getIoc())
    {
        messanger.on_receive<messages::TriggerPresenceChangedInd>(
            [this](const auto& msg) { triggerPresenceChanged.Call(msg); });
    }

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
        thresholdMocks =
            ThresholdMock::makeThresholds(params.thresholdParams());

        auto id = std::make_unique<const std::string>(params.id());

        return std::make_unique<Trigger>(
            DbusEnvironment::getIoc(), DbusEnvironment::getObjServer(),
            std::move(id), params.name(), params.triggerActions(),
            std::make_shared<std::vector<std::string>>(
                params.reportIds().begin(), params.reportIds().end()),
            std::vector<std::shared_ptr<interfaces::Threshold>>(thresholdMocks),
            *triggerManagerMockPtr, storageMock, *triggerFactoryMockPtr,
            SensorMock::makeSensorMocks(params.sensors()));
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

    template <class T>
    struct ChangePropertyParams
    {
        Matcher<T> valueBefore = _;
        T newValue;
        Matcher<boost::system::error_code> ec =
            Eq(boost::system::errc::success);
        Matcher<T> valueAfter = Eq(newValue);
    };

    template <class T>
    static void changeProperty(const std::string& path,
                               const std::string& property,
                               ChangePropertyParams<T> p)
    {
        ASSERT_THAT(getProperty<T>(path, property), p.valueBefore);
        ASSERT_THAT(setProperty<T>(path, property, p.newValue), p.ec);
        EXPECT_THAT(getProperty<T>(path, property), p.valueAfter);
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
        Eq(utils::transform(
            triggerParams.triggerActions(),
            [](const auto& action) { return actionToString(action); })));
    EXPECT_THAT((getProperty<SensorsInfo>(sut->getPath(), "Sensors")),
                Eq(utils::fromLabeledSensorsInfo(triggerParams.sensors())));
    EXPECT_THAT(
        getProperty<std::vector<object_path>>(sut->getPath(), "Reports"),
        Eq(triggerParams.reports()));
    EXPECT_THAT(
        getProperty<bool>(sut->getPath(), "Discrete"),
        Eq(isTriggerThresholdDiscrete(triggerParams.thresholdParams())));
    EXPECT_THAT(
        getProperty<TriggerThresholdParams>(sut->getPath(), "Thresholds"),
        Eq(std::visit(utils::FromLabeledThresholdParamConversion(),
                      triggerParams.thresholdParams())));
}

TEST_F(TestTrigger, checkBasicGetters)
{
    EXPECT_THAT(sut->getId(), Eq(triggerParams.id()));
    EXPECT_THAT(sut->getPath(),
                Eq(utils::constants::triggerDirPath.str + triggerParams.id()));
}

TEST_F(TestTrigger, setPropertyNameToCorrectValue)
{
    std::string name = "custom name 1234 %^#5";
    EXPECT_THAT(setProperty(sut->getPath(), "Name", name),
                Eq(boost::system::errc::success));
    EXPECT_THAT(getProperty<std::string>(sut->getPath(), "Name"), Eq(name));
}

TEST_F(TestTrigger, setPropertyReportNames)
{
    std::vector<object_path> newNames = {
        utils::constants::reportDirPath / "abc",
        utils::constants::reportDirPath / "one",
        utils::constants::reportDirPath / "prefix" / "two"};
    EXPECT_THAT(setProperty(sut->getPath(), "Reports", newNames),
                Eq(boost::system::errc::success));
    EXPECT_THAT(
        getProperty<std::vector<object_path>>(sut->getPath(), "Reports"),
        Eq(newNames));
}

TEST_F(TestTrigger, sendsUpdateWhenReportNamesChanges)
{
    std::vector<object_path> newPropertyVal = {
        utils::constants::reportDirPath / "abc",
        utils::constants::reportDirPath / "one",
        utils::constants::reportDirPath / "two"};

    EXPECT_CALL(triggerPresenceChanged,
                Call(FieldsAre(messages::Presence::Exist, triggerParams.id(),
                               UnorderedElementsAre("abc", "one", "two"))));

    EXPECT_THAT(setProperty(sut->getPath(), "Reports", newPropertyVal),
                Eq(boost::system::errc::success));
}

TEST_F(TestTrigger, sendsUpdateWhenReportNamesChangesToSameValue)
{
    const std::vector<object_path> newPropertyVal = triggerParams.reports();

    EXPECT_CALL(
        triggerPresenceChanged,
        Call(FieldsAre(messages::Presence::Exist, triggerParams.id(),
                       UnorderedElementsAreArray(triggerParams.reportIds()))));

    EXPECT_THAT(setProperty(sut->getPath(), "Reports", newPropertyVal),
                Eq(boost::system::errc::success));
}

TEST_F(TestTrigger,
       DISABLED_settingPropertyReportNamesThrowsExceptionWhenDuplicateReportIds)
{
    std::vector<object_path> newPropertyVal{
        utils::constants::reportDirPath / "report1",
        utils::constants::reportDirPath / "report2",
        utils::constants::reportDirPath / "report1"};

    EXPECT_CALL(triggerPresenceChanged, Call(_)).Times(0);

    EXPECT_THAT(setProperty(sut->getPath(), "Reports", newPropertyVal),
                Eq(boost::system::errc::invalid_argument));
}

TEST_F(
    TestTrigger,
    DISABLED_settingPropertyReportNamesThrowsExceptionWhenReportWithTooManyPrefixes)
{
    std::vector<object_path> newPropertyVal{
        object_path("/xyz/openbmc_project/Telemetry/Reports/P1/P2/MyReport")};

    EXPECT_CALL(triggerPresenceChanged, Call(_)).Times(0);

    EXPECT_THAT(setProperty(sut->getPath(), "Reports", newPropertyVal),
                Eq(boost::system::errc::invalid_argument));
}

TEST_F(
    TestTrigger,
    DISABLED_settingPropertyReportNamesThrowsExceptionWhenReportWithTooLongPrefix)
{
    std::vector<object_path> newPropertyVal{
        object_path("/xyz/openbmc_project/Telemetry/Reports/" +
                    utils::string_utils::getTooLongPrefix() + "/MyReport")};

    EXPECT_CALL(triggerPresenceChanged, Call(_)).Times(0);

    EXPECT_THAT(setProperty(sut->getPath(), "Reports", newPropertyVal),
                Eq(boost::system::errc::invalid_argument));
}

TEST_F(
    TestTrigger,
    DISABLED_settingPropertyReportNamesThrowsExceptionWhenReportWithTooLongId)
{
    std::vector<object_path> newPropertyVal{
        object_path("/xyz/openbmc_project/Telemetry/Reports/Prefix/" +
                    utils::string_utils::getTooLongId())};

    EXPECT_CALL(triggerPresenceChanged, Call(_)).Times(0);

    EXPECT_THAT(setProperty(sut->getPath(), "Reports", newPropertyVal),
                Eq(boost::system::errc::invalid_argument));
}

TEST_F(TestTrigger,
       DISABLED_settingPropertyReportNamesThrowsExceptionWhenReportWithBadPath)
{
    std::vector<object_path> newPropertyVal{
        object_path("/xyz/openbmc_project/Telemetry/NotReports/MyReport")};

    EXPECT_CALL(triggerPresenceChanged, Call(_)).Times(0);

    EXPECT_THAT(setProperty(sut->getPath(), "Reports", newPropertyVal),
                Eq(boost::system::errc::invalid_argument));
}

TEST_F(TestTrigger, setPropertySensors)
{
    EXPECT_CALL(*triggerFactoryMockPtr, updateSensors(_, _));
    for (const auto& threshold : thresholdMocks)
    {
        auto thresholdMockPtr =
            std::dynamic_pointer_cast<NiceMock<ThresholdMock>>(threshold);
        EXPECT_CALL(*thresholdMockPtr, updateSensors(_));
    }
    SensorsInfo newSensors(
        {std::make_pair(object_path("/abc/def"), "metadata")});
    EXPECT_THAT(setProperty(sut->getPath(), "Sensors", newSensors),
                Eq(boost::system::errc::success));
}

TEST_F(TestTrigger, setPropertyThresholds)
{
    EXPECT_CALL(*triggerFactoryMockPtr, updateThresholds(_, _, _, _, _, _));
    TriggerThresholdParams newThresholds =
        std::vector<discrete::ThresholdParam>({std::make_tuple(
            "discrete threshold", utils::enumToString(discrete::Severity::ok),
            10, "12.3")});
    EXPECT_THAT(setProperty(sut->getPath(), "Thresholds", newThresholds),
                Eq(boost::system::errc::success));
}

TEST_F(TestTrigger, setThresholdParamsWithTooLongDiscreteName)
{
    const TriggerThresholdParams currentValue =
        std::visit(utils::FromLabeledThresholdParamConversion(),
                   triggerParams.thresholdParams());

    TriggerThresholdParams newThresholds =
        std::vector<discrete::ThresholdParam>({std::make_tuple(
            utils::string_utils::getTooLongName(),
            utils::enumToString(discrete::Severity::ok), 10, "12.3")});

    changeProperty<TriggerThresholdParams>(
        sut->getPath(), "Thresholds",
        {.valueBefore = Eq(currentValue),
         .newValue = newThresholds,
         .ec = Eq(boost::system::errc::invalid_argument),
         .valueAfter = Eq(currentValue)});
}

TEST_F(TestTrigger, setNameTooLong)
{
    std::string currentValue = TriggerParams().name();

    changeProperty<std::string>(
        sut->getPath(), "Name",
        {.valueBefore = Eq(currentValue),
         .newValue = utils::string_utils::getTooLongName(),
         .ec = Eq(boost::system::errc::invalid_argument),
         .valueAfter = Eq(currentValue)});
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

TEST_F(TestTrigger, sendUpdateWhenTriggerIsDeleted)
{
    EXPECT_CALL(triggerPresenceChanged,
                Call(FieldsAre(messages::Presence::Removed, triggerParams.id(),
                               UnorderedElementsAre())));

    auto ec = deleteTrigger(sut->getPath());
    EXPECT_THAT(ec, Eq(boost::system::errc::success));
}

TEST_F(TestTrigger, deletingNonExistingTriggerReturnInvalidRequestDescriptor)
{
    auto ec =
        deleteTrigger(utils::constants::triggerDirPath.str + "NonExisting"s);
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

class TestTriggerInitialization : public TestTrigger
{
  public:
    void SetUp() override
    {}

    nlohmann::json storedConfiguration;
};

TEST_F(TestTriggerInitialization,
       exceptionDuringTriggerStoreDisablesPersistency)
{
    EXPECT_CALL(storageMock, store(_, _))
        .WillOnce(Throw(std::runtime_error("Generic error!")));

    sut = makeTrigger(triggerParams);

    EXPECT_THAT(getProperty<bool>(sut->getPath(), "Persistent"), Eq(false));
}

TEST_F(TestTriggerInitialization, creatingTriggerThrowsExceptionWhenIdIsInvalid)
{
    EXPECT_CALL(storageMock, store(_, _)).Times(0);

    EXPECT_THROW(makeTrigger(triggerParams.id("inv?lidId")),
                 sdbusplus::exception::SdBusError);
}

TEST_F(TestTriggerInitialization, creatingTriggerUpdatesTriggersIdsInReports)
{
    EXPECT_CALL(
        triggerPresenceChanged,
        Call(FieldsAre(messages::Presence::Exist, triggerParams.id(),
                       UnorderedElementsAreArray(triggerParams.reportIds()))));

    sut = makeTrigger(triggerParams);
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
                Eq(utils::transform(triggerParams.triggerActions(),
                                    [](const auto& action) {
                                        return actionToString(action);
                                    })));
}

TEST_F(TestTriggerStore, settingPersistencyToTrueStoresTriggerReportIds)
{
    ASSERT_THAT(storedConfiguration.at("ReportIds"),
                Eq(triggerParams.reportIds()));
}

TEST_F(TestTriggerStore, settingPersistencyToTrueStoresTriggerSensors)
{
    nlohmann::json expectedItem;
    expectedItem["service"] = "service1";
    expectedItem["path"] = "/xyz/openbmc_project/sensors/temperature/BMC_Temp";
    expectedItem["metadata"] = "metadata1";

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
