#include "dbus_environment.hpp"
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

class TestLogToJournalNumeric : public Test, public WithParamInterface<LogParam>
{
  public:
    void SetUp() override
    {
        auto [type, threshold, value] = GetParam();
        sut = std::make_unique<numeric::LogToJournal>(type, threshold);
        commmitValue = value;
    }

    std::unique_ptr<numeric::LogToJournal> sut;
    double commmitValue;
};

INSTANTIATE_TEST_SUITE_P(LogToJournalNumericParams, TestLogToJournalNumeric,
                         getCorrectParams());

TEST_P(TestLogToJournalNumeric, commitAnActionDoesNotThrow)
{
    EXPECT_NO_THROW(sut->commit("Test", Milliseconds{100'000}, commmitValue));
}

class TestLogToJournalNumericThrow : public TestLogToJournalNumeric
{};

INSTANTIATE_TEST_SUITE_P(_, TestLogToJournalNumericThrow, getIncorrectParams());

TEST_P(TestLogToJournalNumericThrow, commitAnActionExpectThrow)
{
    EXPECT_ANY_THROW(sut->commit("Test", Milliseconds{100'000}, commmitValue));
}

class TestLogToRedfishNumeric : public Test, public WithParamInterface<LogParam>
{
  public:
    void SetUp() override
    {
        auto [type, threshold, value] = GetParam();
        sut = std::make_unique<LogToRedfish>(type, threshold);
        commmitValue = value;
    }

    std::unique_ptr<LogToRedfish> sut;
    double commmitValue;
};

INSTANTIATE_TEST_SUITE_P(LogToRedfishNumericParams, TestLogToRedfishNumeric,
                         getCorrectParams());

TEST_P(TestLogToRedfishNumeric, commitExpectNoThrow)
{
    EXPECT_NO_THROW(sut->commit("Test", Milliseconds{100'000}, commmitValue));
}

class TestLogToRedfishNumericThrow : public TestLogToRedfishNumeric
{};

INSTANTIATE_TEST_SUITE_P(_, TestLogToRedfishNumericThrow, getIncorrectParams());

TEST_P(TestLogToRedfishNumericThrow, commitExpectToThrow)
{
    EXPECT_THROW(sut->commit("Test", Milliseconds{100'000}, commmitValue),
                 std::runtime_error);
}

} // namespace numeric

namespace discrete
{
using LogParam = ::discrete::Severity;

static auto getCorrectParams()
{
    return Values(::discrete::Severity::critical, ::discrete::Severity::warning,
                  ::discrete::Severity::ok);
}

static auto getIncorrectParams()
{
    return Values(static_cast<::discrete::Severity>(-1),
                  static_cast<::discrete::Severity>(42));
}

class TestLogToJournalDiscrete :
    public Test,
    public WithParamInterface<LogParam>
{
  public:
    void SetUp() override
    {
        auto severity = GetParam();
        sut = std::make_unique<LogToJournal>(severity);
    }

    std::unique_ptr<LogToJournal> sut;
};

INSTANTIATE_TEST_SUITE_P(LogToJournalDiscreteParams, TestLogToJournalDiscrete,
                         getCorrectParams());

TEST_P(TestLogToJournalDiscrete, commitAnActionDoesNotThrow)
{
    EXPECT_NO_THROW(sut->commit("Test", Milliseconds{100'000}, 90.0));
}

class TestLogToJournalDiscreteThrow : public TestLogToJournalDiscrete
{};

INSTANTIATE_TEST_SUITE_P(_, TestLogToJournalDiscreteThrow,
                         getIncorrectParams());

TEST_P(TestLogToJournalDiscreteThrow, commitAnActionExpectThrow)
{
    EXPECT_ANY_THROW(sut->commit("Test", Milliseconds{100'000}, 90.0));
}

class TestLogToRedfishDiscrete :
    public Test,
    public WithParamInterface<LogParam>
{
  public:
    void SetUp() override
    {
        auto severity = GetParam();
        sut = std::make_unique<LogToRedfish>(severity);
    }

    std::unique_ptr<LogToRedfish> sut;
};

INSTANTIATE_TEST_SUITE_P(LogToRedfishDiscreteParams, TestLogToRedfishDiscrete,
                         getCorrectParams());

TEST_P(TestLogToRedfishDiscrete, commitExpectNoThrow)
{
    EXPECT_NO_THROW(sut->commit("Test", Milliseconds{100'000}, 90.0));
}

class TestLogToRedfishDiscreteThrow : public TestLogToRedfishDiscrete
{};

INSTANTIATE_TEST_SUITE_P(_, TestLogToRedfishDiscreteThrow,
                         getIncorrectParams());

TEST_P(TestLogToRedfishDiscreteThrow, commitExpectToThrow)
{
    EXPECT_THROW(sut->commit("Test", Milliseconds{100'000}, 90.0),
                 std::runtime_error);
}

namespace onChange
{
class TestLogToJournalDiscreteOnChange : public Test
{
  public:
    void SetUp() override
    {
        sut = std::make_unique<LogToJournal>();
    }

    std::unique_ptr<LogToJournal> sut;
};

TEST_F(TestLogToJournalDiscreteOnChange, commitExpectNoThrow)
{
    EXPECT_NO_THROW(sut->commit("Test", Milliseconds{100'000}, 90.0));
}

class TestLogToRedfishDiscreteOnChange : public Test
{
  public:
    void SetUp() override
    {
        sut = std::make_unique<LogToRedfish>();
    }

    std::unique_ptr<LogToRedfish> sut;
};

TEST_F(TestLogToRedfishDiscreteOnChange, commitExpectNoThrow)
{
    EXPECT_NO_THROW(sut->commit("Test", Milliseconds{100'000}, 90.0));
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

    utils::Messanger messanger;
    NiceMock<MockFunction<void(const messages::UpdateReportInd&)>> updateReport;
    std::unique_ptr<UpdateReport> sut;
};

TEST_F(TestUpdateReport, commitWhenReportNameIsEmptyExpectNoReportUpdate)
{
    EXPECT_CALL(updateReport, Call(_)).Times(0);

    make({});
    sut->commit("Test", Milliseconds{100'000}, 90.0);
}

TEST_F(TestUpdateReport, commitExpectReportUpdate)
{
    std::vector<std::string> names = {"Report1", "Report2", "Report3"};
    EXPECT_CALL(updateReport,
                Call(FieldsAre(UnorderedElementsAreArray(names))));

    make(names);
    sut->commit("Test", Milliseconds{100'000}, 90.0);
}

} // namespace action
