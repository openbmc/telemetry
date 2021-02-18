#include "dbus_environment.hpp"
#include "helpers.hpp"
#include "mocks/json_storage_mock.hpp"
#include "mocks/trigger_manager_mock.hpp"
#include "params/trigger_params.hpp"
#include "trigger.hpp"
#include "utils/set_exception.hpp"

using namespace testing;
using namespace std::literals::string_literals;

static constexpr size_t expectedTriggerVersion = 0;

class TestTrigger : public Test
{
  public:
    TriggerParams triggerParams;

    std::unique_ptr<TriggerManagerMock> triggerManagerMockPtr =
        std::make_unique<NiceMock<TriggerManagerMock>>();
    testing::NiceMock<StorageMock> storageMock;
    std::unique_ptr<Trigger> sut;

    void SetUp() override
    {
        sut = makeTrigger(triggerParams);
    }

    std::unique_ptr<Trigger> makeTrigger(const TriggerParams& params)
    {
        return std::make_unique<Trigger>(
            DbusEnvironment::getIoc(), DbusEnvironment::getObjServer(),
            params.name(), params.isDiscrete(), params.logToJournal(),
            params.logToRedfish(), params.updateReport(), params.sensors(),
            params.reportNames(), params.thresholdParams(),
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
        auto propertyPromise = std::promise<T>();
        auto propertyFuture = propertyPromise.get_future();
        sdbusplus::asio::getProperty<T>(
            *DbusEnvironment::getBus(), DbusEnvironment::serviceName(), path,
            Trigger::triggerIfaceName, property,
            [&propertyPromise](const boost::system::error_code& ec, T t) {
                if (ec)
                {
                    utils::setException(propertyPromise, "GetProperty failed");
                    return;
                }
                propertyPromise.set_value(t);
            });
        return DbusEnvironment::waitForFuture(std::move(propertyFuture));
    }

    template <class T>
    static boost::system::error_code setProperty(const std::string& path,
                                                 const std::string& property,
                                                 const T& newValue)
    {
        auto setPromise = std::promise<boost::system::error_code>();
        auto setFuture = setPromise.get_future();

        sdbusplus::asio::setProperty(
            *DbusEnvironment::getBus(), DbusEnvironment::serviceName(), path,
            Trigger::triggerIfaceName, property, std::move(newValue),
            [setPromise =
                 std::move(setPromise)](boost::system::error_code ec) mutable {
                setPromise.set_value(ec);
            });
        return DbusEnvironment::waitForFuture(std::move(setFuture));
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
    EXPECT_THAT(getProperty<bool>(sut->getPath(), "Persistent"), Eq(true));
    EXPECT_THAT(getProperty<bool>(sut->getPath(), "Discrete"),
                Eq(triggerParams.isDiscrete()));
    EXPECT_THAT(getProperty<bool>(sut->getPath(), "LogToJournal"),
                Eq(triggerParams.logToJournal()));
    EXPECT_THAT(getProperty<bool>(sut->getPath(), "LogToRedfish"),
                Eq(triggerParams.logToRedfish()));
    EXPECT_THAT(getProperty<bool>(sut->getPath(), "UpdateReport"),
                Eq(triggerParams.updateReport()));
    EXPECT_THAT((getProperty<std::vector<
                     std::pair<sdbusplus::message::object_path, std::string>>>(
                    sut->getPath(), "Sensors")),
                Eq(triggerParams.sensors()));
    EXPECT_THAT(
        getProperty<std::vector<std::string>>(sut->getPath(), "ReportNames"),
        Eq(triggerParams.reportNames()));
    EXPECT_THAT(
        getProperty<TriggerThresholdParams>(sut->getPath(), "Thresholds"),
        Eq(triggerParams.thresholdParams()));
}

TEST_F(TestTrigger, deleteTrigger)
{
    EXPECT_CALL(storageMock, remove(to_file_path(sut->getName())));
    EXPECT_CALL(*triggerManagerMockPtr, removeTrigger(sut.get()));
    auto ec = deleteTrigger(sut->getPath());
    EXPECT_THAT(ec, Eq(boost::system::errc::success));
}

TEST_F(TestTrigger, deletingNonExistingTriggerReturnInvalidRequestDescriptor)
{
    auto ec = deleteTrigger(Trigger::triggerDir + "NonExisting"s);
    EXPECT_THAT(ec.value(), Eq(EBADR));
}

TEST_F(TestTrigger, settingPersistencyToFalseRemovesReportFromStorage)
{
    EXPECT_CALL(storageMock, remove(to_file_path(sut->getName())));

    bool persistent = false;
    EXPECT_THAT(setProperty(sut->getPath(), "Persistent", persistent).value(),
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

TEST_F(TestTriggerErrors, throwingExceptionDoesNotStoreTriggerReportNames)
{
    EXPECT_CALL(storageMock, store(_, _))
        .WillOnce(Throw(std::runtime_error("Generic error!")));

    sut = makeTrigger(triggerParams);

    EXPECT_THAT(getProperty<bool>(sut->getPath(), "Persistent"), Eq(false));
}

TEST_F(TestTriggerErrors, creatingTriggerThrowsExceptionWhenNameIsInvalid)
{
    EXPECT_CALL(storageMock, store(_, _)).Times(0);

    EXPECT_THROW(makeTrigger(triggerParams.name("inv?lidName")),
                 sdbusplus::exception::SdBusError);
}

class TestTriggerStore : public TestTrigger
{
  public:
    void SetUp() override
    {
        ON_CALL(storageMock, store(_, _))
            .WillByDefault(SaveArg<1>(&storedConfiguration));

        sut = makeTrigger(triggerParams);
    }

    nlohmann::json storedConfiguration;
};

TEST_F(TestTriggerStore, settingPersistencyToTrueStoresTriggerVersion)
{
    ASSERT_THAT(storedConfiguration.at("Version"), Eq(expectedTriggerVersion));
}

TEST_F(TestTriggerStore, settingPersistencyToTrueStoresTriggerName)
{
    ASSERT_THAT(storedConfiguration.at("Name"), Eq(triggerParams.name()));
}

TEST_F(TestTriggerStore, settingPersistencyToTrueStoresTriggerIsDiscrete)
{
    ASSERT_THAT(storedConfiguration.at("IsDiscrete"),
                Eq(triggerParams.isDiscrete()));
}

TEST_F(TestTriggerStore, settingPersistencyToTrueStoresTriggerLogToJournal)
{
    ASSERT_THAT(storedConfiguration.at("LogToJournal"),
                Eq(triggerParams.logToRedfish()));
}

TEST_F(TestTriggerStore, settingPersistencyToTrueStoresTriggerLogToRedfish)
{
    ASSERT_THAT(storedConfiguration.at("LogToRedfish"),
                Eq(triggerParams.logToRedfish()));
}

TEST_F(TestTriggerStore, settingPersistencyToTrueStoresTriggerUpdateReport)
{
    ASSERT_THAT(storedConfiguration.at("UpdateReport"),
                Eq(triggerParams.updateReport()));
}

TEST_F(TestTriggerStore, settingPersistencyToTrueStoresTriggerReportNames)
{
    ASSERT_THAT(storedConfiguration.at("ReportNames"),
                Eq(triggerParams.reportNames()));
}

TEST_F(TestTriggerStore, settingPersistencyToTrueStoresTriggerSensors)
{
    nlohmann::json expectedItem;
    expectedItem["sensorPath"] =
        "/xyz/openbmc_project/sensors/temperature/BMC_Temp";
    expectedItem["sensorMetadata"] = "";

    ASSERT_THAT(storedConfiguration.at("Sensors"), ElementsAre(expectedItem));
}

TEST_F(TestTriggerStore, settingPersistencyToTrueStoresTriggerThresholdParams)
{
    ASSERT_THAT(storedConfiguration.at("ThresholdParamsDiscriminator"), Eq(0));

    nlohmann::json expectedItem0;
    expectedItem0["type"] = 0;
    expectedItem0["dwellTime"] = 10;
    expectedItem0["direction"] = 1;
    expectedItem0["thresholdValue"] = 0.0;

    nlohmann::json expectedItem1;
    expectedItem1["type"] = 3;
    expectedItem1["dwellTime"] = 10;
    expectedItem1["direction"] = 2;
    expectedItem1["thresholdValue"] = 90.0;

    ASSERT_THAT(storedConfiguration.at("ThresholdParams"),
                ElementsAre(ElementsAre(expectedItem0, expectedItem1)));
}

class TestTriggerStoreDiscrete : public TestTrigger
{
  public:
    void SetUp() override
    {
        triggerParams.isDiscrete(true);

        triggerParams.thresholdParams(std::vector<discrete::ThresholdParam>{
            {"111", discrete::severityToString(discrete::Severity::warning),
             10.0, 20},
            {"112", discrete::severityToString(discrete::Severity::critical),
             15.0, 5}});

        ON_CALL(storageMock, store(_, _))
            .WillByDefault(SaveArg<1>(&storedConfiguration));

        sut = makeTrigger(triggerParams);
    }

    nlohmann::json storedConfiguration;
};

TEST_F(TestTriggerStoreDiscrete,
       settingPersistencyToTrueStoresDiscreteTriggerIsDiscrete)
{
    ASSERT_THAT(storedConfiguration.at("IsDiscrete"),
                Eq(triggerParams.isDiscrete()));
}

TEST_F(TestTriggerStoreDiscrete,
       settingPersistencyToTrueStoresDiscreteTriggerThresholdParams)
{
    ASSERT_THAT(storedConfiguration.at("ThresholdParamsDiscriminator"), Eq(1));

    nlohmann::json expectedItem0;
    expectedItem0["userId"] = "111";
    expectedItem0["severity"] = 1;
    expectedItem0["dwellTime"] = 10.0;
    expectedItem0["thresholdValue"] = 20;

    nlohmann::json expectedItem1;
    expectedItem1["userId"] = "112";
    expectedItem1["severity"] = 2;
    expectedItem1["dwellTime"] = 15.0;
    expectedItem1["thresholdValue"] = 5;

    ASSERT_THAT(storedConfiguration.at("ThresholdParams"),
                ElementsAre(ElementsAre(expectedItem0, expectedItem1)));
}