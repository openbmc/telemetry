#include "mocks/report_manager_mock.hpp"
#include "trigger_actions.hpp"

#include <stdexcept>

using namespace testing;

namespace action
{

using LogParam = std::pair<numeric::Type, double>;

class TestLogToJournal : public Test, public WithParamInterface<LogParam>
{
  public:
    void SetUp() override
    {
        auto [type, threshold] = GetParam();
        sut = std::make_unique<LogToJournal>(type, threshold);
    }

    std::unique_ptr<LogToJournal> sut;
};

INSTANTIATE_TEST_SUITE_P(
    LogToJournalParams, TestLogToJournal,
    Values(std::make_pair(numeric::Type::upperCritical, 91.1),
           std::make_pair(numeric::Type::lowerCritical, 91.2),
           std::make_pair(numeric::Type::upperWarning, 88.5),
           std::make_pair(numeric::Type::lowerWarning, 88.6)));

TEST_P(TestLogToJournal, commitAnActionDoesNotThrow)
{
    EXPECT_NO_THROW(sut->commit("Test", 100'000, 90.0));
}

class TestLogToJournalThrow : public TestLogToJournal
{};

INSTANTIATE_TEST_SUITE_P(
    _, TestLogToJournalThrow,
    Values(std::make_pair(numeric::Type::upperCritical, 90.0),
           std::make_pair(static_cast<numeric::Type>(-1), 88.0),
           std::make_pair(static_cast<numeric::Type>(123), 123.0)));

TEST_P(TestLogToJournalThrow, commitAnActionExpectThrow)
{
    EXPECT_THROW(sut->commit("Test", 100'000, 90.0), std::runtime_error);
}

class TestLogToRedfish : public Test, public WithParamInterface<LogParam>
{
  public:
    void SetUp() override
    {
        auto [type, threshold] = GetParam();
        sut = std::make_unique<LogToRedfish>(type, threshold);
    }

    std::unique_ptr<LogToRedfish> sut;
};

INSTANTIATE_TEST_SUITE_P(
    LogToRedfishParams, TestLogToRedfish,
    Values(std::make_pair(numeric::Type::upperCritical, 91.1),
           std::make_pair(numeric::Type::lowerCritical, 91.4),
           std::make_pair(numeric::Type::upperWarning, 88.6),
           std::make_pair(numeric::Type::lowerWarning, 88.5)));

TEST_P(TestLogToRedfish, commitExpectNoThrow)
{
    EXPECT_NO_THROW(sut->commit("Test", 100'000, 90.0));
}

class TestLogToRedfishThrow : public TestLogToRedfish
{};

INSTANTIATE_TEST_SUITE_P(
    _, TestLogToRedfishThrow,
    Values(std::make_pair(numeric::Type::upperCritical, 90.0),
           std::make_pair(static_cast<numeric::Type>(-1), 88.5),
           std::make_pair(static_cast<numeric::Type>(123), 123.6)));

TEST_P(TestLogToRedfishThrow, commitExpectToThrow)
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
