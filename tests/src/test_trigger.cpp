#include "dbus_environment.hpp"
#include "helpers.hpp"
#include "mocks/json_storage_mock.hpp"
#include "mocks/trigger_manager_mock.hpp"
#include "params/trigger_params.hpp"
#include "trigger.hpp"
#include "utils/set_exception.hpp"

using namespace testing;
using namespace std::literals::string_literals;

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
        sut = std::make_unique<Trigger>(
            DbusEnvironment::getIoc(), DbusEnvironment::getObjServer(),
            triggerParams.name(), triggerParams.isDiscrete(),
            triggerParams.logToJournal(), triggerParams.logToRedfish(),
            triggerParams.updateReport(), triggerParams.sensors(),
            triggerParams.reportNames(), triggerParams.thresholdParams(),
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
        std::promise<T> propertyPromise;
        sdbusplus::asio::getProperty<T>(
            *DbusEnvironment::getBus(), DbusEnvironment::serviceName(), path,
            Trigger::triggerIfaceName, property,
            [&propertyPromise](boost::system::error_code) {
                utils::setException(propertyPromise, "GetProperty failed");
            },
            [&propertyPromise](T t) { propertyPromise.set_value(t); });
        return DbusEnvironment::waitForFuture(propertyPromise.get_future());
    }

    template <class T>
    static boost::system::error_code setProperty(const std::string& path,
                                                 const std::string& property,
                                                 const T& newValue)
    {
        std::promise<boost::system::error_code> setPromise;
        sdbusplus::asio::setProperty(
            *DbusEnvironment::getBus(), DbusEnvironment::serviceName(), path,
            Trigger::triggerIfaceName, property, std::move(newValue),
            [&setPromise](boost::system::error_code ec) {
                setPromise.set_value(ec);
            },
            [&setPromise]() {
                setPromise.set_value(boost::system::error_code{});
            });
        return DbusEnvironment::waitForFuture(setPromise.get_future());
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

TEST_F(TestTrigger, throwingExceptionDoesNotStoreTriggerReportNames)
{
    EXPECT_CALL(storageMock, store(_, _))
        .WillOnce(Throw(std::runtime_error("Generic error!")));

    sut = nullptr;

    sut = std::make_unique<Trigger>(
        DbusEnvironment::getIoc(), DbusEnvironment::getObjServer(),
        triggerParams.name(), triggerParams.isDiscrete(),
        triggerParams.logToJournal(), triggerParams.logToRedfish(),
        triggerParams.updateReport(), triggerParams.sensors(),
        triggerParams.reportNames(), triggerParams.thresholdParams(),
        std::vector<std::shared_ptr<interfaces::Threshold>>{},
        *triggerManagerMockPtr, storageMock);

    EXPECT_THAT(getProperty<bool>(sut->getPath(), "Persistent"), Eq(false));
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

TEST_F(TestTrigger, settingPersistencyToTrueStoresTriggerName)
{
    nlohmann::json storedConfiguration;

    EXPECT_CALL(storageMock, store(_, _))
        .WillOnce(SaveArg<1>(&storedConfiguration));

    sut = nullptr;

    sut = std::make_unique<Trigger>(
        DbusEnvironment::getIoc(), DbusEnvironment::getObjServer(),
        triggerParams.name(), triggerParams.isDiscrete(),
        triggerParams.logToJournal(), triggerParams.logToRedfish(),
        triggerParams.updateReport(), triggerParams.sensors(),
        triggerParams.reportNames(), triggerParams.thresholdParams(),
        std::vector<std::shared_ptr<interfaces::Threshold>>{},
        *triggerManagerMockPtr, storageMock);

    ASSERT_THAT(storedConfiguration.at("Name"), Eq(triggerParams.name()));
}

TEST_F(TestTrigger, settingPersistencyToTrueStoresTriggerIsDiscrete)
{
    nlohmann::json storedConfiguration;

    EXPECT_CALL(storageMock, store(_, _))
        .WillOnce(SaveArg<1>(&storedConfiguration));

    sut = nullptr;

    sut = std::make_unique<Trigger>(
        DbusEnvironment::getIoc(), DbusEnvironment::getObjServer(),
        triggerParams.name(), triggerParams.isDiscrete(),
        triggerParams.logToJournal(), triggerParams.logToRedfish(),
        triggerParams.updateReport(), triggerParams.sensors(),
        triggerParams.reportNames(), triggerParams.thresholdParams(),
        std::vector<std::shared_ptr<interfaces::Threshold>>{},
        *triggerManagerMockPtr, storageMock);

    ASSERT_THAT(storedConfiguration.at("IsDiscrete"),
                Eq(triggerParams.isDiscrete()));
}

TEST_F(TestTrigger, settingPersistencyToTrueStoresTriggerLogToJournal)
{
    nlohmann::json storedConfiguration;

    EXPECT_CALL(storageMock, store(_, _))
        .WillOnce(SaveArg<1>(&storedConfiguration));

    sut = nullptr;

    sut = std::make_unique<Trigger>(
        DbusEnvironment::getIoc(), DbusEnvironment::getObjServer(),
        triggerParams.name(), triggerParams.isDiscrete(),
        triggerParams.logToJournal(), triggerParams.logToRedfish(),
        triggerParams.updateReport(), triggerParams.sensors(),
        triggerParams.reportNames(), triggerParams.thresholdParams(),
        std::vector<std::shared_ptr<interfaces::Threshold>>{},
        *triggerManagerMockPtr, storageMock);

    ASSERT_THAT(storedConfiguration.at("LogToJournal"),
                Eq(triggerParams.logToRedfish()));
}

TEST_F(TestTrigger, settingPersistencyToTrueStoresTriggerLogToRedfish)
{
    nlohmann::json storedConfiguration;

    EXPECT_CALL(storageMock, store(_, _))
        .WillOnce(SaveArg<1>(&storedConfiguration));

    sut = nullptr;

    sut = std::make_unique<Trigger>(
        DbusEnvironment::getIoc(), DbusEnvironment::getObjServer(),
        triggerParams.name(), triggerParams.isDiscrete(),
        triggerParams.logToJournal(), triggerParams.logToRedfish(),
        triggerParams.updateReport(), triggerParams.sensors(),
        triggerParams.reportNames(), triggerParams.thresholdParams(),
        std::vector<std::shared_ptr<interfaces::Threshold>>{},
        *triggerManagerMockPtr, storageMock);

    ASSERT_THAT(storedConfiguration.at("LogToRedfish"),
                Eq(triggerParams.logToRedfish()));
}

TEST_F(TestTrigger, settingPersistencyToTrueStoresTriggerUpdateReport)
{
    nlohmann::json storedConfiguration;

    EXPECT_CALL(storageMock, store(_, _))
        .WillOnce(SaveArg<1>(&storedConfiguration));

    sut = nullptr;

    sut = std::make_unique<Trigger>(
        DbusEnvironment::getIoc(), DbusEnvironment::getObjServer(),
        triggerParams.name(), triggerParams.isDiscrete(),
        triggerParams.logToJournal(), triggerParams.logToRedfish(),
        triggerParams.updateReport(), triggerParams.sensors(),
        triggerParams.reportNames(), triggerParams.thresholdParams(),
        std::vector<std::shared_ptr<interfaces::Threshold>>{},
        *triggerManagerMockPtr, storageMock);

    ASSERT_THAT(storedConfiguration.at("UpdateReport"),
                Eq(triggerParams.updateReport()));
}

TEST_F(TestTrigger, settingPersistencyToTrueStoresTriggerReportNames)
{
    nlohmann::json storedConfiguration;

    EXPECT_CALL(storageMock, store(_, _))
        .WillOnce(SaveArg<1>(&storedConfiguration));

    sut = nullptr;

    sut = std::make_unique<Trigger>(
        DbusEnvironment::getIoc(), DbusEnvironment::getObjServer(),
        triggerParams.name(), triggerParams.isDiscrete(),
        triggerParams.logToJournal(), triggerParams.logToRedfish(),
        triggerParams.updateReport(), triggerParams.sensors(),
        triggerParams.reportNames(), triggerParams.thresholdParams(),
        std::vector<std::shared_ptr<interfaces::Threshold>>{},
        *triggerManagerMockPtr, storageMock);

    ASSERT_THAT(storedConfiguration.at("ReportNames"),
                Eq(triggerParams.reportNames()));
}

TEST_F(TestTrigger, settingUpdateReportToFalseDoesNotStoreTriggerReportNames)
{
    nlohmann::json storedConfiguration;

    EXPECT_CALL(storageMock, store(_, _))
        .WillOnce(SaveArg<1>(&storedConfiguration));

    sut = nullptr;

    triggerParams.updateReport(false);

    sut = std::make_unique<Trigger>(
        DbusEnvironment::getIoc(), DbusEnvironment::getObjServer(),
        triggerParams.name(), triggerParams.isDiscrete(),
        triggerParams.logToJournal(), triggerParams.logToRedfish(),
        triggerParams.updateReport(), triggerParams.sensors(),
        triggerParams.reportNames(), triggerParams.thresholdParams(),
        std::vector<std::shared_ptr<interfaces::Threshold>>{},
        *triggerManagerMockPtr, storageMock);

    ASSERT_THROW(storedConfiguration.at("ReportNames"),
                 nlohmann::json::exception);
}

TEST_F(TestTrigger, settingPersistencyToTrueStoresTriggerSensors)
{
    nlohmann::json storedConfiguration;

    EXPECT_CALL(storageMock, store(_, _))
        .WillOnce(SaveArg<1>(&storedConfiguration));

    sut = nullptr;

    sut = std::make_unique<Trigger>(
        DbusEnvironment::getIoc(), DbusEnvironment::getObjServer(),
        triggerParams.name(), triggerParams.isDiscrete(),
        triggerParams.logToJournal(), triggerParams.logToRedfish(),
        triggerParams.updateReport(), triggerParams.sensors(),
        triggerParams.reportNames(), triggerParams.thresholdParams(),
        std::vector<std::shared_ptr<interfaces::Threshold>>{},
        *triggerManagerMockPtr, storageMock);

    nlohmann::json expectedItem;
    expectedItem["sensorPath"] =
        "/xyz/openbmc_project/sensors/temperature/BMC_Temp";
    expectedItem["sensorMetadata"] = "";

    ASSERT_THAT(storedConfiguration.at("Sensors"), ElementsAre(expectedItem));
}

TEST_F(TestTrigger, settingPersistencyToTrueStoresTriggerThresholdParams)
{
    nlohmann::json storedConfiguration;

    EXPECT_CALL(storageMock, store(_, _))
        .WillOnce(SaveArg<1>(&storedConfiguration));

    sut = nullptr;

    sut = std::make_unique<Trigger>(
        DbusEnvironment::getIoc(), DbusEnvironment::getObjServer(),
        triggerParams.name(), triggerParams.isDiscrete(),
        triggerParams.logToJournal(), triggerParams.logToRedfish(),
        triggerParams.updateReport(), triggerParams.sensors(),
        triggerParams.reportNames(), triggerParams.thresholdParams(),
        std::vector<std::shared_ptr<interfaces::Threshold>>{},
        *triggerManagerMockPtr, storageMock);

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
                ElementsAre(expectedItem0, expectedItem1));
}
