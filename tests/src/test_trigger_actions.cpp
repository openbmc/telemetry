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
using LogParam = std::tuple<::numeric::Type, double, double>;

static auto getCorrectParams()
{
    return Values(std::make_tuple(::numeric::Type::upperCritical, 91.1, 90),
                  std::make_tuple(::numeric::Type::lowerCritical, 91.2, 90),
                  std::make_tuple(::numeric::Type::upperWarning, 88.5, 90),
                  std::make_tuple(::numeric::Type::lowerWarning, 88.6, 90));
}

static auto getIncorrectParams()
{
    return Values(
        std::make_tuple(::numeric::Type::upperCritical, 90.0, 90),
        std::make_tuple(static_cast<::numeric::Type>(-1), 88.0, 90),
        std::make_tuple(static_cast<::numeric::Type>(123), 123.0, 90));
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
    double commmitValue;
};

class TestLogToJournalNumeric : public TestActionNumeric<LogToJournal>
{};

INSTANTIATE_TEST_SUITE_P(LogToJournalNumericParams, TestLogToJournalNumeric,
                         getCorrectParams());

TEST_P(TestLogToJournalNumeric, commitAnActionDoesNotThrow)
{
    EXPECT_NO_THROW(commit());
}

class TestLogToJournalNumericThrow : public TestLogToJournalNumeric
{};

INSTANTIATE_TEST_SUITE_P(_, TestLogToJournalNumericThrow, getIncorrectParams());

TEST_P(TestLogToJournalNumericThrow, commitAnActionExpectThrow)
{
    EXPECT_ANY_THROW(commit());
}

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
using LogParam = std::tuple<::discrete::Severity, TriggerValue>;

static auto getCorrectParams()
{
    return Values(
        std::make_tuple(::discrete::Severity::critical,
                        TriggerValue("DiscreteVal")),
        std::make_tuple(::discrete::Severity::warning, TriggerValue("On")),
        std::make_tuple(::discrete::Severity::ok, TriggerValue("Off")));
}

static auto getIncorrectParams()
{
    return Values(
        std::make_tuple(static_cast<::discrete::Severity>(-1),
                        TriggerValue("DiscreteVal42")),
        std::make_tuple(static_cast<::discrete::Severity>(42),
                        TriggerValue("On")),
        std::make_tuple(::discrete::Severity::critical, TriggerValue(42.0)),
        std::make_tuple(::discrete::Severity::warning, TriggerValue(0.0)),
        std::make_tuple(::discrete::Severity::ok, TriggerValue(0.1)));
}

template <typename ActionType>
class TestActionDiscrete : public Test, public WithParamInterface<LogParam>
{
  public:
    void SetUp() override
    {
        auto [severity, value] = GetParam();
        sut = std::make_unique<ActionType>(severity);
        commitValue = value;
    }

    void commit()
    {
        std::string thresholdName = "MyThreshold";
        sut->commit("MyTrigger", std::cref(thresholdName), "MySensor",
                    Milliseconds{100'000}, commitValue);
    }

    TriggerValue commitValue;
    std::unique_ptr<ActionType> sut;
};

class TestLogToJournalDiscrete : public TestActionDiscrete<LogToJournal>
{};

INSTANTIATE_TEST_SUITE_P(LogToJournalDiscreteParams, TestLogToJournalDiscrete,
                         getCorrectParams());

TEST_P(TestLogToJournalDiscrete, commitAnActionWIthDiscreteValueDoesNotThrow)
{
    EXPECT_NO_THROW(commit());
}

class TestLogToJournalDiscreteThrow : public TestLogToJournalDiscrete
{};

INSTANTIATE_TEST_SUITE_P(_, TestLogToJournalDiscreteThrow,
                         getIncorrectParams());

TEST_P(TestLogToJournalDiscreteThrow, commitAnActionExpectThrow)
{
    EXPECT_ANY_THROW(commit());
}

class TestLogToRedfishEventLogDiscrete :
    public TestActionDiscrete<LogToRedfishEventLog>
{};

INSTANTIATE_TEST_SUITE_P(LogToRedfishEventLogDiscreteParams,
                         TestLogToRedfishEventLogDiscrete, getCorrectParams());

TEST_P(TestLogToRedfishEventLogDiscrete, commitExpectNoThrow)
{
    EXPECT_NO_THROW(commit());
}

class TestLogToRedfishEventLogDiscreteThrow :
    public TestLogToRedfishEventLogDiscrete
{};

INSTANTIATE_TEST_SUITE_P(_, TestLogToRedfishEventLogDiscreteThrow,
                         getIncorrectParams());

TEST_P(TestLogToRedfishEventLogDiscreteThrow, commitExpectToThrow)
{
    EXPECT_ANY_THROW(commit());
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

class TestLogToJournalDiscreteOnChange : public TestActionOnChange<LogToJournal>
{};

TEST_F(TestLogToJournalDiscreteOnChange, commitNumericValueExpectNoThrow)
{
    EXPECT_NO_THROW(commit(90.0));
}

TEST_F(TestLogToJournalDiscreteOnChange, commitDiscreteValueExpectNoThrow)
{
    EXPECT_NO_THROW(commit("Off"));
}

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
    TestUpdateReport() : messanger(DbusEnvironment::getIoc())
    {}

    void make(std::vector<std::string> names)
    {
        messanger.on_receive<messages::UpdateReportInd>(
            [this](const auto& msg) { updateReport.Call(msg); });

        sut = std::make_unique<UpdateReport>(
            DbusEnvironment::getIoc(),
            std::make_shared<std::vector<std::string>>(std::move(names)));
    }

    void commit(TriggerValue value)
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
