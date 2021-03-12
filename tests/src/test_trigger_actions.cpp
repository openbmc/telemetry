#include "mocks/report_manager_mock.hpp"
#include "trigger_actions.hpp"

#include <stdexcept>

using namespace testing;

namespace action
{

using LogNumericParam = std::tuple<numeric::Type, double, double>;
using LogDiscreteParam = discrete::Severity;

static auto getCorrectNumericParams()
{
    return Values(std::make_tuple(numeric::Type::upperCritical, 91.1, 90),
                  std::make_tuple(numeric::Type::lowerCritical, 91.2, 90),
                  std::make_tuple(numeric::Type::upperWarning, 88.5, 90),
                  std::make_tuple(numeric::Type::lowerWarning, 88.6, 90));
}

static auto getIncorrectNumericParams()
{
    return Values(std::make_tuple(numeric::Type::upperCritical, 90.0, 90),
                  std::make_tuple(static_cast<numeric::Type>(-1), 88.0, 90),
                  std::make_tuple(static_cast<numeric::Type>(123), 123.0, 90));
}

static auto getCorrectDiscreteParams()
{
    return Values(discrete::Severity::critical, discrete::Severity::warning,
                  discrete::Severity::ok);
}

static auto getIncorrectDiscreteParams()
{
    return Values(static_cast<discrete::Severity>(-1),
                  static_cast<discrete::Severity>(42));
}

class TestLogToJournalNumeric :
    public Test,
    public WithParamInterface<LogNumericParam>
{
  public:
    void SetUp() override
    {
        auto [type, threshold, value] = GetParam();
        sut = std::make_unique<LogToJournalNumeric>(type, threshold);
        commmitValue = value;
    }

    std::unique_ptr<LogToJournalNumeric> sut;
    double commmitValue;
};

INSTANTIATE_TEST_SUITE_P(LogToJournalNumericParams, TestLogToJournalNumeric,
                         getCorrectNumericParams());

TEST_P(TestLogToJournalNumeric, commitAnActionDoesNotThrow)
{
    EXPECT_NO_THROW(sut->commit("Test", 100'000, commmitValue));
}

class TestLogToJournalNumericThrow : public TestLogToJournalNumeric
{};

INSTANTIATE_TEST_SUITE_P(_, TestLogToJournalNumericThrow,
                         getIncorrectNumericParams());

TEST_P(TestLogToJournalNumericThrow, commitAnActionExpectThrow)
{
    EXPECT_THROW(sut->commit("Test", 100'000, commmitValue),
                 std::runtime_error);
}

class TestLogToRedfishNumeric :
    public Test,
    public WithParamInterface<LogNumericParam>
{
  public:
    void SetUp() override
    {
        auto [type, threshold, value] = GetParam();
        sut = std::make_unique<LogToRedfishNumeric>(type, threshold);
        commmitValue = value;
    }

    std::unique_ptr<LogToRedfishNumeric> sut;
    double commmitValue;
};

INSTANTIATE_TEST_SUITE_P(LogToRedfishNumericParams, TestLogToRedfishNumeric,
                         getCorrectNumericParams());

TEST_P(TestLogToRedfishNumeric, commitExpectNoThrow)
{
    EXPECT_NO_THROW(sut->commit("Test", 100'000, commmitValue));
}

class TestLogToRedfishNumericThrow : public TestLogToRedfishNumeric
{};

INSTANTIATE_TEST_SUITE_P(_, TestLogToRedfishNumericThrow,
                         getIncorrectNumericParams());

TEST_P(TestLogToRedfishNumericThrow, commitExpectToThrow)
{
    EXPECT_THROW(sut->commit("Test", 100'000, commmitValue),
                 std::runtime_error);
}

class TestLogToJournalDiscrete :
    public Test,
    public WithParamInterface<LogDiscreteParam>
{
  public:
    void SetUp() override
    {
        auto severity = GetParam();
        sut = std::make_unique<LogToJournalDiscrete>(severity);
    }

    std::unique_ptr<LogToJournalDiscrete> sut;
};

INSTANTIATE_TEST_SUITE_P(LogToJournalDiscreteParams, TestLogToJournalDiscrete,
                         getCorrectDiscreteParams());

TEST_P(TestLogToJournalDiscrete, commitAnActionDoesNotThrow)
{
    EXPECT_NO_THROW(sut->commit("Test", 100'000, 90.0));
}

class TestLogToJournalDiscreteThrow : public TestLogToJournalDiscrete
{};

INSTANTIATE_TEST_SUITE_P(_, TestLogToJournalDiscreteThrow,
                         getIncorrectDiscreteParams());

TEST_P(TestLogToJournalDiscreteThrow, commitAnActionExpectThrow)
{
    EXPECT_THROW(sut->commit("Test", 100'000, 90.0), std::runtime_error);
}

class TestLogToRedfishDiscrete :
    public Test,
    public WithParamInterface<LogDiscreteParam>
{
  public:
    void SetUp() override
    {
        auto severity = GetParam();
        sut = std::make_unique<LogToRedfishDiscrete>(severity);
    }

    std::unique_ptr<LogToRedfishDiscrete> sut;
};

INSTANTIATE_TEST_SUITE_P(LogToRedfishDiscreteParams, TestLogToRedfishDiscrete,
                         getCorrectDiscreteParams());

TEST_P(TestLogToRedfishDiscrete, commitExpectNoThrow)
{
    EXPECT_NO_THROW(sut->commit("Test", 100'000, 90.0));
}

class TestLogToRedfishDiscreteThrow : public TestLogToRedfishDiscrete
{};

INSTANTIATE_TEST_SUITE_P(_, TestLogToRedfishDiscreteThrow,
                         getIncorrectDiscreteParams());

TEST_P(TestLogToRedfishDiscreteThrow, commitExpectToThrow)
{
    EXPECT_THROW(sut->commit("Test", 100'000, 90.0), std::runtime_error);
}

class TestUpdateReport : public Test
{
  public:
    void make(std::vector<std::string> names)
    {
        sut = std::make_unique<UpdateReport>(reportManager, std::move(names));
    }

    NiceMock<ReportManagerMock> reportManager;
    std::unique_ptr<UpdateReport> sut;
};

TEST_F(TestUpdateReport, commitWhenReportNameIsEmptyExpectNoReportUpdate)
{
    EXPECT_CALL(reportManager, updateReport(_)).Times(0);

    make({});
    sut->commit("Test", 100'000, 90.0);
}

TEST_F(TestUpdateReport, commitExpectReportUpdate)
{
    std::vector<std::string> names = {"Report1", "Report2", "Report3"};
    for (const auto& name : names)
    {
        EXPECT_CALL(reportManager, updateReport(name));
    }

    make(names);
    sut->commit("Test", 100'000, 90.0);
}

} // namespace action
