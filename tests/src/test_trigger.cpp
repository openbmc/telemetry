#include "dbus_environment.hpp"
#include "helpers.hpp"
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
            *triggerManagerMockPtr);
    }

    template <class T>
    static T getProperty(const std::string& path, const std::string& property)
    {
        auto propertyPromise = std::promise<T>{};
        auto promiseFuture = propertyPromise.get_future();
        sdbusplus::asio::getProperty<T>(
            *DbusEnvironment::getBus(), DbusEnvironment::serviceName(), path,
            Trigger::triggerIfaceName, property,
            [&propertyPromise](boost::system::error_code ec, T t) {
                if (ec)
                {
                    utils::setException(propertyPromise, "GetProperty failed");
                    return;
                }
                propertyPromise.set_value(t);
            });
        return DbusEnvironment::waitForFuture(std::move(promiseFuture));
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
    EXPECT_CALL(*triggerManagerMockPtr, removeTrigger(sut.get()));
    auto ec = deleteTrigger(sut->getPath());
    EXPECT_THAT(ec, Eq(boost::system::errc::success));
}

TEST_F(TestTrigger, deletingNonExistingTriggerReturnInvalidRequestDescriptor)
{
    auto ec = deleteTrigger(Trigger::triggerDir + "NonExisting"s);
    EXPECT_THAT(ec.value(), Eq(EBADR));
}
