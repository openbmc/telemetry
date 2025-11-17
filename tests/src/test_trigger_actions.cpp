#include "dbus_environment.hpp"
#include "helpers.hpp"
#include "messages/update_report_ind.hpp"
#include "trigger_actions.hpp"
#include "utils/messanger.hpp"

#include <stdexcept>

#include <gmock/gmock.h>

using namespace testing;

namespace action
{
namespace numeric
{
using LogParam = std::tuple<::numeric::Type, double, TriggerValue>;

static auto getCorrectParams()
{
    return Values(
        std::make_tuple(::numeric::Type::upperCritical, 91.1,
                        TriggerValue(90.0)),
        std::make_tuple(::numeric::Type::upperCritical, 90, TriggerValue(91.1)),
        std::make_tuple(::numeric::Type::lowerCritical, 91.2,
                        TriggerValue(90.0)),
        std::make_tuple(::numeric::Type::lowerCritical, 90, TriggerValue(91.2)),
        std::make_tuple(::numeric::Type::upperWarning, 88.5,
                        TriggerValue(90.0)),
        std::make_tuple(::numeric::Type::upperWarning, 90, TriggerValue(88.5)),
        std::make_tuple(::numeric::Type::lowerWarning, 88.6,
                        TriggerValue(90.0)),
        std::make_tuple(::numeric::Type::lowerWarning, 90, TriggerValue(88.6)));
}

static auto getIncorrectParams()
{
    return Values(std::make_tuple(::numeric::Type::upperCritical, 90.0,
                                  TriggerValue(90.0)),
                  std::make_tuple(static_cast<::numeric::Type>(-1), 88.0,
                                  TriggerValue(90.0)),
                  std::make_tuple(static_cast<::numeric::Type>(123), 123.0,
                                  TriggerValue(90.0)),
                  std::make_tuple(::numeric::Type::upperCritical, 90.0,
                                  TriggerValue("numeric")));
}

template <typename ActionType>
class TestActionNumeric : public Test, public WithParamInterface<LogParam>
{
  public:
    void SetUp() override
    {
        auto [type, threshold, value] = GetParam();
        sut = std::make_unique<ActionType>(type, threshold);
        commmitValue = value;
    }

    void commit()
    {
        sut->commit("MyTrigger", std::nullopt, "MySensor",
                    Milliseconds{100'000}, commmitValue);
    }

    std::unique_ptr<ActionType> sut;
    TriggerValue commmitValue;
};

class TestLogToRedfishEventLogNumeric :
    public TestActionNumeric<LogToRedfishEventLog>
{};

INSTANTIATE_TEST_SUITE_P(LogToRedfishEventLogNumericParams,
                         TestLogToRedfishEventLogNumeric, getCorrectParams());

TEST_P(TestLogToRedfishEventLogNumeric, commitExpectNoThrow)
{
    EXPECT_NO_THROW(commit());
}

class TestLogToRedfishEventLogNumericThrow :
    public TestLogToRedfishEventLogNumeric
{};

INSTANTIATE_TEST_SUITE_P(_, TestLogToRedfishEventLogNumericThrow,
                         getIncorrectParams());

TEST_P(TestLogToRedfishEventLogNumericThrow, commitExpectToThrow)
{
    EXPECT_ANY_THROW(commit());
}

} // namespace numeric

namespace discrete
{

class TestLogToRedfishEventLogDiscrete : public Test
{
  public:
    void SetUp()
    {
        sut = std::make_unique<LogToRedfishEventLog>();
    }

    void commit(TriggerValue value) const
    {
        std::string thresholdName = "MyThreshold";
        sut->commit("MyTrigger", std::cref(thresholdName), "MySensor",
                    Milliseconds{100'000}, value);
    }

    std::unique_ptr<LogToRedfishEventLog> sut;
};

TEST_F(TestLogToRedfishEventLogDiscrete, commitDiscreteValueExpectNoThrow)
{
    EXPECT_NO_THROW(commit("DiscreteVal"));
}

TEST_F(TestLogToRedfishEventLogDiscrete, commitNumericValueExpectToThrow)
{
    EXPECT_ANY_THROW(commit(42.0));
}

namespace onChange
{

template <typename ActionType>
class TestActionOnChange : public Test
{
  public:
    void SetUp() override
    {
        sut = std::make_unique<ActionType>();
    }

    void commit(TriggerValue value)
    {
        sut->commit("MyTrigger", std::nullopt, "MySensor",
                    Milliseconds{100'000}, value);
    }

    std::unique_ptr<ActionType> sut;
};

class TestLogToRedfishEventLogDiscreteOnChange :
    public TestActionOnChange<LogToRedfishEventLog>
{};

TEST_F(TestLogToRedfishEventLogDiscreteOnChange,
       commitNumericValueExpectNoThrow)
{
    EXPECT_NO_THROW(commit(90.0));
}

TEST_F(TestLogToRedfishEventLogDiscreteOnChange,
       commitDiscreteValueExpectNoThrow)
{
    EXPECT_NO_THROW(commit("Off"));
}

} // namespace onChange
} // namespace discrete

class TestUpdateReport : public Test
{
  public:
    TestUpdateReport() : messanger(DbusEnvironment::getIoc()) {}

    void make(std::vector<std::string> names)
    {
        messanger.on_receive<messages::UpdateReportInd>(
            [this](const auto& msg) { updateReport.Call(msg); });

        sut = std::make_unique<UpdateReport>(
            DbusEnvironment::getIoc(),
            std::make_shared<std::vector<std::string>>(std::move(names)));
    }

    void commit(TriggerValue value) const
    {
        sut->commit(triggerId, std::nullopt, "MySensor", Milliseconds{100'000},
                    value);
    }

    utils::Messanger messanger;
    NiceMock<MockFunction<void(const messages::UpdateReportInd&)>> updateReport;
    std::unique_ptr<UpdateReport> sut;
    std::string triggerId = "MyTrigger";
};

TEST_F(TestUpdateReport, commitWhenReportNameIsEmptyExpectNoReportUpdate)
{
    EXPECT_CALL(updateReport, Call(_)).Times(0);

    make({});
    commit(90.0);
}

TEST_F(TestUpdateReport, commitExpectReportUpdate)
{
    std::vector<std::string> names = {"Report1", "Report2", "Report3"};
    EXPECT_CALL(updateReport,
                Call(FieldsAre(UnorderedElementsAreArray(names))));

    make(names);
    commit(90.0);
}

} // namespace action
